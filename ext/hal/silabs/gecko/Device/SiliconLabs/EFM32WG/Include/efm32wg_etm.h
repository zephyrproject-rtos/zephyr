/**************************************************************************//**
 * @file efm32wg_etm.h
 * @brief EFM32WG_ETM register and bit field definitions
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
 * @defgroup EFM32WG_ETM
 * @{
 * @brief EFM32WG_ETM Register Declaration
 *****************************************************************************/
typedef struct
{
  __IOM uint32_t ETMCR;           /**< Main Control Register  */
  __IM uint32_t  ETMCCR;          /**< Configuration Code Register  */
  __IOM uint32_t ETMTRIGGER;      /**< ETM Trigger Event Register  */
  uint32_t       RESERVED0[1];    /**< Reserved for future use **/
  __IOM uint32_t ETMSR;           /**< ETM Status Register  */
  __IM uint32_t  ETMSCR;          /**< ETM System Configuration Register  */
  uint32_t       RESERVED1[2];    /**< Reserved for future use **/
  __IOM uint32_t ETMTEEVR;        /**< ETM TraceEnable Event Register  */
  __IOM uint32_t ETMTECR1;        /**< ETM Trace control Register  */
  uint32_t       RESERVED2[1];    /**< Reserved for future use **/
  __IOM uint32_t ETMFFLR;         /**< ETM Fifo Full Level Register  */
  uint32_t       RESERVED3[68];   /**< Reserved for future use **/
  __IOM uint32_t ETMCNTRLDVR1;    /**< Counter Reload Value  */
  uint32_t       RESERVED4[39];   /**< Reserved for future use **/
  __IOM uint32_t ETMSYNCFR;       /**< Synchronisation Frequency Register  */
  __IM uint32_t  ETMIDR;          /**< ID Register  */
  __IM uint32_t  ETMCCER;         /**< Configuration Code Extension Register  */
  uint32_t       RESERVED5[1];    /**< Reserved for future use **/
  __IOM uint32_t ETMTESSEICR;     /**< TraceEnable Start/Stop EmbeddedICE Control Register  */
  uint32_t       RESERVED6[1];    /**< Reserved for future use **/
  __IOM uint32_t ETMTSEVR;        /**< Timestamp Event Register  */
  uint32_t       RESERVED7[1];    /**< Reserved for future use **/
  __IOM uint32_t ETMTRACEIDR;     /**< CoreSight Trace ID Register  */
  uint32_t       RESERVED8[1];    /**< Reserved for future use **/
  __IM uint32_t  ETMIDR2;         /**< ETM ID Register 2  */
  uint32_t       RESERVED9[66];   /**< Reserved for future use **/
  __IM uint32_t  ETMPDSR;         /**< Device Power-down Status Register  */
  uint32_t       RESERVED10[754]; /**< Reserved for future use **/
  __IOM uint32_t ETMISCIN;        /**< Integration Test Miscellaneous Inputs Register  */
  uint32_t       RESERVED11[1];   /**< Reserved for future use **/
  __OM uint32_t  ITTRIGOUT;       /**< Integration Test Trigger Out Register  */
  uint32_t       RESERVED12[1];   /**< Reserved for future use **/
  __IM uint32_t  ETMITATBCTR2;    /**< ETM Integration Test ATB Control 2 Register  */
  uint32_t       RESERVED13[1];   /**< Reserved for future use **/
  __OM uint32_t  ETMITATBCTR0;    /**< ETM Integration Test ATB Control 0 Register  */
  uint32_t       RESERVED14[1];   /**< Reserved for future use **/
  __IOM uint32_t ETMITCTRL;       /**< ETM Integration Control Register  */
  uint32_t       RESERVED15[39];  /**< Reserved for future use **/
  __IOM uint32_t ETMCLAIMSET;     /**< ETM Claim Tag Set Register  */
  __IOM uint32_t ETMCLAIMCLR;     /**< ETM Claim Tag Clear Register  */
  uint32_t       RESERVED16[2];   /**< Reserved for future use **/
  __IOM uint32_t ETMLAR;          /**< ETM Lock Access Register  */
  __IM uint32_t  ETMLSR;          /**< Lock Status Register  */
  __IM uint32_t  ETMAUTHSTATUS;   /**< ETM Authentication Status Register  */
  uint32_t       RESERVED17[4];   /**< Reserved for future use **/
  __IM uint32_t  ETMDEVTYPE;      /**< CoreSight Device Type Register  */
  __IM uint32_t  ETMPIDR4;        /**< Peripheral ID4 Register  */
  __OM uint32_t  ETMPIDR5;        /**< Peripheral ID5 Register  */
  __OM uint32_t  ETMPIDR6;        /**< Peripheral ID6 Register  */
  __OM uint32_t  ETMPIDR7;        /**< Peripheral ID7 Register  */
  __IM uint32_t  ETMPIDR0;        /**< Peripheral ID0 Register  */
  __IM uint32_t  ETMPIDR1;        /**< Peripheral ID1 Register  */
  __IM uint32_t  ETMPIDR2;        /**< Peripheral ID2 Register  */
  __IM uint32_t  ETMPIDR3;        /**< Peripheral ID3 Register  */
  __IM uint32_t  ETMCIDR0;        /**< Component ID0 Register  */
  __IM uint32_t  ETMCIDR1;        /**< Component ID1 Register  */
  __IM uint32_t  ETMCIDR2;        /**< Component ID2 Register  */
  __IM uint32_t  ETMCIDR3;        /**< Component ID3 Register  */
} ETM_TypeDef;                    /** @} */

/**************************************************************************//**
 * @defgroup EFM32WG_ETM_BitFields
 * @{
 *****************************************************************************/

/* Bit fields for ETM ETMCR */
#define _ETM_ETMCR_RESETVALUE                         0x00000411UL                           /**< Default value for ETM_ETMCR */
#define _ETM_ETMCR_MASK                               0x10632FF1UL                           /**< Mask for ETM_ETMCR */
#define ETM_ETMCR_POWERDWN                            (0x1UL << 0)                           /**< ETM Control in low power mode */
#define _ETM_ETMCR_POWERDWN_SHIFT                     0                                      /**< Shift value for ETM_POWERDWN */
#define _ETM_ETMCR_POWERDWN_MASK                      0x1UL                                  /**< Bit mask for ETM_POWERDWN */
#define _ETM_ETMCR_POWERDWN_DEFAULT                   0x00000001UL                           /**< Mode DEFAULT for ETM_ETMCR */
#define ETM_ETMCR_POWERDWN_DEFAULT                    (_ETM_ETMCR_POWERDWN_DEFAULT << 0)     /**< Shifted mode DEFAULT for ETM_ETMCR */
#define _ETM_ETMCR_PORTSIZE_SHIFT                     4                                      /**< Shift value for ETM_PORTSIZE */
#define _ETM_ETMCR_PORTSIZE_MASK                      0x70UL                                 /**< Bit mask for ETM_PORTSIZE */
#define _ETM_ETMCR_PORTSIZE_DEFAULT                   0x00000001UL                           /**< Mode DEFAULT for ETM_ETMCR */
#define ETM_ETMCR_PORTSIZE_DEFAULT                    (_ETM_ETMCR_PORTSIZE_DEFAULT << 4)     /**< Shifted mode DEFAULT for ETM_ETMCR */
#define ETM_ETMCR_STALL                               (0x1UL << 7)                           /**< Stall Processor */
#define _ETM_ETMCR_STALL_SHIFT                        7                                      /**< Shift value for ETM_STALL */
#define _ETM_ETMCR_STALL_MASK                         0x80UL                                 /**< Bit mask for ETM_STALL */
#define _ETM_ETMCR_STALL_DEFAULT                      0x00000000UL                           /**< Mode DEFAULT for ETM_ETMCR */
#define ETM_ETMCR_STALL_DEFAULT                       (_ETM_ETMCR_STALL_DEFAULT << 7)        /**< Shifted mode DEFAULT for ETM_ETMCR */
#define ETM_ETMCR_BRANCHOUTPUT                        (0x1UL << 8)                           /**< Branch Output */
#define _ETM_ETMCR_BRANCHOUTPUT_SHIFT                 8                                      /**< Shift value for ETM_BRANCHOUTPUT */
#define _ETM_ETMCR_BRANCHOUTPUT_MASK                  0x100UL                                /**< Bit mask for ETM_BRANCHOUTPUT */
#define _ETM_ETMCR_BRANCHOUTPUT_DEFAULT               0x00000000UL                           /**< Mode DEFAULT for ETM_ETMCR */
#define ETM_ETMCR_BRANCHOUTPUT_DEFAULT                (_ETM_ETMCR_BRANCHOUTPUT_DEFAULT << 8) /**< Shifted mode DEFAULT for ETM_ETMCR */
#define ETM_ETMCR_DBGREQCTRL                          (0x1UL << 9)                           /**< Debug Request Control */
#define _ETM_ETMCR_DBGREQCTRL_SHIFT                   9                                      /**< Shift value for ETM_DBGREQCTRL */
#define _ETM_ETMCR_DBGREQCTRL_MASK                    0x200UL                                /**< Bit mask for ETM_DBGREQCTRL */
#define _ETM_ETMCR_DBGREQCTRL_DEFAULT                 0x00000000UL                           /**< Mode DEFAULT for ETM_ETMCR */
#define ETM_ETMCR_DBGREQCTRL_DEFAULT                  (_ETM_ETMCR_DBGREQCTRL_DEFAULT << 9)   /**< Shifted mode DEFAULT for ETM_ETMCR */
#define ETM_ETMCR_ETMPROG                             (0x1UL << 10)                          /**< ETM Programming */
#define _ETM_ETMCR_ETMPROG_SHIFT                      10                                     /**< Shift value for ETM_ETMPROG */
#define _ETM_ETMCR_ETMPROG_MASK                       0x400UL                                /**< Bit mask for ETM_ETMPROG */
#define _ETM_ETMCR_ETMPROG_DEFAULT                    0x00000001UL                           /**< Mode DEFAULT for ETM_ETMCR */
#define ETM_ETMCR_ETMPROG_DEFAULT                     (_ETM_ETMCR_ETMPROG_DEFAULT << 10)     /**< Shifted mode DEFAULT for ETM_ETMCR */
#define ETM_ETMCR_ETMPORTSEL                          (0x1UL << 11)                          /**< ETM Port Selection */
#define _ETM_ETMCR_ETMPORTSEL_SHIFT                   11                                     /**< Shift value for ETM_ETMPORTSEL */
#define _ETM_ETMCR_ETMPORTSEL_MASK                    0x800UL                                /**< Bit mask for ETM_ETMPORTSEL */
#define _ETM_ETMCR_ETMPORTSEL_DEFAULT                 0x00000000UL                           /**< Mode DEFAULT for ETM_ETMCR */
#define _ETM_ETMCR_ETMPORTSEL_ETMLOW                  0x00000000UL                           /**< Mode ETMLOW for ETM_ETMCR */
#define _ETM_ETMCR_ETMPORTSEL_ETMHIGH                 0x00000001UL                           /**< Mode ETMHIGH for ETM_ETMCR */
#define ETM_ETMCR_ETMPORTSEL_DEFAULT                  (_ETM_ETMCR_ETMPORTSEL_DEFAULT << 11)  /**< Shifted mode DEFAULT for ETM_ETMCR */
#define ETM_ETMCR_ETMPORTSEL_ETMLOW                   (_ETM_ETMCR_ETMPORTSEL_ETMLOW << 11)   /**< Shifted mode ETMLOW for ETM_ETMCR */
#define ETM_ETMCR_ETMPORTSEL_ETMHIGH                  (_ETM_ETMCR_ETMPORTSEL_ETMHIGH << 11)  /**< Shifted mode ETMHIGH for ETM_ETMCR */
#define ETM_ETMCR_PORTMODE2                           (0x1UL << 13)                          /**< Port Mode[2] */
#define _ETM_ETMCR_PORTMODE2_SHIFT                    13                                     /**< Shift value for ETM_PORTMODE2 */
#define _ETM_ETMCR_PORTMODE2_MASK                     0x2000UL                               /**< Bit mask for ETM_PORTMODE2 */
#define _ETM_ETMCR_PORTMODE2_DEFAULT                  0x00000000UL                           /**< Mode DEFAULT for ETM_ETMCR */
#define ETM_ETMCR_PORTMODE2_DEFAULT                   (_ETM_ETMCR_PORTMODE2_DEFAULT << 13)   /**< Shifted mode DEFAULT for ETM_ETMCR */
#define _ETM_ETMCR_PORTMODE_SHIFT                     16                                     /**< Shift value for ETM_PORTMODE */
#define _ETM_ETMCR_PORTMODE_MASK                      0x30000UL                              /**< Bit mask for ETM_PORTMODE */
#define _ETM_ETMCR_PORTMODE_DEFAULT                   0x00000000UL                           /**< Mode DEFAULT for ETM_ETMCR */
#define ETM_ETMCR_PORTMODE_DEFAULT                    (_ETM_ETMCR_PORTMODE_DEFAULT << 16)    /**< Shifted mode DEFAULT for ETM_ETMCR */
#define _ETM_ETMCR_EPORTSIZE_SHIFT                    21                                     /**< Shift value for ETM_EPORTSIZE */
#define _ETM_ETMCR_EPORTSIZE_MASK                     0x600000UL                             /**< Bit mask for ETM_EPORTSIZE */
#define _ETM_ETMCR_EPORTSIZE_DEFAULT                  0x00000000UL                           /**< Mode DEFAULT for ETM_ETMCR */
#define ETM_ETMCR_EPORTSIZE_DEFAULT                   (_ETM_ETMCR_EPORTSIZE_DEFAULT << 21)   /**< Shifted mode DEFAULT for ETM_ETMCR */
#define ETM_ETMCR_TSTAMPEN                            (0x1UL << 28)                          /**< Time Stamp Enable */
#define _ETM_ETMCR_TSTAMPEN_SHIFT                     28                                     /**< Shift value for ETM_TSTAMPEN */
#define _ETM_ETMCR_TSTAMPEN_MASK                      0x10000000UL                           /**< Bit mask for ETM_TSTAMPEN */
#define _ETM_ETMCR_TSTAMPEN_DEFAULT                   0x00000000UL                           /**< Mode DEFAULT for ETM_ETMCR */
#define ETM_ETMCR_TSTAMPEN_DEFAULT                    (_ETM_ETMCR_TSTAMPEN_DEFAULT << 28)    /**< Shifted mode DEFAULT for ETM_ETMCR */

/* Bit fields for ETM ETMCCR */
#define _ETM_ETMCCR_RESETVALUE                        0x8C802000UL                             /**< Default value for ETM_ETMCCR */
#define _ETM_ETMCCR_MASK                              0x8FFFFFFFUL                             /**< Mask for ETM_ETMCCR */
#define _ETM_ETMCCR_ADRCMPPAIR_SHIFT                  0                                        /**< Shift value for ETM_ADRCMPPAIR */
#define _ETM_ETMCCR_ADRCMPPAIR_MASK                   0xFUL                                    /**< Bit mask for ETM_ADRCMPPAIR */
#define _ETM_ETMCCR_ADRCMPPAIR_DEFAULT                0x00000000UL                             /**< Mode DEFAULT for ETM_ETMCCR */
#define ETM_ETMCCR_ADRCMPPAIR_DEFAULT                 (_ETM_ETMCCR_ADRCMPPAIR_DEFAULT << 0)    /**< Shifted mode DEFAULT for ETM_ETMCCR */
#define _ETM_ETMCCR_DATACMPNUM_SHIFT                  4                                        /**< Shift value for ETM_DATACMPNUM */
#define _ETM_ETMCCR_DATACMPNUM_MASK                   0xF0UL                                   /**< Bit mask for ETM_DATACMPNUM */
#define _ETM_ETMCCR_DATACMPNUM_DEFAULT                0x00000000UL                             /**< Mode DEFAULT for ETM_ETMCCR */
#define ETM_ETMCCR_DATACMPNUM_DEFAULT                 (_ETM_ETMCCR_DATACMPNUM_DEFAULT << 4)    /**< Shifted mode DEFAULT for ETM_ETMCCR */
#define _ETM_ETMCCR_MMDECCNT_SHIFT                    8                                        /**< Shift value for ETM_MMDECCNT */
#define _ETM_ETMCCR_MMDECCNT_MASK                     0x1F00UL                                 /**< Bit mask for ETM_MMDECCNT */
#define _ETM_ETMCCR_MMDECCNT_DEFAULT                  0x00000000UL                             /**< Mode DEFAULT for ETM_ETMCCR */
#define ETM_ETMCCR_MMDECCNT_DEFAULT                   (_ETM_ETMCCR_MMDECCNT_DEFAULT << 8)      /**< Shifted mode DEFAULT for ETM_ETMCCR */
#define _ETM_ETMCCR_COUNTNUM_SHIFT                    13                                       /**< Shift value for ETM_COUNTNUM */
#define _ETM_ETMCCR_COUNTNUM_MASK                     0xE000UL                                 /**< Bit mask for ETM_COUNTNUM */
#define _ETM_ETMCCR_COUNTNUM_DEFAULT                  0x00000001UL                             /**< Mode DEFAULT for ETM_ETMCCR */
#define ETM_ETMCCR_COUNTNUM_DEFAULT                   (_ETM_ETMCCR_COUNTNUM_DEFAULT << 13)     /**< Shifted mode DEFAULT for ETM_ETMCCR */
#define ETM_ETMCCR_SEQPRES                            (0x1UL << 16)                            /**< Sequencer Present */
#define _ETM_ETMCCR_SEQPRES_SHIFT                     16                                       /**< Shift value for ETM_SEQPRES */
#define _ETM_ETMCCR_SEQPRES_MASK                      0x10000UL                                /**< Bit mask for ETM_SEQPRES */
#define _ETM_ETMCCR_SEQPRES_DEFAULT                   0x00000000UL                             /**< Mode DEFAULT for ETM_ETMCCR */
#define ETM_ETMCCR_SEQPRES_DEFAULT                    (_ETM_ETMCCR_SEQPRES_DEFAULT << 16)      /**< Shifted mode DEFAULT for ETM_ETMCCR */
#define _ETM_ETMCCR_EXTINPNUM_SHIFT                   17                                       /**< Shift value for ETM_EXTINPNUM */
#define _ETM_ETMCCR_EXTINPNUM_MASK                    0xE0000UL                                /**< Bit mask for ETM_EXTINPNUM */
#define _ETM_ETMCCR_EXTINPNUM_DEFAULT                 0x00000000UL                             /**< Mode DEFAULT for ETM_ETMCCR */
#define _ETM_ETMCCR_EXTINPNUM_ZERO                    0x00000000UL                             /**< Mode ZERO for ETM_ETMCCR */
#define _ETM_ETMCCR_EXTINPNUM_ONE                     0x00000001UL                             /**< Mode ONE for ETM_ETMCCR */
#define _ETM_ETMCCR_EXTINPNUM_TWO                     0x00000002UL                             /**< Mode TWO for ETM_ETMCCR */
#define ETM_ETMCCR_EXTINPNUM_DEFAULT                  (_ETM_ETMCCR_EXTINPNUM_DEFAULT << 17)    /**< Shifted mode DEFAULT for ETM_ETMCCR */
#define ETM_ETMCCR_EXTINPNUM_ZERO                     (_ETM_ETMCCR_EXTINPNUM_ZERO << 17)       /**< Shifted mode ZERO for ETM_ETMCCR */
#define ETM_ETMCCR_EXTINPNUM_ONE                      (_ETM_ETMCCR_EXTINPNUM_ONE << 17)        /**< Shifted mode ONE for ETM_ETMCCR */
#define ETM_ETMCCR_EXTINPNUM_TWO                      (_ETM_ETMCCR_EXTINPNUM_TWO << 17)        /**< Shifted mode TWO for ETM_ETMCCR */
#define _ETM_ETMCCR_EXTOUTNUM_SHIFT                   20                                       /**< Shift value for ETM_EXTOUTNUM */
#define _ETM_ETMCCR_EXTOUTNUM_MASK                    0x700000UL                               /**< Bit mask for ETM_EXTOUTNUM */
#define _ETM_ETMCCR_EXTOUTNUM_DEFAULT                 0x00000000UL                             /**< Mode DEFAULT for ETM_ETMCCR */
#define ETM_ETMCCR_EXTOUTNUM_DEFAULT                  (_ETM_ETMCCR_EXTOUTNUM_DEFAULT << 20)    /**< Shifted mode DEFAULT for ETM_ETMCCR */
#define ETM_ETMCCR_FIFOFULLPRES                       (0x1UL << 23)                            /**< FIFIO FULL present */
#define _ETM_ETMCCR_FIFOFULLPRES_SHIFT                23                                       /**< Shift value for ETM_FIFOFULLPRES */
#define _ETM_ETMCCR_FIFOFULLPRES_MASK                 0x800000UL                               /**< Bit mask for ETM_FIFOFULLPRES */
#define _ETM_ETMCCR_FIFOFULLPRES_DEFAULT              0x00000001UL                             /**< Mode DEFAULT for ETM_ETMCCR */
#define ETM_ETMCCR_FIFOFULLPRES_DEFAULT               (_ETM_ETMCCR_FIFOFULLPRES_DEFAULT << 23) /**< Shifted mode DEFAULT for ETM_ETMCCR */
#define _ETM_ETMCCR_IDCOMPNUM_SHIFT                   24                                       /**< Shift value for ETM_IDCOMPNUM */
#define _ETM_ETMCCR_IDCOMPNUM_MASK                    0x3000000UL                              /**< Bit mask for ETM_IDCOMPNUM */
#define _ETM_ETMCCR_IDCOMPNUM_DEFAULT                 0x00000000UL                             /**< Mode DEFAULT for ETM_ETMCCR */
#define ETM_ETMCCR_IDCOMPNUM_DEFAULT                  (_ETM_ETMCCR_IDCOMPNUM_DEFAULT << 24)    /**< Shifted mode DEFAULT for ETM_ETMCCR */
#define ETM_ETMCCR_TRACESS                            (0x1UL << 26)                            /**< Trace Start/Stop Block Present */
#define _ETM_ETMCCR_TRACESS_SHIFT                     26                                       /**< Shift value for ETM_TRACESS */
#define _ETM_ETMCCR_TRACESS_MASK                      0x4000000UL                              /**< Bit mask for ETM_TRACESS */
#define _ETM_ETMCCR_TRACESS_DEFAULT                   0x00000001UL                             /**< Mode DEFAULT for ETM_ETMCCR */
#define ETM_ETMCCR_TRACESS_DEFAULT                    (_ETM_ETMCCR_TRACESS_DEFAULT << 26)      /**< Shifted mode DEFAULT for ETM_ETMCCR */
#define ETM_ETMCCR_MMACCESS                           (0x1UL << 27)                            /**< Coprocessor and Memeory Access */
#define _ETM_ETMCCR_MMACCESS_SHIFT                    27                                       /**< Shift value for ETM_MMACCESS */
#define _ETM_ETMCCR_MMACCESS_MASK                     0x8000000UL                              /**< Bit mask for ETM_MMACCESS */
#define _ETM_ETMCCR_MMACCESS_DEFAULT                  0x00000001UL                             /**< Mode DEFAULT for ETM_ETMCCR */
#define ETM_ETMCCR_MMACCESS_DEFAULT                   (_ETM_ETMCCR_MMACCESS_DEFAULT << 27)     /**< Shifted mode DEFAULT for ETM_ETMCCR */
#define ETM_ETMCCR_ETMID                              (0x1UL << 31)                            /**< ETM ID Register Present */
#define _ETM_ETMCCR_ETMID_SHIFT                       31                                       /**< Shift value for ETM_ETMID */
#define _ETM_ETMCCR_ETMID_MASK                        0x80000000UL                             /**< Bit mask for ETM_ETMID */
#define _ETM_ETMCCR_ETMID_DEFAULT                     0x00000001UL                             /**< Mode DEFAULT for ETM_ETMCCR */
#define ETM_ETMCCR_ETMID_DEFAULT                      (_ETM_ETMCCR_ETMID_DEFAULT << 31)        /**< Shifted mode DEFAULT for ETM_ETMCCR */

/* Bit fields for ETM ETMTRIGGER */
#define _ETM_ETMTRIGGER_RESETVALUE                    0x00000000UL                           /**< Default value for ETM_ETMTRIGGER */
#define _ETM_ETMTRIGGER_MASK                          0x0001FFFFUL                           /**< Mask for ETM_ETMTRIGGER */
#define _ETM_ETMTRIGGER_RESA_SHIFT                    0                                      /**< Shift value for ETM_RESA */
#define _ETM_ETMTRIGGER_RESA_MASK                     0x7FUL                                 /**< Bit mask for ETM_RESA */
#define _ETM_ETMTRIGGER_RESA_DEFAULT                  0x00000000UL                           /**< Mode DEFAULT for ETM_ETMTRIGGER */
#define ETM_ETMTRIGGER_RESA_DEFAULT                   (_ETM_ETMTRIGGER_RESA_DEFAULT << 0)    /**< Shifted mode DEFAULT for ETM_ETMTRIGGER */
#define _ETM_ETMTRIGGER_RESB_SHIFT                    7                                      /**< Shift value for ETM_RESB */
#define _ETM_ETMTRIGGER_RESB_MASK                     0x3F80UL                               /**< Bit mask for ETM_RESB */
#define _ETM_ETMTRIGGER_RESB_DEFAULT                  0x00000000UL                           /**< Mode DEFAULT for ETM_ETMTRIGGER */
#define ETM_ETMTRIGGER_RESB_DEFAULT                   (_ETM_ETMTRIGGER_RESB_DEFAULT << 7)    /**< Shifted mode DEFAULT for ETM_ETMTRIGGER */
#define _ETM_ETMTRIGGER_ETMFCN_SHIFT                  14                                     /**< Shift value for ETM_ETMFCN */
#define _ETM_ETMTRIGGER_ETMFCN_MASK                   0x1C000UL                              /**< Bit mask for ETM_ETMFCN */
#define _ETM_ETMTRIGGER_ETMFCN_DEFAULT                0x00000000UL                           /**< Mode DEFAULT for ETM_ETMTRIGGER */
#define ETM_ETMTRIGGER_ETMFCN_DEFAULT                 (_ETM_ETMTRIGGER_ETMFCN_DEFAULT << 14) /**< Shifted mode DEFAULT for ETM_ETMTRIGGER */

/* Bit fields for ETM ETMSR */
#define _ETM_ETMSR_RESETVALUE                         0x00000002UL                         /**< Default value for ETM_ETMSR */
#define _ETM_ETMSR_MASK                               0x0000000FUL                         /**< Mask for ETM_ETMSR */
#define ETM_ETMSR_ETHOF                               (0x1UL << 0)                         /**< ETM Overflow */
#define _ETM_ETMSR_ETHOF_SHIFT                        0                                    /**< Shift value for ETM_ETHOF */
#define _ETM_ETMSR_ETHOF_MASK                         0x1UL                                /**< Bit mask for ETM_ETHOF */
#define _ETM_ETMSR_ETHOF_DEFAULT                      0x00000000UL                         /**< Mode DEFAULT for ETM_ETMSR */
#define ETM_ETMSR_ETHOF_DEFAULT                       (_ETM_ETMSR_ETHOF_DEFAULT << 0)      /**< Shifted mode DEFAULT for ETM_ETMSR */
#define ETM_ETMSR_ETMPROGBIT                          (0x1UL << 1)                         /**< ETM Programming Bit Status */
#define _ETM_ETMSR_ETMPROGBIT_SHIFT                   1                                    /**< Shift value for ETM_ETMPROGBIT */
#define _ETM_ETMSR_ETMPROGBIT_MASK                    0x2UL                                /**< Bit mask for ETM_ETMPROGBIT */
#define _ETM_ETMSR_ETMPROGBIT_DEFAULT                 0x00000001UL                         /**< Mode DEFAULT for ETM_ETMSR */
#define ETM_ETMSR_ETMPROGBIT_DEFAULT                  (_ETM_ETMSR_ETMPROGBIT_DEFAULT << 1) /**< Shifted mode DEFAULT for ETM_ETMSR */
#define ETM_ETMSR_TRACESTAT                           (0x1UL << 2)                         /**< Trace Start/Stop Status */
#define _ETM_ETMSR_TRACESTAT_SHIFT                    2                                    /**< Shift value for ETM_TRACESTAT */
#define _ETM_ETMSR_TRACESTAT_MASK                     0x4UL                                /**< Bit mask for ETM_TRACESTAT */
#define _ETM_ETMSR_TRACESTAT_DEFAULT                  0x00000000UL                         /**< Mode DEFAULT for ETM_ETMSR */
#define ETM_ETMSR_TRACESTAT_DEFAULT                   (_ETM_ETMSR_TRACESTAT_DEFAULT << 2)  /**< Shifted mode DEFAULT for ETM_ETMSR */
#define ETM_ETMSR_TRIGBIT                             (0x1UL << 3)                         /**< Trigger Bit */
#define _ETM_ETMSR_TRIGBIT_SHIFT                      3                                    /**< Shift value for ETM_TRIGBIT */
#define _ETM_ETMSR_TRIGBIT_MASK                       0x8UL                                /**< Bit mask for ETM_TRIGBIT */
#define _ETM_ETMSR_TRIGBIT_DEFAULT                    0x00000000UL                         /**< Mode DEFAULT for ETM_ETMSR */
#define ETM_ETMSR_TRIGBIT_DEFAULT                     (_ETM_ETMSR_TRIGBIT_DEFAULT << 3)    /**< Shifted mode DEFAULT for ETM_ETMSR */

/* Bit fields for ETM ETMSCR */
#define _ETM_ETMSCR_RESETVALUE                        0x00020D09UL                            /**< Default value for ETM_ETMSCR */
#define _ETM_ETMSCR_MASK                              0x00027F0FUL                            /**< Mask for ETM_ETMSCR */
#define _ETM_ETMSCR_MAXPORTSIZE_SHIFT                 0                                       /**< Shift value for ETM_MAXPORTSIZE */
#define _ETM_ETMSCR_MAXPORTSIZE_MASK                  0x7UL                                   /**< Bit mask for ETM_MAXPORTSIZE */
#define _ETM_ETMSCR_MAXPORTSIZE_DEFAULT               0x00000001UL                            /**< Mode DEFAULT for ETM_ETMSCR */
#define ETM_ETMSCR_MAXPORTSIZE_DEFAULT                (_ETM_ETMSCR_MAXPORTSIZE_DEFAULT << 0)  /**< Shifted mode DEFAULT for ETM_ETMSCR */
#define ETM_ETMSCR_Reserved                           (0x1UL << 3)                            /**< Reserved */
#define _ETM_ETMSCR_Reserved_SHIFT                    3                                       /**< Shift value for ETM_Reserved */
#define _ETM_ETMSCR_Reserved_MASK                     0x8UL                                   /**< Bit mask for ETM_Reserved */
#define _ETM_ETMSCR_Reserved_DEFAULT                  0x00000001UL                            /**< Mode DEFAULT for ETM_ETMSCR */
#define ETM_ETMSCR_Reserved_DEFAULT                   (_ETM_ETMSCR_Reserved_DEFAULT << 3)     /**< Shifted mode DEFAULT for ETM_ETMSCR */
#define ETM_ETMSCR_FIFOFULL                           (0x1UL << 8)                            /**< FIFO FULL Supported */
#define _ETM_ETMSCR_FIFOFULL_SHIFT                    8                                       /**< Shift value for ETM_FIFOFULL */
#define _ETM_ETMSCR_FIFOFULL_MASK                     0x100UL                                 /**< Bit mask for ETM_FIFOFULL */
#define _ETM_ETMSCR_FIFOFULL_DEFAULT                  0x00000001UL                            /**< Mode DEFAULT for ETM_ETMSCR */
#define ETM_ETMSCR_FIFOFULL_DEFAULT                   (_ETM_ETMSCR_FIFOFULL_DEFAULT << 8)     /**< Shifted mode DEFAULT for ETM_ETMSCR */
#define ETM_ETMSCR_MAXPORTSIZE3                       (0x1UL << 9)                            /**< Max Port Size[3] */
#define _ETM_ETMSCR_MAXPORTSIZE3_SHIFT                9                                       /**< Shift value for ETM_MAXPORTSIZE3 */
#define _ETM_ETMSCR_MAXPORTSIZE3_MASK                 0x200UL                                 /**< Bit mask for ETM_MAXPORTSIZE3 */
#define _ETM_ETMSCR_MAXPORTSIZE3_DEFAULT              0x00000000UL                            /**< Mode DEFAULT for ETM_ETMSCR */
#define ETM_ETMSCR_MAXPORTSIZE3_DEFAULT               (_ETM_ETMSCR_MAXPORTSIZE3_DEFAULT << 9) /**< Shifted mode DEFAULT for ETM_ETMSCR */
#define ETM_ETMSCR_PORTSIZE                           (0x1UL << 10)                           /**< Port Size Supported */
#define _ETM_ETMSCR_PORTSIZE_SHIFT                    10                                      /**< Shift value for ETM_PORTSIZE */
#define _ETM_ETMSCR_PORTSIZE_MASK                     0x400UL                                 /**< Bit mask for ETM_PORTSIZE */
#define _ETM_ETMSCR_PORTSIZE_DEFAULT                  0x00000001UL                            /**< Mode DEFAULT for ETM_ETMSCR */
#define ETM_ETMSCR_PORTSIZE_DEFAULT                   (_ETM_ETMSCR_PORTSIZE_DEFAULT << 10)    /**< Shifted mode DEFAULT for ETM_ETMSCR */
#define ETM_ETMSCR_PORTMODE                           (0x1UL << 11)                           /**< Port Mode Supported */
#define _ETM_ETMSCR_PORTMODE_SHIFT                    11                                      /**< Shift value for ETM_PORTMODE */
#define _ETM_ETMSCR_PORTMODE_MASK                     0x800UL                                 /**< Bit mask for ETM_PORTMODE */
#define _ETM_ETMSCR_PORTMODE_DEFAULT                  0x00000001UL                            /**< Mode DEFAULT for ETM_ETMSCR */
#define ETM_ETMSCR_PORTMODE_DEFAULT                   (_ETM_ETMSCR_PORTMODE_DEFAULT << 11)    /**< Shifted mode DEFAULT for ETM_ETMSCR */
#define _ETM_ETMSCR_PROCNUM_SHIFT                     12                                      /**< Shift value for ETM_PROCNUM */
#define _ETM_ETMSCR_PROCNUM_MASK                      0x7000UL                                /**< Bit mask for ETM_PROCNUM */
#define _ETM_ETMSCR_PROCNUM_DEFAULT                   0x00000000UL                            /**< Mode DEFAULT for ETM_ETMSCR */
#define ETM_ETMSCR_PROCNUM_DEFAULT                    (_ETM_ETMSCR_PROCNUM_DEFAULT << 12)     /**< Shifted mode DEFAULT for ETM_ETMSCR */
#define ETM_ETMSCR_NOFETCHCOMP                        (0x1UL << 17)                           /**< No Fetch Comparison */
#define _ETM_ETMSCR_NOFETCHCOMP_SHIFT                 17                                      /**< Shift value for ETM_NOFETCHCOMP */
#define _ETM_ETMSCR_NOFETCHCOMP_MASK                  0x20000UL                               /**< Bit mask for ETM_NOFETCHCOMP */
#define _ETM_ETMSCR_NOFETCHCOMP_DEFAULT               0x00000001UL                            /**< Mode DEFAULT for ETM_ETMSCR */
#define ETM_ETMSCR_NOFETCHCOMP_DEFAULT                (_ETM_ETMSCR_NOFETCHCOMP_DEFAULT << 17) /**< Shifted mode DEFAULT for ETM_ETMSCR */

/* Bit fields for ETM ETMTEEVR */
#define _ETM_ETMTEEVR_RESETVALUE                      0x00000000UL                           /**< Default value for ETM_ETMTEEVR */
#define _ETM_ETMTEEVR_MASK                            0x0001FFFFUL                           /**< Mask for ETM_ETMTEEVR */
#define _ETM_ETMTEEVR_RESA_SHIFT                      0                                      /**< Shift value for ETM_RESA */
#define _ETM_ETMTEEVR_RESA_MASK                       0x7FUL                                 /**< Bit mask for ETM_RESA */
#define _ETM_ETMTEEVR_RESA_DEFAULT                    0x00000000UL                           /**< Mode DEFAULT for ETM_ETMTEEVR */
#define ETM_ETMTEEVR_RESA_DEFAULT                     (_ETM_ETMTEEVR_RESA_DEFAULT << 0)      /**< Shifted mode DEFAULT for ETM_ETMTEEVR */
#define _ETM_ETMTEEVR_RESB_SHIFT                      7                                      /**< Shift value for ETM_RESB */
#define _ETM_ETMTEEVR_RESB_MASK                       0x3F80UL                               /**< Bit mask for ETM_RESB */
#define _ETM_ETMTEEVR_RESB_DEFAULT                    0x00000000UL                           /**< Mode DEFAULT for ETM_ETMTEEVR */
#define ETM_ETMTEEVR_RESB_DEFAULT                     (_ETM_ETMTEEVR_RESB_DEFAULT << 7)      /**< Shifted mode DEFAULT for ETM_ETMTEEVR */
#define _ETM_ETMTEEVR_ETMFCNEN_SHIFT                  14                                     /**< Shift value for ETM_ETMFCNEN */
#define _ETM_ETMTEEVR_ETMFCNEN_MASK                   0x1C000UL                              /**< Bit mask for ETM_ETMFCNEN */
#define _ETM_ETMTEEVR_ETMFCNEN_DEFAULT                0x00000000UL                           /**< Mode DEFAULT for ETM_ETMTEEVR */
#define ETM_ETMTEEVR_ETMFCNEN_DEFAULT                 (_ETM_ETMTEEVR_ETMFCNEN_DEFAULT << 14) /**< Shifted mode DEFAULT for ETM_ETMTEEVR */

/* Bit fields for ETM ETMTECR1 */
#define _ETM_ETMTECR1_RESETVALUE                      0x00000000UL                           /**< Default value for ETM_ETMTECR1 */
#define _ETM_ETMTECR1_MASK                            0x03FFFFFFUL                           /**< Mask for ETM_ETMTECR1 */
#define _ETM_ETMTECR1_ADRCMP_SHIFT                    0                                      /**< Shift value for ETM_ADRCMP */
#define _ETM_ETMTECR1_ADRCMP_MASK                     0xFFUL                                 /**< Bit mask for ETM_ADRCMP */
#define _ETM_ETMTECR1_ADRCMP_DEFAULT                  0x00000000UL                           /**< Mode DEFAULT for ETM_ETMTECR1 */
#define ETM_ETMTECR1_ADRCMP_DEFAULT                   (_ETM_ETMTECR1_ADRCMP_DEFAULT << 0)    /**< Shifted mode DEFAULT for ETM_ETMTECR1 */
#define _ETM_ETMTECR1_MEMMAP_SHIFT                    8                                      /**< Shift value for ETM_MEMMAP */
#define _ETM_ETMTECR1_MEMMAP_MASK                     0xFFFF00UL                             /**< Bit mask for ETM_MEMMAP */
#define _ETM_ETMTECR1_MEMMAP_DEFAULT                  0x00000000UL                           /**< Mode DEFAULT for ETM_ETMTECR1 */
#define ETM_ETMTECR1_MEMMAP_DEFAULT                   (_ETM_ETMTECR1_MEMMAP_DEFAULT << 8)    /**< Shifted mode DEFAULT for ETM_ETMTECR1 */
#define ETM_ETMTECR1_INCEXCTL                         (0x1UL << 24)                          /**< Trace Include/Exclude Flag */
#define _ETM_ETMTECR1_INCEXCTL_SHIFT                  24                                     /**< Shift value for ETM_INCEXCTL */
#define _ETM_ETMTECR1_INCEXCTL_MASK                   0x1000000UL                            /**< Bit mask for ETM_INCEXCTL */
#define _ETM_ETMTECR1_INCEXCTL_DEFAULT                0x00000000UL                           /**< Mode DEFAULT for ETM_ETMTECR1 */
#define _ETM_ETMTECR1_INCEXCTL_INC                    0x00000000UL                           /**< Mode INC for ETM_ETMTECR1 */
#define _ETM_ETMTECR1_INCEXCTL_EXC                    0x00000001UL                           /**< Mode EXC for ETM_ETMTECR1 */
#define ETM_ETMTECR1_INCEXCTL_DEFAULT                 (_ETM_ETMTECR1_INCEXCTL_DEFAULT << 24) /**< Shifted mode DEFAULT for ETM_ETMTECR1 */
#define ETM_ETMTECR1_INCEXCTL_INC                     (_ETM_ETMTECR1_INCEXCTL_INC << 24)     /**< Shifted mode INC for ETM_ETMTECR1 */
#define ETM_ETMTECR1_INCEXCTL_EXC                     (_ETM_ETMTECR1_INCEXCTL_EXC << 24)     /**< Shifted mode EXC for ETM_ETMTECR1 */
#define ETM_ETMTECR1_TCE                              (0x1UL << 25)                          /**< Trace Control Enable */
#define _ETM_ETMTECR1_TCE_SHIFT                       25                                     /**< Shift value for ETM_TCE */
#define _ETM_ETMTECR1_TCE_MASK                        0x2000000UL                            /**< Bit mask for ETM_TCE */
#define _ETM_ETMTECR1_TCE_DEFAULT                     0x00000000UL                           /**< Mode DEFAULT for ETM_ETMTECR1 */
#define _ETM_ETMTECR1_TCE_EN                          0x00000000UL                           /**< Mode EN for ETM_ETMTECR1 */
#define _ETM_ETMTECR1_TCE_DIS                         0x00000001UL                           /**< Mode DIS for ETM_ETMTECR1 */
#define ETM_ETMTECR1_TCE_DEFAULT                      (_ETM_ETMTECR1_TCE_DEFAULT << 25)      /**< Shifted mode DEFAULT for ETM_ETMTECR1 */
#define ETM_ETMTECR1_TCE_EN                           (_ETM_ETMTECR1_TCE_EN << 25)           /**< Shifted mode EN for ETM_ETMTECR1 */
#define ETM_ETMTECR1_TCE_DIS                          (_ETM_ETMTECR1_TCE_DIS << 25)          /**< Shifted mode DIS for ETM_ETMTECR1 */

/* Bit fields for ETM ETMFFLR */
#define _ETM_ETMFFLR_RESETVALUE                       0x00000000UL                        /**< Default value for ETM_ETMFFLR */
#define _ETM_ETMFFLR_MASK                             0x000000FFUL                        /**< Mask for ETM_ETMFFLR */
#define _ETM_ETMFFLR_BYTENUM_SHIFT                    0                                   /**< Shift value for ETM_BYTENUM */
#define _ETM_ETMFFLR_BYTENUM_MASK                     0xFFUL                              /**< Bit mask for ETM_BYTENUM */
#define _ETM_ETMFFLR_BYTENUM_DEFAULT                  0x00000000UL                        /**< Mode DEFAULT for ETM_ETMFFLR */
#define ETM_ETMFFLR_BYTENUM_DEFAULT                   (_ETM_ETMFFLR_BYTENUM_DEFAULT << 0) /**< Shifted mode DEFAULT for ETM_ETMFFLR */

/* Bit fields for ETM ETMCNTRLDVR1 */
#define _ETM_ETMCNTRLDVR1_RESETVALUE                  0x00000000UL                           /**< Default value for ETM_ETMCNTRLDVR1 */
#define _ETM_ETMCNTRLDVR1_MASK                        0x0000FFFFUL                           /**< Mask for ETM_ETMCNTRLDVR1 */
#define _ETM_ETMCNTRLDVR1_COUNT_SHIFT                 0                                      /**< Shift value for ETM_COUNT */
#define _ETM_ETMCNTRLDVR1_COUNT_MASK                  0xFFFFUL                               /**< Bit mask for ETM_COUNT */
#define _ETM_ETMCNTRLDVR1_COUNT_DEFAULT               0x00000000UL                           /**< Mode DEFAULT for ETM_ETMCNTRLDVR1 */
#define ETM_ETMCNTRLDVR1_COUNT_DEFAULT                (_ETM_ETMCNTRLDVR1_COUNT_DEFAULT << 0) /**< Shifted mode DEFAULT for ETM_ETMCNTRLDVR1 */

/* Bit fields for ETM ETMSYNCFR */
#define _ETM_ETMSYNCFR_RESETVALUE                     0x00000400UL                       /**< Default value for ETM_ETMSYNCFR */
#define _ETM_ETMSYNCFR_MASK                           0x00000FFFUL                       /**< Mask for ETM_ETMSYNCFR */
#define _ETM_ETMSYNCFR_FREQ_SHIFT                     0                                  /**< Shift value for ETM_FREQ */
#define _ETM_ETMSYNCFR_FREQ_MASK                      0xFFFUL                            /**< Bit mask for ETM_FREQ */
#define _ETM_ETMSYNCFR_FREQ_DEFAULT                   0x00000400UL                       /**< Mode DEFAULT for ETM_ETMSYNCFR */
#define ETM_ETMSYNCFR_FREQ_DEFAULT                    (_ETM_ETMSYNCFR_FREQ_DEFAULT << 0) /**< Shifted mode DEFAULT for ETM_ETMSYNCFR */

/* Bit fields for ETM ETMIDR */
#define _ETM_ETMIDR_RESETVALUE                        0x4114F253UL                         /**< Default value for ETM_ETMIDR */
#define _ETM_ETMIDR_MASK                              0xFF1DFFFFUL                         /**< Mask for ETM_ETMIDR */
#define _ETM_ETMIDR_IMPVER_SHIFT                      0                                    /**< Shift value for ETM_IMPVER */
#define _ETM_ETMIDR_IMPVER_MASK                       0xFUL                                /**< Bit mask for ETM_IMPVER */
#define _ETM_ETMIDR_IMPVER_DEFAULT                    0x00000003UL                         /**< Mode DEFAULT for ETM_ETMIDR */
#define ETM_ETMIDR_IMPVER_DEFAULT                     (_ETM_ETMIDR_IMPVER_DEFAULT << 0)    /**< Shifted mode DEFAULT for ETM_ETMIDR */
#define _ETM_ETMIDR_ETMMINVER_SHIFT                   4                                    /**< Shift value for ETM_ETMMINVER */
#define _ETM_ETMIDR_ETMMINVER_MASK                    0xF0UL                               /**< Bit mask for ETM_ETMMINVER */
#define _ETM_ETMIDR_ETMMINVER_DEFAULT                 0x00000005UL                         /**< Mode DEFAULT for ETM_ETMIDR */
#define ETM_ETMIDR_ETMMINVER_DEFAULT                  (_ETM_ETMIDR_ETMMINVER_DEFAULT << 4) /**< Shifted mode DEFAULT for ETM_ETMIDR */
#define _ETM_ETMIDR_ETMMAJVER_SHIFT                   8                                    /**< Shift value for ETM_ETMMAJVER */
#define _ETM_ETMIDR_ETMMAJVER_MASK                    0xF00UL                              /**< Bit mask for ETM_ETMMAJVER */
#define _ETM_ETMIDR_ETMMAJVER_DEFAULT                 0x00000002UL                         /**< Mode DEFAULT for ETM_ETMIDR */
#define ETM_ETMIDR_ETMMAJVER_DEFAULT                  (_ETM_ETMIDR_ETMMAJVER_DEFAULT << 8) /**< Shifted mode DEFAULT for ETM_ETMIDR */
#define _ETM_ETMIDR_PROCFAM_SHIFT                     12                                   /**< Shift value for ETM_PROCFAM */
#define _ETM_ETMIDR_PROCFAM_MASK                      0xF000UL                             /**< Bit mask for ETM_PROCFAM */
#define _ETM_ETMIDR_PROCFAM_DEFAULT                   0x0000000FUL                         /**< Mode DEFAULT for ETM_ETMIDR */
#define ETM_ETMIDR_PROCFAM_DEFAULT                    (_ETM_ETMIDR_PROCFAM_DEFAULT << 12)  /**< Shifted mode DEFAULT for ETM_ETMIDR */
#define ETM_ETMIDR_LPCF                               (0x1UL << 16)                        /**< Load PC First */
#define _ETM_ETMIDR_LPCF_SHIFT                        16                                   /**< Shift value for ETM_LPCF */
#define _ETM_ETMIDR_LPCF_MASK                         0x10000UL                            /**< Bit mask for ETM_LPCF */
#define _ETM_ETMIDR_LPCF_DEFAULT                      0x00000000UL                         /**< Mode DEFAULT for ETM_ETMIDR */
#define ETM_ETMIDR_LPCF_DEFAULT                       (_ETM_ETMIDR_LPCF_DEFAULT << 16)     /**< Shifted mode DEFAULT for ETM_ETMIDR */
#define ETM_ETMIDR_THUMBT                             (0x1UL << 18)                        /**< 32-bit Thumb Instruction Tracing */
#define _ETM_ETMIDR_THUMBT_SHIFT                      18                                   /**< Shift value for ETM_THUMBT */
#define _ETM_ETMIDR_THUMBT_MASK                       0x40000UL                            /**< Bit mask for ETM_THUMBT */
#define _ETM_ETMIDR_THUMBT_DEFAULT                    0x00000001UL                         /**< Mode DEFAULT for ETM_ETMIDR */
#define ETM_ETMIDR_THUMBT_DEFAULT                     (_ETM_ETMIDR_THUMBT_DEFAULT << 18)   /**< Shifted mode DEFAULT for ETM_ETMIDR */
#define ETM_ETMIDR_SECEXT                             (0x1UL << 19)                        /**< Security Extension Support */
#define _ETM_ETMIDR_SECEXT_SHIFT                      19                                   /**< Shift value for ETM_SECEXT */
#define _ETM_ETMIDR_SECEXT_MASK                       0x80000UL                            /**< Bit mask for ETM_SECEXT */
#define _ETM_ETMIDR_SECEXT_DEFAULT                    0x00000000UL                         /**< Mode DEFAULT for ETM_ETMIDR */
#define ETM_ETMIDR_SECEXT_DEFAULT                     (_ETM_ETMIDR_SECEXT_DEFAULT << 19)   /**< Shifted mode DEFAULT for ETM_ETMIDR */
#define ETM_ETMIDR_BPE                                (0x1UL << 20)                        /**< Branch Packet Encoding */
#define _ETM_ETMIDR_BPE_SHIFT                         20                                   /**< Shift value for ETM_BPE */
#define _ETM_ETMIDR_BPE_MASK                          0x100000UL                           /**< Bit mask for ETM_BPE */
#define _ETM_ETMIDR_BPE_DEFAULT                       0x00000001UL                         /**< Mode DEFAULT for ETM_ETMIDR */
#define ETM_ETMIDR_BPE_DEFAULT                        (_ETM_ETMIDR_BPE_DEFAULT << 20)      /**< Shifted mode DEFAULT for ETM_ETMIDR */
#define _ETM_ETMIDR_IMPCODE_SHIFT                     24                                   /**< Shift value for ETM_IMPCODE */
#define _ETM_ETMIDR_IMPCODE_MASK                      0xFF000000UL                         /**< Bit mask for ETM_IMPCODE */
#define _ETM_ETMIDR_IMPCODE_DEFAULT                   0x00000041UL                         /**< Mode DEFAULT for ETM_ETMIDR */
#define ETM_ETMIDR_IMPCODE_DEFAULT                    (_ETM_ETMIDR_IMPCODE_DEFAULT << 24)  /**< Shifted mode DEFAULT for ETM_ETMIDR */

/* Bit fields for ETM ETMCCER */
#define _ETM_ETMCCER_RESETVALUE                       0x18541800UL                           /**< Default value for ETM_ETMCCER */
#define _ETM_ETMCCER_MASK                             0x387FFFFBUL                           /**< Mask for ETM_ETMCCER */
#define _ETM_ETMCCER_EXTINPSEL_SHIFT                  0                                      /**< Shift value for ETM_EXTINPSEL */
#define _ETM_ETMCCER_EXTINPSEL_MASK                   0x3UL                                  /**< Bit mask for ETM_EXTINPSEL */
#define _ETM_ETMCCER_EXTINPSEL_DEFAULT                0x00000000UL                           /**< Mode DEFAULT for ETM_ETMCCER */
#define ETM_ETMCCER_EXTINPSEL_DEFAULT                 (_ETM_ETMCCER_EXTINPSEL_DEFAULT << 0)  /**< Shifted mode DEFAULT for ETM_ETMCCER */
#define _ETM_ETMCCER_EXTINPBUS_SHIFT                  3                                      /**< Shift value for ETM_EXTINPBUS */
#define _ETM_ETMCCER_EXTINPBUS_MASK                   0x7F8UL                                /**< Bit mask for ETM_EXTINPBUS */
#define _ETM_ETMCCER_EXTINPBUS_DEFAULT                0x00000000UL                           /**< Mode DEFAULT for ETM_ETMCCER */
#define ETM_ETMCCER_EXTINPBUS_DEFAULT                 (_ETM_ETMCCER_EXTINPBUS_DEFAULT << 3)  /**< Shifted mode DEFAULT for ETM_ETMCCER */
#define ETM_ETMCCER_READREGS                          (0x1UL << 11)                          /**< Readable Registers */
#define _ETM_ETMCCER_READREGS_SHIFT                   11                                     /**< Shift value for ETM_READREGS */
#define _ETM_ETMCCER_READREGS_MASK                    0x800UL                                /**< Bit mask for ETM_READREGS */
#define _ETM_ETMCCER_READREGS_DEFAULT                 0x00000001UL                           /**< Mode DEFAULT for ETM_ETMCCER */
#define ETM_ETMCCER_READREGS_DEFAULT                  (_ETM_ETMCCER_READREGS_DEFAULT << 11)  /**< Shifted mode DEFAULT for ETM_ETMCCER */
#define ETM_ETMCCER_DADDRCMP                          (0x1UL << 12)                          /**< Data Address comparisons */
#define _ETM_ETMCCER_DADDRCMP_SHIFT                   12                                     /**< Shift value for ETM_DADDRCMP */
#define _ETM_ETMCCER_DADDRCMP_MASK                    0x1000UL                               /**< Bit mask for ETM_DADDRCMP */
#define _ETM_ETMCCER_DADDRCMP_DEFAULT                 0x00000001UL                           /**< Mode DEFAULT for ETM_ETMCCER */
#define ETM_ETMCCER_DADDRCMP_DEFAULT                  (_ETM_ETMCCER_DADDRCMP_DEFAULT << 12)  /**< Shifted mode DEFAULT for ETM_ETMCCER */
#define _ETM_ETMCCER_INSTRES_SHIFT                    13                                     /**< Shift value for ETM_INSTRES */
#define _ETM_ETMCCER_INSTRES_MASK                     0xE000UL                               /**< Bit mask for ETM_INSTRES */
#define _ETM_ETMCCER_INSTRES_DEFAULT                  0x00000000UL                           /**< Mode DEFAULT for ETM_ETMCCER */
#define ETM_ETMCCER_INSTRES_DEFAULT                   (_ETM_ETMCCER_INSTRES_DEFAULT << 13)   /**< Shifted mode DEFAULT for ETM_ETMCCER */
#define _ETM_ETMCCER_EICEWPNT_SHIFT                   16                                     /**< Shift value for ETM_EICEWPNT */
#define _ETM_ETMCCER_EICEWPNT_MASK                    0xF0000UL                              /**< Bit mask for ETM_EICEWPNT */
#define _ETM_ETMCCER_EICEWPNT_DEFAULT                 0x00000004UL                           /**< Mode DEFAULT for ETM_ETMCCER */
#define ETM_ETMCCER_EICEWPNT_DEFAULT                  (_ETM_ETMCCER_EICEWPNT_DEFAULT << 16)  /**< Shifted mode DEFAULT for ETM_ETMCCER */
#define ETM_ETMCCER_TEICEWPNT                         (0x1UL << 20)                          /**< Trace Sart/Stop Block Uses EmbeddedICE watchpoint inputs */
#define _ETM_ETMCCER_TEICEWPNT_SHIFT                  20                                     /**< Shift value for ETM_TEICEWPNT */
#define _ETM_ETMCCER_TEICEWPNT_MASK                   0x100000UL                             /**< Bit mask for ETM_TEICEWPNT */
#define _ETM_ETMCCER_TEICEWPNT_DEFAULT                0x00000001UL                           /**< Mode DEFAULT for ETM_ETMCCER */
#define ETM_ETMCCER_TEICEWPNT_DEFAULT                 (_ETM_ETMCCER_TEICEWPNT_DEFAULT << 20) /**< Shifted mode DEFAULT for ETM_ETMCCER */
#define ETM_ETMCCER_EICEIMP                           (0x1UL << 21)                          /**< EmbeddedICE Behavior control Implemented */
#define _ETM_ETMCCER_EICEIMP_SHIFT                    21                                     /**< Shift value for ETM_EICEIMP */
#define _ETM_ETMCCER_EICEIMP_MASK                     0x200000UL                             /**< Bit mask for ETM_EICEIMP */
#define _ETM_ETMCCER_EICEIMP_DEFAULT                  0x00000000UL                           /**< Mode DEFAULT for ETM_ETMCCER */
#define ETM_ETMCCER_EICEIMP_DEFAULT                   (_ETM_ETMCCER_EICEIMP_DEFAULT << 21)   /**< Shifted mode DEFAULT for ETM_ETMCCER */
#define ETM_ETMCCER_TIMP                              (0x1UL << 22)                          /**< Timestamping Implemented */
#define _ETM_ETMCCER_TIMP_SHIFT                       22                                     /**< Shift value for ETM_TIMP */
#define _ETM_ETMCCER_TIMP_MASK                        0x400000UL                             /**< Bit mask for ETM_TIMP */
#define _ETM_ETMCCER_TIMP_DEFAULT                     0x00000001UL                           /**< Mode DEFAULT for ETM_ETMCCER */
#define ETM_ETMCCER_TIMP_DEFAULT                      (_ETM_ETMCCER_TIMP_DEFAULT << 22)      /**< Shifted mode DEFAULT for ETM_ETMCCER */
#define ETM_ETMCCER_RFCNT                             (0x1UL << 27)                          /**< Reduced Function Counter */
#define _ETM_ETMCCER_RFCNT_SHIFT                      27                                     /**< Shift value for ETM_RFCNT */
#define _ETM_ETMCCER_RFCNT_MASK                       0x8000000UL                            /**< Bit mask for ETM_RFCNT */
#define _ETM_ETMCCER_RFCNT_DEFAULT                    0x00000001UL                           /**< Mode DEFAULT for ETM_ETMCCER */
#define ETM_ETMCCER_RFCNT_DEFAULT                     (_ETM_ETMCCER_RFCNT_DEFAULT << 27)     /**< Shifted mode DEFAULT for ETM_ETMCCER */
#define ETM_ETMCCER_TENC                              (0x1UL << 28)                          /**< Timestamp Encoding */
#define _ETM_ETMCCER_TENC_SHIFT                       28                                     /**< Shift value for ETM_TENC */
#define _ETM_ETMCCER_TENC_MASK                        0x10000000UL                           /**< Bit mask for ETM_TENC */
#define _ETM_ETMCCER_TENC_DEFAULT                     0x00000001UL                           /**< Mode DEFAULT for ETM_ETMCCER */
#define ETM_ETMCCER_TENC_DEFAULT                      (_ETM_ETMCCER_TENC_DEFAULT << 28)      /**< Shifted mode DEFAULT for ETM_ETMCCER */
#define ETM_ETMCCER_TSIZE                             (0x1UL << 29)                          /**< Timestamp Size */
#define _ETM_ETMCCER_TSIZE_SHIFT                      29                                     /**< Shift value for ETM_TSIZE */
#define _ETM_ETMCCER_TSIZE_MASK                       0x20000000UL                           /**< Bit mask for ETM_TSIZE */
#define _ETM_ETMCCER_TSIZE_DEFAULT                    0x00000000UL                           /**< Mode DEFAULT for ETM_ETMCCER */
#define ETM_ETMCCER_TSIZE_DEFAULT                     (_ETM_ETMCCER_TSIZE_DEFAULT << 29)     /**< Shifted mode DEFAULT for ETM_ETMCCER */

/* Bit fields for ETM ETMTESSEICR */
#define _ETM_ETMTESSEICR_RESETVALUE                   0x00000000UL                              /**< Default value for ETM_ETMTESSEICR */
#define _ETM_ETMTESSEICR_MASK                         0x000F000FUL                              /**< Mask for ETM_ETMTESSEICR */
#define _ETM_ETMTESSEICR_STARTRSEL_SHIFT              0                                         /**< Shift value for ETM_STARTRSEL */
#define _ETM_ETMTESSEICR_STARTRSEL_MASK               0xFUL                                     /**< Bit mask for ETM_STARTRSEL */
#define _ETM_ETMTESSEICR_STARTRSEL_DEFAULT            0x00000000UL                              /**< Mode DEFAULT for ETM_ETMTESSEICR */
#define ETM_ETMTESSEICR_STARTRSEL_DEFAULT             (_ETM_ETMTESSEICR_STARTRSEL_DEFAULT << 0) /**< Shifted mode DEFAULT for ETM_ETMTESSEICR */
#define _ETM_ETMTESSEICR_STOPRSEL_SHIFT               16                                        /**< Shift value for ETM_STOPRSEL */
#define _ETM_ETMTESSEICR_STOPRSEL_MASK                0xF0000UL                                 /**< Bit mask for ETM_STOPRSEL */
#define _ETM_ETMTESSEICR_STOPRSEL_DEFAULT             0x00000000UL                              /**< Mode DEFAULT for ETM_ETMTESSEICR */
#define ETM_ETMTESSEICR_STOPRSEL_DEFAULT              (_ETM_ETMTESSEICR_STOPRSEL_DEFAULT << 16) /**< Shifted mode DEFAULT for ETM_ETMTESSEICR */

/* Bit fields for ETM ETMTSEVR */
#define _ETM_ETMTSEVR_RESETVALUE                      0x00000000UL                            /**< Default value for ETM_ETMTSEVR */
#define _ETM_ETMTSEVR_MASK                            0x0001FFFFUL                            /**< Mask for ETM_ETMTSEVR */
#define _ETM_ETMTSEVR_RESAEVT_SHIFT                   0                                       /**< Shift value for ETM_RESAEVT */
#define _ETM_ETMTSEVR_RESAEVT_MASK                    0x7FUL                                  /**< Bit mask for ETM_RESAEVT */
#define _ETM_ETMTSEVR_RESAEVT_DEFAULT                 0x00000000UL                            /**< Mode DEFAULT for ETM_ETMTSEVR */
#define ETM_ETMTSEVR_RESAEVT_DEFAULT                  (_ETM_ETMTSEVR_RESAEVT_DEFAULT << 0)    /**< Shifted mode DEFAULT for ETM_ETMTSEVR */
#define _ETM_ETMTSEVR_RESBEVT_SHIFT                   7                                       /**< Shift value for ETM_RESBEVT */
#define _ETM_ETMTSEVR_RESBEVT_MASK                    0x3F80UL                                /**< Bit mask for ETM_RESBEVT */
#define _ETM_ETMTSEVR_RESBEVT_DEFAULT                 0x00000000UL                            /**< Mode DEFAULT for ETM_ETMTSEVR */
#define ETM_ETMTSEVR_RESBEVT_DEFAULT                  (_ETM_ETMTSEVR_RESBEVT_DEFAULT << 7)    /**< Shifted mode DEFAULT for ETM_ETMTSEVR */
#define _ETM_ETMTSEVR_ETMFCNEVT_SHIFT                 14                                      /**< Shift value for ETM_ETMFCNEVT */
#define _ETM_ETMTSEVR_ETMFCNEVT_MASK                  0x1C000UL                               /**< Bit mask for ETM_ETMFCNEVT */
#define _ETM_ETMTSEVR_ETMFCNEVT_DEFAULT               0x00000000UL                            /**< Mode DEFAULT for ETM_ETMTSEVR */
#define ETM_ETMTSEVR_ETMFCNEVT_DEFAULT                (_ETM_ETMTSEVR_ETMFCNEVT_DEFAULT << 14) /**< Shifted mode DEFAULT for ETM_ETMTSEVR */

/* Bit fields for ETM ETMTRACEIDR */
#define _ETM_ETMTRACEIDR_RESETVALUE                   0x00000000UL                            /**< Default value for ETM_ETMTRACEIDR */
#define _ETM_ETMTRACEIDR_MASK                         0x0000007FUL                            /**< Mask for ETM_ETMTRACEIDR */
#define _ETM_ETMTRACEIDR_TRACEID_SHIFT                0                                       /**< Shift value for ETM_TRACEID */
#define _ETM_ETMTRACEIDR_TRACEID_MASK                 0x7FUL                                  /**< Bit mask for ETM_TRACEID */
#define _ETM_ETMTRACEIDR_TRACEID_DEFAULT              0x00000000UL                            /**< Mode DEFAULT for ETM_ETMTRACEIDR */
#define ETM_ETMTRACEIDR_TRACEID_DEFAULT               (_ETM_ETMTRACEIDR_TRACEID_DEFAULT << 0) /**< Shifted mode DEFAULT for ETM_ETMTRACEIDR */

/* Bit fields for ETM ETMIDR2 */
#define _ETM_ETMIDR2_RESETVALUE                       0x00000000UL                    /**< Default value for ETM_ETMIDR2 */
#define _ETM_ETMIDR2_MASK                             0x00000003UL                    /**< Mask for ETM_ETMIDR2 */
#define ETM_ETMIDR2_RFE                               (0x1UL << 0)                    /**< RFE Transfer Order */
#define _ETM_ETMIDR2_RFE_SHIFT                        0                               /**< Shift value for ETM_RFE */
#define _ETM_ETMIDR2_RFE_MASK                         0x1UL                           /**< Bit mask for ETM_RFE */
#define _ETM_ETMIDR2_RFE_DEFAULT                      0x00000000UL                    /**< Mode DEFAULT for ETM_ETMIDR2 */
#define _ETM_ETMIDR2_RFE_PC                           0x00000000UL                    /**< Mode PC for ETM_ETMIDR2 */
#define _ETM_ETMIDR2_RFE_CPSR                         0x00000001UL                    /**< Mode CPSR for ETM_ETMIDR2 */
#define ETM_ETMIDR2_RFE_DEFAULT                       (_ETM_ETMIDR2_RFE_DEFAULT << 0) /**< Shifted mode DEFAULT for ETM_ETMIDR2 */
#define ETM_ETMIDR2_RFE_PC                            (_ETM_ETMIDR2_RFE_PC << 0)      /**< Shifted mode PC for ETM_ETMIDR2 */
#define ETM_ETMIDR2_RFE_CPSR                          (_ETM_ETMIDR2_RFE_CPSR << 0)    /**< Shifted mode CPSR for ETM_ETMIDR2 */
#define ETM_ETMIDR2_SWP                               (0x1UL << 1)                    /**< SWP Transfer Order */
#define _ETM_ETMIDR2_SWP_SHIFT                        1                               /**< Shift value for ETM_SWP */
#define _ETM_ETMIDR2_SWP_MASK                         0x2UL                           /**< Bit mask for ETM_SWP */
#define _ETM_ETMIDR2_SWP_DEFAULT                      0x00000000UL                    /**< Mode DEFAULT for ETM_ETMIDR2 */
#define _ETM_ETMIDR2_SWP_LOAD                         0x00000000UL                    /**< Mode LOAD for ETM_ETMIDR2 */
#define _ETM_ETMIDR2_SWP_STORE                        0x00000001UL                    /**< Mode STORE for ETM_ETMIDR2 */
#define ETM_ETMIDR2_SWP_DEFAULT                       (_ETM_ETMIDR2_SWP_DEFAULT << 1) /**< Shifted mode DEFAULT for ETM_ETMIDR2 */
#define ETM_ETMIDR2_SWP_LOAD                          (_ETM_ETMIDR2_SWP_LOAD << 1)    /**< Shifted mode LOAD for ETM_ETMIDR2 */
#define ETM_ETMIDR2_SWP_STORE                         (_ETM_ETMIDR2_SWP_STORE << 1)   /**< Shifted mode STORE for ETM_ETMIDR2 */

/* Bit fields for ETM ETMPDSR */
#define _ETM_ETMPDSR_RESETVALUE                       0x00000001UL                      /**< Default value for ETM_ETMPDSR */
#define _ETM_ETMPDSR_MASK                             0x00000001UL                      /**< Mask for ETM_ETMPDSR */
#define ETM_ETMPDSR_ETMUP                             (0x1UL << 0)                      /**< ETM Powered Up */
#define _ETM_ETMPDSR_ETMUP_SHIFT                      0                                 /**< Shift value for ETM_ETMUP */
#define _ETM_ETMPDSR_ETMUP_MASK                       0x1UL                             /**< Bit mask for ETM_ETMUP */
#define _ETM_ETMPDSR_ETMUP_DEFAULT                    0x00000001UL                      /**< Mode DEFAULT for ETM_ETMPDSR */
#define ETM_ETMPDSR_ETMUP_DEFAULT                     (_ETM_ETMPDSR_ETMUP_DEFAULT << 0) /**< Shifted mode DEFAULT for ETM_ETMPDSR */

/* Bit fields for ETM ETMISCIN */
#define _ETM_ETMISCIN_RESETVALUE                      0x00000000UL                          /**< Default value for ETM_ETMISCIN */
#define _ETM_ETMISCIN_MASK                            0x00000013UL                          /**< Mask for ETM_ETMISCIN */
#define _ETM_ETMISCIN_EXTIN_SHIFT                     0                                     /**< Shift value for ETM_EXTIN */
#define _ETM_ETMISCIN_EXTIN_MASK                      0x3UL                                 /**< Bit mask for ETM_EXTIN */
#define _ETM_ETMISCIN_EXTIN_DEFAULT                   0x00000000UL                          /**< Mode DEFAULT for ETM_ETMISCIN */
#define ETM_ETMISCIN_EXTIN_DEFAULT                    (_ETM_ETMISCIN_EXTIN_DEFAULT << 0)    /**< Shifted mode DEFAULT for ETM_ETMISCIN */
#define ETM_ETMISCIN_COREHALT                         (0x1UL << 4)                          /**< Core Halt */
#define _ETM_ETMISCIN_COREHALT_SHIFT                  4                                     /**< Shift value for ETM_COREHALT */
#define _ETM_ETMISCIN_COREHALT_MASK                   0x10UL                                /**< Bit mask for ETM_COREHALT */
#define _ETM_ETMISCIN_COREHALT_DEFAULT                0x00000000UL                          /**< Mode DEFAULT for ETM_ETMISCIN */
#define ETM_ETMISCIN_COREHALT_DEFAULT                 (_ETM_ETMISCIN_COREHALT_DEFAULT << 4) /**< Shifted mode DEFAULT for ETM_ETMISCIN */

/* Bit fields for ETM ITTRIGOUT */
#define _ETM_ITTRIGOUT_RESETVALUE                     0x00000000UL                             /**< Default value for ETM_ITTRIGOUT */
#define _ETM_ITTRIGOUT_MASK                           0x00000001UL                             /**< Mask for ETM_ITTRIGOUT */
#define ETM_ITTRIGOUT_TRIGGEROUT                      (0x1UL << 0)                             /**< Trigger output value */
#define _ETM_ITTRIGOUT_TRIGGEROUT_SHIFT               0                                        /**< Shift value for ETM_TRIGGEROUT */
#define _ETM_ITTRIGOUT_TRIGGEROUT_MASK                0x1UL                                    /**< Bit mask for ETM_TRIGGEROUT */
#define _ETM_ITTRIGOUT_TRIGGEROUT_DEFAULT             0x00000000UL                             /**< Mode DEFAULT for ETM_ITTRIGOUT */
#define ETM_ITTRIGOUT_TRIGGEROUT_DEFAULT              (_ETM_ITTRIGOUT_TRIGGEROUT_DEFAULT << 0) /**< Shifted mode DEFAULT for ETM_ITTRIGOUT */

/* Bit fields for ETM ETMITATBCTR2 */
#define _ETM_ETMITATBCTR2_RESETVALUE                  0x00000001UL                             /**< Default value for ETM_ETMITATBCTR2 */
#define _ETM_ETMITATBCTR2_MASK                        0x00000001UL                             /**< Mask for ETM_ETMITATBCTR2 */
#define ETM_ETMITATBCTR2_ATREADY                      (0x1UL << 0)                             /**< ATREADY Input Value */
#define _ETM_ETMITATBCTR2_ATREADY_SHIFT               0                                        /**< Shift value for ETM_ATREADY */
#define _ETM_ETMITATBCTR2_ATREADY_MASK                0x1UL                                    /**< Bit mask for ETM_ATREADY */
#define _ETM_ETMITATBCTR2_ATREADY_DEFAULT             0x00000001UL                             /**< Mode DEFAULT for ETM_ETMITATBCTR2 */
#define ETM_ETMITATBCTR2_ATREADY_DEFAULT              (_ETM_ETMITATBCTR2_ATREADY_DEFAULT << 0) /**< Shifted mode DEFAULT for ETM_ETMITATBCTR2 */

/* Bit fields for ETM ETMITATBCTR0 */
#define _ETM_ETMITATBCTR0_RESETVALUE                  0x00000000UL                             /**< Default value for ETM_ETMITATBCTR0 */
#define _ETM_ETMITATBCTR0_MASK                        0x00000001UL                             /**< Mask for ETM_ETMITATBCTR0 */
#define ETM_ETMITATBCTR0_ATVALID                      (0x1UL << 0)                             /**< ATVALID Output Value */
#define _ETM_ETMITATBCTR0_ATVALID_SHIFT               0                                        /**< Shift value for ETM_ATVALID */
#define _ETM_ETMITATBCTR0_ATVALID_MASK                0x1UL                                    /**< Bit mask for ETM_ATVALID */
#define _ETM_ETMITATBCTR0_ATVALID_DEFAULT             0x00000000UL                             /**< Mode DEFAULT for ETM_ETMITATBCTR0 */
#define ETM_ETMITATBCTR0_ATVALID_DEFAULT              (_ETM_ETMITATBCTR0_ATVALID_DEFAULT << 0) /**< Shifted mode DEFAULT for ETM_ETMITATBCTR0 */

/* Bit fields for ETM ETMITCTRL */
#define _ETM_ETMITCTRL_RESETVALUE                     0x00000000UL                       /**< Default value for ETM_ETMITCTRL */
#define _ETM_ETMITCTRL_MASK                           0x00000001UL                       /**< Mask for ETM_ETMITCTRL */
#define ETM_ETMITCTRL_ITEN                            (0x1UL << 0)                       /**< Integration Mode Enable */
#define _ETM_ETMITCTRL_ITEN_SHIFT                     0                                  /**< Shift value for ETM_ITEN */
#define _ETM_ETMITCTRL_ITEN_MASK                      0x1UL                              /**< Bit mask for ETM_ITEN */
#define _ETM_ETMITCTRL_ITEN_DEFAULT                   0x00000000UL                       /**< Mode DEFAULT for ETM_ETMITCTRL */
#define ETM_ETMITCTRL_ITEN_DEFAULT                    (_ETM_ETMITCTRL_ITEN_DEFAULT << 0) /**< Shifted mode DEFAULT for ETM_ETMITCTRL */

/* Bit fields for ETM ETMCLAIMSET */
#define _ETM_ETMCLAIMSET_RESETVALUE                   0x0000000FUL                           /**< Default value for ETM_ETMCLAIMSET */
#define _ETM_ETMCLAIMSET_MASK                         0x000000FFUL                           /**< Mask for ETM_ETMCLAIMSET */
#define _ETM_ETMCLAIMSET_SETTAG_SHIFT                 0                                      /**< Shift value for ETM_SETTAG */
#define _ETM_ETMCLAIMSET_SETTAG_MASK                  0xFFUL                                 /**< Bit mask for ETM_SETTAG */
#define _ETM_ETMCLAIMSET_SETTAG_DEFAULT               0x0000000FUL                           /**< Mode DEFAULT for ETM_ETMCLAIMSET */
#define ETM_ETMCLAIMSET_SETTAG_DEFAULT                (_ETM_ETMCLAIMSET_SETTAG_DEFAULT << 0) /**< Shifted mode DEFAULT for ETM_ETMCLAIMSET */

/* Bit fields for ETM ETMCLAIMCLR */
#define _ETM_ETMCLAIMCLR_RESETVALUE                   0x00000000UL                           /**< Default value for ETM_ETMCLAIMCLR */
#define _ETM_ETMCLAIMCLR_MASK                         0x00000001UL                           /**< Mask for ETM_ETMCLAIMCLR */
#define ETM_ETMCLAIMCLR_CLRTAG                        (0x1UL << 0)                           /**< Tag Bits */
#define _ETM_ETMCLAIMCLR_CLRTAG_SHIFT                 0                                      /**< Shift value for ETM_CLRTAG */
#define _ETM_ETMCLAIMCLR_CLRTAG_MASK                  0x1UL                                  /**< Bit mask for ETM_CLRTAG */
#define _ETM_ETMCLAIMCLR_CLRTAG_DEFAULT               0x00000000UL                           /**< Mode DEFAULT for ETM_ETMCLAIMCLR */
#define ETM_ETMCLAIMCLR_CLRTAG_DEFAULT                (_ETM_ETMCLAIMCLR_CLRTAG_DEFAULT << 0) /**< Shifted mode DEFAULT for ETM_ETMCLAIMCLR */

/* Bit fields for ETM ETMLAR */
#define _ETM_ETMLAR_RESETVALUE                        0x00000000UL                   /**< Default value for ETM_ETMLAR */
#define _ETM_ETMLAR_MASK                              0x00000001UL                   /**< Mask for ETM_ETMLAR */
#define ETM_ETMLAR_KEY                                (0x1UL << 0)                   /**< Key Value */
#define _ETM_ETMLAR_KEY_SHIFT                         0                              /**< Shift value for ETM_KEY */
#define _ETM_ETMLAR_KEY_MASK                          0x1UL                          /**< Bit mask for ETM_KEY */
#define _ETM_ETMLAR_KEY_DEFAULT                       0x00000000UL                   /**< Mode DEFAULT for ETM_ETMLAR */
#define ETM_ETMLAR_KEY_DEFAULT                        (_ETM_ETMLAR_KEY_DEFAULT << 0) /**< Shifted mode DEFAULT for ETM_ETMLAR */

/* Bit fields for ETM ETMLSR */
#define _ETM_ETMLSR_RESETVALUE                        0x00000003UL                       /**< Default value for ETM_ETMLSR */
#define _ETM_ETMLSR_MASK                              0x00000003UL                       /**< Mask for ETM_ETMLSR */
#define ETM_ETMLSR_LOCKIMP                            (0x1UL << 0)                       /**< ETM Locking Implemented */
#define _ETM_ETMLSR_LOCKIMP_SHIFT                     0                                  /**< Shift value for ETM_LOCKIMP */
#define _ETM_ETMLSR_LOCKIMP_MASK                      0x1UL                              /**< Bit mask for ETM_LOCKIMP */
#define _ETM_ETMLSR_LOCKIMP_DEFAULT                   0x00000001UL                       /**< Mode DEFAULT for ETM_ETMLSR */
#define ETM_ETMLSR_LOCKIMP_DEFAULT                    (_ETM_ETMLSR_LOCKIMP_DEFAULT << 0) /**< Shifted mode DEFAULT for ETM_ETMLSR */
#define ETM_ETMLSR_LOCKED                             (0x1UL << 1)                       /**< ETM locked */
#define _ETM_ETMLSR_LOCKED_SHIFT                      1                                  /**< Shift value for ETM_LOCKED */
#define _ETM_ETMLSR_LOCKED_MASK                       0x2UL                              /**< Bit mask for ETM_LOCKED */
#define _ETM_ETMLSR_LOCKED_DEFAULT                    0x00000001UL                       /**< Mode DEFAULT for ETM_ETMLSR */
#define ETM_ETMLSR_LOCKED_DEFAULT                     (_ETM_ETMLSR_LOCKED_DEFAULT << 1)  /**< Shifted mode DEFAULT for ETM_ETMLSR */

/* Bit fields for ETM ETMAUTHSTATUS */
#define _ETM_ETMAUTHSTATUS_RESETVALUE                 0x000000C0UL                                      /**< Default value for ETM_ETMAUTHSTATUS */
#define _ETM_ETMAUTHSTATUS_MASK                       0x000000FFUL                                      /**< Mask for ETM_ETMAUTHSTATUS */
#define _ETM_ETMAUTHSTATUS_NONSECINVDBG_SHIFT         0                                                 /**< Shift value for ETM_NONSECINVDBG */
#define _ETM_ETMAUTHSTATUS_NONSECINVDBG_MASK          0x3UL                                             /**< Bit mask for ETM_NONSECINVDBG */
#define _ETM_ETMAUTHSTATUS_NONSECINVDBG_DEFAULT       0x00000000UL                                      /**< Mode DEFAULT for ETM_ETMAUTHSTATUS */
#define ETM_ETMAUTHSTATUS_NONSECINVDBG_DEFAULT        (_ETM_ETMAUTHSTATUS_NONSECINVDBG_DEFAULT << 0)    /**< Shifted mode DEFAULT for ETM_ETMAUTHSTATUS */
#define _ETM_ETMAUTHSTATUS_NONSECNONINVDBG_SHIFT      2                                                 /**< Shift value for ETM_NONSECNONINVDBG */
#define _ETM_ETMAUTHSTATUS_NONSECNONINVDBG_MASK       0xCUL                                             /**< Bit mask for ETM_NONSECNONINVDBG */
#define _ETM_ETMAUTHSTATUS_NONSECNONINVDBG_DEFAULT    0x00000000UL                                      /**< Mode DEFAULT for ETM_ETMAUTHSTATUS */
#define _ETM_ETMAUTHSTATUS_NONSECNONINVDBG_DISABLE    0x00000002UL                                      /**< Mode DISABLE for ETM_ETMAUTHSTATUS */
#define _ETM_ETMAUTHSTATUS_NONSECNONINVDBG_ENABLE     0x00000003UL                                      /**< Mode ENABLE for ETM_ETMAUTHSTATUS */
#define ETM_ETMAUTHSTATUS_NONSECNONINVDBG_DEFAULT     (_ETM_ETMAUTHSTATUS_NONSECNONINVDBG_DEFAULT << 2) /**< Shifted mode DEFAULT for ETM_ETMAUTHSTATUS */
#define ETM_ETMAUTHSTATUS_NONSECNONINVDBG_DISABLE     (_ETM_ETMAUTHSTATUS_NONSECNONINVDBG_DISABLE << 2) /**< Shifted mode DISABLE for ETM_ETMAUTHSTATUS */
#define ETM_ETMAUTHSTATUS_NONSECNONINVDBG_ENABLE      (_ETM_ETMAUTHSTATUS_NONSECNONINVDBG_ENABLE << 2)  /**< Shifted mode ENABLE for ETM_ETMAUTHSTATUS */
#define _ETM_ETMAUTHSTATUS_SECINVDBG_SHIFT            4                                                 /**< Shift value for ETM_SECINVDBG */
#define _ETM_ETMAUTHSTATUS_SECINVDBG_MASK             0x30UL                                            /**< Bit mask for ETM_SECINVDBG */
#define _ETM_ETMAUTHSTATUS_SECINVDBG_DEFAULT          0x00000000UL                                      /**< Mode DEFAULT for ETM_ETMAUTHSTATUS */
#define ETM_ETMAUTHSTATUS_SECINVDBG_DEFAULT           (_ETM_ETMAUTHSTATUS_SECINVDBG_DEFAULT << 4)       /**< Shifted mode DEFAULT for ETM_ETMAUTHSTATUS */
#define _ETM_ETMAUTHSTATUS_SECNONINVDBG_SHIFT         6                                                 /**< Shift value for ETM_SECNONINVDBG */
#define _ETM_ETMAUTHSTATUS_SECNONINVDBG_MASK          0xC0UL                                            /**< Bit mask for ETM_SECNONINVDBG */
#define _ETM_ETMAUTHSTATUS_SECNONINVDBG_DEFAULT       0x00000003UL                                      /**< Mode DEFAULT for ETM_ETMAUTHSTATUS */
#define ETM_ETMAUTHSTATUS_SECNONINVDBG_DEFAULT        (_ETM_ETMAUTHSTATUS_SECNONINVDBG_DEFAULT << 6)    /**< Shifted mode DEFAULT for ETM_ETMAUTHSTATUS */

/* Bit fields for ETM ETMDEVTYPE */
#define _ETM_ETMDEVTYPE_RESETVALUE                    0x00000013UL                             /**< Default value for ETM_ETMDEVTYPE */
#define _ETM_ETMDEVTYPE_MASK                          0x000000FFUL                             /**< Mask for ETM_ETMDEVTYPE */
#define _ETM_ETMDEVTYPE_TRACESRC_SHIFT                0                                        /**< Shift value for ETM_TRACESRC */
#define _ETM_ETMDEVTYPE_TRACESRC_MASK                 0xFUL                                    /**< Bit mask for ETM_TRACESRC */
#define _ETM_ETMDEVTYPE_TRACESRC_DEFAULT              0x00000003UL                             /**< Mode DEFAULT for ETM_ETMDEVTYPE */
#define ETM_ETMDEVTYPE_TRACESRC_DEFAULT               (_ETM_ETMDEVTYPE_TRACESRC_DEFAULT << 0)  /**< Shifted mode DEFAULT for ETM_ETMDEVTYPE */
#define _ETM_ETMDEVTYPE_PROCTRACE_SHIFT               4                                        /**< Shift value for ETM_PROCTRACE */
#define _ETM_ETMDEVTYPE_PROCTRACE_MASK                0xF0UL                                   /**< Bit mask for ETM_PROCTRACE */
#define _ETM_ETMDEVTYPE_PROCTRACE_DEFAULT             0x00000001UL                             /**< Mode DEFAULT for ETM_ETMDEVTYPE */
#define ETM_ETMDEVTYPE_PROCTRACE_DEFAULT              (_ETM_ETMDEVTYPE_PROCTRACE_DEFAULT << 4) /**< Shifted mode DEFAULT for ETM_ETMDEVTYPE */

/* Bit fields for ETM ETMPIDR4 */
#define _ETM_ETMPIDR4_RESETVALUE                      0x00000004UL                          /**< Default value for ETM_ETMPIDR4 */
#define _ETM_ETMPIDR4_MASK                            0x000000FFUL                          /**< Mask for ETM_ETMPIDR4 */
#define _ETM_ETMPIDR4_CONTCODE_SHIFT                  0                                     /**< Shift value for ETM_CONTCODE */
#define _ETM_ETMPIDR4_CONTCODE_MASK                   0xFUL                                 /**< Bit mask for ETM_CONTCODE */
#define _ETM_ETMPIDR4_CONTCODE_DEFAULT                0x00000004UL                          /**< Mode DEFAULT for ETM_ETMPIDR4 */
#define ETM_ETMPIDR4_CONTCODE_DEFAULT                 (_ETM_ETMPIDR4_CONTCODE_DEFAULT << 0) /**< Shifted mode DEFAULT for ETM_ETMPIDR4 */
#define _ETM_ETMPIDR4_COUNT_SHIFT                     4                                     /**< Shift value for ETM_COUNT */
#define _ETM_ETMPIDR4_COUNT_MASK                      0xF0UL                                /**< Bit mask for ETM_COUNT */
#define _ETM_ETMPIDR4_COUNT_DEFAULT                   0x00000000UL                          /**< Mode DEFAULT for ETM_ETMPIDR4 */
#define ETM_ETMPIDR4_COUNT_DEFAULT                    (_ETM_ETMPIDR4_COUNT_DEFAULT << 4)    /**< Shifted mode DEFAULT for ETM_ETMPIDR4 */

/* Bit fields for ETM ETMPIDR5 */
#define _ETM_ETMPIDR5_RESETVALUE                      0x00000000UL /**< Default value for ETM_ETMPIDR5 */
#define _ETM_ETMPIDR5_MASK                            0x00000000UL /**< Mask for ETM_ETMPIDR5 */

/* Bit fields for ETM ETMPIDR6 */
#define _ETM_ETMPIDR6_RESETVALUE                      0x00000000UL /**< Default value for ETM_ETMPIDR6 */
#define _ETM_ETMPIDR6_MASK                            0x00000000UL /**< Mask for ETM_ETMPIDR6 */

/* Bit fields for ETM ETMPIDR7 */
#define _ETM_ETMPIDR7_RESETVALUE                      0x00000000UL /**< Default value for ETM_ETMPIDR7 */
#define _ETM_ETMPIDR7_MASK                            0x00000000UL /**< Mask for ETM_ETMPIDR7 */

/* Bit fields for ETM ETMPIDR0 */
#define _ETM_ETMPIDR0_RESETVALUE                      0x00000024UL                         /**< Default value for ETM_ETMPIDR0 */
#define _ETM_ETMPIDR0_MASK                            0x000000FFUL                         /**< Mask for ETM_ETMPIDR0 */
#define _ETM_ETMPIDR0_PARTNUM_SHIFT                   0                                    /**< Shift value for ETM_PARTNUM */
#define _ETM_ETMPIDR0_PARTNUM_MASK                    0xFFUL                               /**< Bit mask for ETM_PARTNUM */
#define _ETM_ETMPIDR0_PARTNUM_DEFAULT                 0x00000024UL                         /**< Mode DEFAULT for ETM_ETMPIDR0 */
#define ETM_ETMPIDR0_PARTNUM_DEFAULT                  (_ETM_ETMPIDR0_PARTNUM_DEFAULT << 0) /**< Shifted mode DEFAULT for ETM_ETMPIDR0 */

/* Bit fields for ETM ETMPIDR1 */
#define _ETM_ETMPIDR1_RESETVALUE                      0x000000B9UL                         /**< Default value for ETM_ETMPIDR1 */
#define _ETM_ETMPIDR1_MASK                            0x000000FFUL                         /**< Mask for ETM_ETMPIDR1 */
#define _ETM_ETMPIDR1_PARTNUM_SHIFT                   0                                    /**< Shift value for ETM_PARTNUM */
#define _ETM_ETMPIDR1_PARTNUM_MASK                    0xFUL                                /**< Bit mask for ETM_PARTNUM */
#define _ETM_ETMPIDR1_PARTNUM_DEFAULT                 0x00000009UL                         /**< Mode DEFAULT for ETM_ETMPIDR1 */
#define ETM_ETMPIDR1_PARTNUM_DEFAULT                  (_ETM_ETMPIDR1_PARTNUM_DEFAULT << 0) /**< Shifted mode DEFAULT for ETM_ETMPIDR1 */
#define _ETM_ETMPIDR1_IDCODE_SHIFT                    4                                    /**< Shift value for ETM_IDCODE */
#define _ETM_ETMPIDR1_IDCODE_MASK                     0xF0UL                               /**< Bit mask for ETM_IDCODE */
#define _ETM_ETMPIDR1_IDCODE_DEFAULT                  0x0000000BUL                         /**< Mode DEFAULT for ETM_ETMPIDR1 */
#define ETM_ETMPIDR1_IDCODE_DEFAULT                   (_ETM_ETMPIDR1_IDCODE_DEFAULT << 4)  /**< Shifted mode DEFAULT for ETM_ETMPIDR1 */

/* Bit fields for ETM ETMPIDR2 */
#define _ETM_ETMPIDR2_RESETVALUE                      0x0000003BUL                         /**< Default value for ETM_ETMPIDR2 */
#define _ETM_ETMPIDR2_MASK                            0x000000FFUL                         /**< Mask for ETM_ETMPIDR2 */
#define _ETM_ETMPIDR2_IDCODE_SHIFT                    0                                    /**< Shift value for ETM_IDCODE */
#define _ETM_ETMPIDR2_IDCODE_MASK                     0x7UL                                /**< Bit mask for ETM_IDCODE */
#define _ETM_ETMPIDR2_IDCODE_DEFAULT                  0x00000003UL                         /**< Mode DEFAULT for ETM_ETMPIDR2 */
#define ETM_ETMPIDR2_IDCODE_DEFAULT                   (_ETM_ETMPIDR2_IDCODE_DEFAULT << 0)  /**< Shifted mode DEFAULT for ETM_ETMPIDR2 */
#define ETM_ETMPIDR2_ALWAYS1                          (0x1UL << 3)                         /**< Always 1 */
#define _ETM_ETMPIDR2_ALWAYS1_SHIFT                   3                                    /**< Shift value for ETM_ALWAYS1 */
#define _ETM_ETMPIDR2_ALWAYS1_MASK                    0x8UL                                /**< Bit mask for ETM_ALWAYS1 */
#define _ETM_ETMPIDR2_ALWAYS1_DEFAULT                 0x00000001UL                         /**< Mode DEFAULT for ETM_ETMPIDR2 */
#define ETM_ETMPIDR2_ALWAYS1_DEFAULT                  (_ETM_ETMPIDR2_ALWAYS1_DEFAULT << 3) /**< Shifted mode DEFAULT for ETM_ETMPIDR2 */
#define _ETM_ETMPIDR2_REV_SHIFT                       4                                    /**< Shift value for ETM_REV */
#define _ETM_ETMPIDR2_REV_MASK                        0xF0UL                               /**< Bit mask for ETM_REV */
#define _ETM_ETMPIDR2_REV_DEFAULT                     0x00000003UL                         /**< Mode DEFAULT for ETM_ETMPIDR2 */
#define ETM_ETMPIDR2_REV_DEFAULT                      (_ETM_ETMPIDR2_REV_DEFAULT << 4)     /**< Shifted mode DEFAULT for ETM_ETMPIDR2 */

/* Bit fields for ETM ETMPIDR3 */
#define _ETM_ETMPIDR3_RESETVALUE                      0x00000000UL                         /**< Default value for ETM_ETMPIDR3 */
#define _ETM_ETMPIDR3_MASK                            0x000000FFUL                         /**< Mask for ETM_ETMPIDR3 */
#define _ETM_ETMPIDR3_CUSTMOD_SHIFT                   0                                    /**< Shift value for ETM_CUSTMOD */
#define _ETM_ETMPIDR3_CUSTMOD_MASK                    0xFUL                                /**< Bit mask for ETM_CUSTMOD */
#define _ETM_ETMPIDR3_CUSTMOD_DEFAULT                 0x00000000UL                         /**< Mode DEFAULT for ETM_ETMPIDR3 */
#define ETM_ETMPIDR3_CUSTMOD_DEFAULT                  (_ETM_ETMPIDR3_CUSTMOD_DEFAULT << 0) /**< Shifted mode DEFAULT for ETM_ETMPIDR3 */
#define _ETM_ETMPIDR3_REVAND_SHIFT                    4                                    /**< Shift value for ETM_REVAND */
#define _ETM_ETMPIDR3_REVAND_MASK                     0xF0UL                               /**< Bit mask for ETM_REVAND */
#define _ETM_ETMPIDR3_REVAND_DEFAULT                  0x00000000UL                         /**< Mode DEFAULT for ETM_ETMPIDR3 */
#define ETM_ETMPIDR3_REVAND_DEFAULT                   (_ETM_ETMPIDR3_REVAND_DEFAULT << 4)  /**< Shifted mode DEFAULT for ETM_ETMPIDR3 */

/* Bit fields for ETM ETMCIDR0 */
#define _ETM_ETMCIDR0_RESETVALUE                      0x0000000DUL                        /**< Default value for ETM_ETMCIDR0 */
#define _ETM_ETMCIDR0_MASK                            0x000000FFUL                        /**< Mask for ETM_ETMCIDR0 */
#define _ETM_ETMCIDR0_PREAMB_SHIFT                    0                                   /**< Shift value for ETM_PREAMB */
#define _ETM_ETMCIDR0_PREAMB_MASK                     0xFFUL                              /**< Bit mask for ETM_PREAMB */
#define _ETM_ETMCIDR0_PREAMB_DEFAULT                  0x0000000DUL                        /**< Mode DEFAULT for ETM_ETMCIDR0 */
#define ETM_ETMCIDR0_PREAMB_DEFAULT                   (_ETM_ETMCIDR0_PREAMB_DEFAULT << 0) /**< Shifted mode DEFAULT for ETM_ETMCIDR0 */

/* Bit fields for ETM ETMCIDR1 */
#define _ETM_ETMCIDR1_RESETVALUE                      0x00000090UL                        /**< Default value for ETM_ETMCIDR1 */
#define _ETM_ETMCIDR1_MASK                            0x000000FFUL                        /**< Mask for ETM_ETMCIDR1 */
#define _ETM_ETMCIDR1_PREAMB_SHIFT                    0                                   /**< Shift value for ETM_PREAMB */
#define _ETM_ETMCIDR1_PREAMB_MASK                     0xFFUL                              /**< Bit mask for ETM_PREAMB */
#define _ETM_ETMCIDR1_PREAMB_DEFAULT                  0x00000090UL                        /**< Mode DEFAULT for ETM_ETMCIDR1 */
#define ETM_ETMCIDR1_PREAMB_DEFAULT                   (_ETM_ETMCIDR1_PREAMB_DEFAULT << 0) /**< Shifted mode DEFAULT for ETM_ETMCIDR1 */

/* Bit fields for ETM ETMCIDR2 */
#define _ETM_ETMCIDR2_RESETVALUE                      0x00000005UL                        /**< Default value for ETM_ETMCIDR2 */
#define _ETM_ETMCIDR2_MASK                            0x000000FFUL                        /**< Mask for ETM_ETMCIDR2 */
#define _ETM_ETMCIDR2_PREAMB_SHIFT                    0                                   /**< Shift value for ETM_PREAMB */
#define _ETM_ETMCIDR2_PREAMB_MASK                     0xFFUL                              /**< Bit mask for ETM_PREAMB */
#define _ETM_ETMCIDR2_PREAMB_DEFAULT                  0x00000005UL                        /**< Mode DEFAULT for ETM_ETMCIDR2 */
#define ETM_ETMCIDR2_PREAMB_DEFAULT                   (_ETM_ETMCIDR2_PREAMB_DEFAULT << 0) /**< Shifted mode DEFAULT for ETM_ETMCIDR2 */

/* Bit fields for ETM ETMCIDR3 */
#define _ETM_ETMCIDR3_RESETVALUE                      0x000000B1UL                        /**< Default value for ETM_ETMCIDR3 */
#define _ETM_ETMCIDR3_MASK                            0x000000FFUL                        /**< Mask for ETM_ETMCIDR3 */
#define _ETM_ETMCIDR3_PREAMB_SHIFT                    0                                   /**< Shift value for ETM_PREAMB */
#define _ETM_ETMCIDR3_PREAMB_MASK                     0xFFUL                              /**< Bit mask for ETM_PREAMB */
#define _ETM_ETMCIDR3_PREAMB_DEFAULT                  0x000000B1UL                        /**< Mode DEFAULT for ETM_ETMCIDR3 */
#define ETM_ETMCIDR3_PREAMB_DEFAULT                   (_ETM_ETMCIDR3_PREAMB_DEFAULT << 0) /**< Shifted mode DEFAULT for ETM_ETMCIDR3 */

/** @} End of group EFM32WG_ETM */
/** @} End of group Parts */

