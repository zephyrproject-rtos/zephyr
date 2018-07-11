/**************************************************************************//**
 * @file efr32fg1p_crypto.h
 * @brief EFR32FG1P_CRYPTO register and bit field definitions
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
 * @defgroup EFR32FG1P_CRYPTO
 * @{
 * @brief EFR32FG1P_CRYPTO Register Declaration
 *****************************************************************************/
typedef struct
{
  __IOM uint32_t CTRL;           /**< Control Register  */
  __IOM uint32_t WAC;            /**< Wide Arithmetic Configuration  */
  __IOM uint32_t CMD;            /**< Command Register  */
  uint32_t       RESERVED0[1];   /**< Reserved for future use **/
  __IM uint32_t  STATUS;         /**< Status Register  */
  __IM uint32_t  DSTATUS;        /**< Data Status Register  */
  __IM uint32_t  CSTATUS;        /**< Control Status Register  */
  uint32_t       RESERVED1[1];   /**< Reserved for future use **/
  __IOM uint32_t KEY;            /**< KEY Register Access  */
  __IOM uint32_t KEYBUF;         /**< KEY Buffer Register Access  */
  uint32_t       RESERVED2[2];   /**< Reserved for future use **/
  __IOM uint32_t SEQCTRL;        /**< Sequence Control  */
  __IOM uint32_t SEQCTRLB;       /**< Sequence Control B  */
  uint32_t       RESERVED3[2];   /**< Reserved for future use **/
  __IM uint32_t  IF;             /**< AES Interrupt Flags  */
  __IOM uint32_t IFS;            /**< Interrupt Flag Set Register  */
  __IOM uint32_t IFC;            /**< Interrupt Flag Clear Register  */
  __IOM uint32_t IEN;            /**< Interrupt Enable Register  */
  __IOM uint32_t SEQ0;           /**< Sequence register 0  */
  __IOM uint32_t SEQ1;           /**< Sequence Register 1  */
  __IOM uint32_t SEQ2;           /**< Sequence Register 2  */
  __IOM uint32_t SEQ3;           /**< Sequence Register 3  */
  __IOM uint32_t SEQ4;           /**< Sequence Register 4  */
  uint32_t       RESERVED4[7];   /**< Reserved for future use **/
  __IOM uint32_t DATA0;          /**< DATA0 Register Access  */
  __IOM uint32_t DATA1;          /**< DATA1 Register Access  */
  __IOM uint32_t DATA2;          /**< DATA2 Register Access  */
  __IOM uint32_t DATA3;          /**< DATA3 Register Access  */
  uint32_t       RESERVED5[4];   /**< Reserved for future use **/
  __IOM uint32_t DATA0XOR;       /**< DATA0XOR Register Access  */
  uint32_t       RESERVED6[3];   /**< Reserved for future use **/
  __IOM uint32_t DATA0BYTE;      /**< DATA0 Register Byte Access  */
  __IOM uint32_t DATA1BYTE;      /**< DATA1 Register Byte Access  */
  uint32_t       RESERVED7[1];   /**< Reserved for future use **/
  __IOM uint32_t DATA0XORBYTE;   /**< DATA0 Register Byte XOR Access  */
  __IOM uint32_t DATA0BYTE12;    /**< DATA0 Register Byte 12 Access  */
  __IOM uint32_t DATA0BYTE13;    /**< DATA0 Register Byte 13 Access  */
  __IOM uint32_t DATA0BYTE14;    /**< DATA0 Register Byte 14 Access  */
  __IOM uint32_t DATA0BYTE15;    /**< DATA0 Register Byte 15 Access  */
  uint32_t       RESERVED8[12];  /**< Reserved for future use **/
  __IOM uint32_t DDATA0;         /**< DDATA0 Register Access  */
  __IOM uint32_t DDATA1;         /**< DDATA1 Register Access  */
  __IOM uint32_t DDATA2;         /**< DDATA2 Register Access  */
  __IOM uint32_t DDATA3;         /**< DDATA3 Register Access  */
  __IOM uint32_t DDATA4;         /**< DDATA4 Register Access  */
  uint32_t       RESERVED9[7];   /**< Reserved for future use **/
  __IOM uint32_t DDATA0BIG;      /**< DDATA0 Register Big Endian Access  */
  uint32_t       RESERVED10[3];  /**< Reserved for future use **/
  __IOM uint32_t DDATA0BYTE;     /**< DDATA0 Register Byte Access  */
  __IOM uint32_t DDATA1BYTE;     /**< DDATA1 Register Byte Access  */
  __IOM uint32_t DDATA0BYTE32;   /**< DDATA0 Register Byte 32 access.  */
  uint32_t       RESERVED11[13]; /**< Reserved for future use **/
  __IOM uint32_t QDATA0;         /**< QDATA0 Register Access  */
  __IOM uint32_t QDATA1;         /**< QDATA1 Register Access  */
  uint32_t       RESERVED12[7];  /**< Reserved for future use **/
  __IOM uint32_t QDATA1BIG;      /**< QDATA1 Register Big Endian Access  */
  uint32_t       RESERVED13[6];  /**< Reserved for future use **/
  __IOM uint32_t QDATA0BYTE;     /**< QDATA0 Register Byte Access  */
  __IOM uint32_t QDATA1BYTE;     /**< QDATA1 Register Byte Access  */
} CRYPTO_TypeDef;                /** @} */

/**************************************************************************//**
 * @defgroup EFR32FG1P_CRYPTO_BitFields
 * @{
 *****************************************************************************/

/* Bit fields for CRYPTO CTRL */
#define _CRYPTO_CTRL_RESETVALUE                      0x00000000UL                               /**< Default value for CRYPTO_CTRL */
#define _CRYPTO_CTRL_MASK                            0xB333C407UL                               /**< Mask for CRYPTO_CTRL */
#define CRYPTO_CTRL_AES                              (0x1UL << 0)                               /**< AES Mode */
#define _CRYPTO_CTRL_AES_SHIFT                       0                                          /**< Shift value for CRYPTO_AES */
#define _CRYPTO_CTRL_AES_MASK                        0x1UL                                      /**< Bit mask for CRYPTO_AES */
#define _CRYPTO_CTRL_AES_DEFAULT                     0x00000000UL                               /**< Mode DEFAULT for CRYPTO_CTRL */
#define _CRYPTO_CTRL_AES_AES128                      0x00000000UL                               /**< Mode AES128 for CRYPTO_CTRL */
#define _CRYPTO_CTRL_AES_AES256                      0x00000001UL                               /**< Mode AES256 for CRYPTO_CTRL */
#define CRYPTO_CTRL_AES_DEFAULT                      (_CRYPTO_CTRL_AES_DEFAULT << 0)            /**< Shifted mode DEFAULT for CRYPTO_CTRL */
#define CRYPTO_CTRL_AES_AES128                       (_CRYPTO_CTRL_AES_AES128 << 0)             /**< Shifted mode AES128 for CRYPTO_CTRL */
#define CRYPTO_CTRL_AES_AES256                       (_CRYPTO_CTRL_AES_AES256 << 0)             /**< Shifted mode AES256 for CRYPTO_CTRL */
#define CRYPTO_CTRL_KEYBUFDIS                        (0x1UL << 1)                               /**< Key Buffer Disable */
#define _CRYPTO_CTRL_KEYBUFDIS_SHIFT                 1                                          /**< Shift value for CRYPTO_KEYBUFDIS */
#define _CRYPTO_CTRL_KEYBUFDIS_MASK                  0x2UL                                      /**< Bit mask for CRYPTO_KEYBUFDIS */
#define _CRYPTO_CTRL_KEYBUFDIS_DEFAULT               0x00000000UL                               /**< Mode DEFAULT for CRYPTO_CTRL */
#define CRYPTO_CTRL_KEYBUFDIS_DEFAULT                (_CRYPTO_CTRL_KEYBUFDIS_DEFAULT << 1)      /**< Shifted mode DEFAULT for CRYPTO_CTRL */
#define CRYPTO_CTRL_SHA                              (0x1UL << 2)                               /**< SHA Mode */
#define _CRYPTO_CTRL_SHA_SHIFT                       2                                          /**< Shift value for CRYPTO_SHA */
#define _CRYPTO_CTRL_SHA_MASK                        0x4UL                                      /**< Bit mask for CRYPTO_SHA */
#define _CRYPTO_CTRL_SHA_DEFAULT                     0x00000000UL                               /**< Mode DEFAULT for CRYPTO_CTRL */
#define _CRYPTO_CTRL_SHA_SHA1                        0x00000000UL                               /**< Mode SHA1 for CRYPTO_CTRL */
#define _CRYPTO_CTRL_SHA_SHA2                        0x00000001UL                               /**< Mode SHA2 for CRYPTO_CTRL */
#define CRYPTO_CTRL_SHA_DEFAULT                      (_CRYPTO_CTRL_SHA_DEFAULT << 2)            /**< Shifted mode DEFAULT for CRYPTO_CTRL */
#define CRYPTO_CTRL_SHA_SHA1                         (_CRYPTO_CTRL_SHA_SHA1 << 2)               /**< Shifted mode SHA1 for CRYPTO_CTRL */
#define CRYPTO_CTRL_SHA_SHA2                         (_CRYPTO_CTRL_SHA_SHA2 << 2)               /**< Shifted mode SHA2 for CRYPTO_CTRL */
#define CRYPTO_CTRL_NOBUSYSTALL                      (0x1UL << 10)                              /**< No Stalling of Bus When Busy */
#define _CRYPTO_CTRL_NOBUSYSTALL_SHIFT               10                                         /**< Shift value for CRYPTO_NOBUSYSTALL */
#define _CRYPTO_CTRL_NOBUSYSTALL_MASK                0x400UL                                    /**< Bit mask for CRYPTO_NOBUSYSTALL */
#define _CRYPTO_CTRL_NOBUSYSTALL_DEFAULT             0x00000000UL                               /**< Mode DEFAULT for CRYPTO_CTRL */
#define CRYPTO_CTRL_NOBUSYSTALL_DEFAULT              (_CRYPTO_CTRL_NOBUSYSTALL_DEFAULT << 10)   /**< Shifted mode DEFAULT for CRYPTO_CTRL */
#define _CRYPTO_CTRL_INCWIDTH_SHIFT                  14                                         /**< Shift value for CRYPTO_INCWIDTH */
#define _CRYPTO_CTRL_INCWIDTH_MASK                   0xC000UL                                   /**< Bit mask for CRYPTO_INCWIDTH */
#define _CRYPTO_CTRL_INCWIDTH_DEFAULT                0x00000000UL                               /**< Mode DEFAULT for CRYPTO_CTRL */
#define _CRYPTO_CTRL_INCWIDTH_INCWIDTH1              0x00000000UL                               /**< Mode INCWIDTH1 for CRYPTO_CTRL */
#define _CRYPTO_CTRL_INCWIDTH_INCWIDTH2              0x00000001UL                               /**< Mode INCWIDTH2 for CRYPTO_CTRL */
#define _CRYPTO_CTRL_INCWIDTH_INCWIDTH3              0x00000002UL                               /**< Mode INCWIDTH3 for CRYPTO_CTRL */
#define _CRYPTO_CTRL_INCWIDTH_INCWIDTH4              0x00000003UL                               /**< Mode INCWIDTH4 for CRYPTO_CTRL */
#define CRYPTO_CTRL_INCWIDTH_DEFAULT                 (_CRYPTO_CTRL_INCWIDTH_DEFAULT << 14)      /**< Shifted mode DEFAULT for CRYPTO_CTRL */
#define CRYPTO_CTRL_INCWIDTH_INCWIDTH1               (_CRYPTO_CTRL_INCWIDTH_INCWIDTH1 << 14)    /**< Shifted mode INCWIDTH1 for CRYPTO_CTRL */
#define CRYPTO_CTRL_INCWIDTH_INCWIDTH2               (_CRYPTO_CTRL_INCWIDTH_INCWIDTH2 << 14)    /**< Shifted mode INCWIDTH2 for CRYPTO_CTRL */
#define CRYPTO_CTRL_INCWIDTH_INCWIDTH3               (_CRYPTO_CTRL_INCWIDTH_INCWIDTH3 << 14)    /**< Shifted mode INCWIDTH3 for CRYPTO_CTRL */
#define CRYPTO_CTRL_INCWIDTH_INCWIDTH4               (_CRYPTO_CTRL_INCWIDTH_INCWIDTH4 << 14)    /**< Shifted mode INCWIDTH4 for CRYPTO_CTRL */
#define _CRYPTO_CTRL_DMA0MODE_SHIFT                  16                                         /**< Shift value for CRYPTO_DMA0MODE */
#define _CRYPTO_CTRL_DMA0MODE_MASK                   0x30000UL                                  /**< Bit mask for CRYPTO_DMA0MODE */
#define _CRYPTO_CTRL_DMA0MODE_DEFAULT                0x00000000UL                               /**< Mode DEFAULT for CRYPTO_CTRL */
#define _CRYPTO_CTRL_DMA0MODE_FULL                   0x00000000UL                               /**< Mode FULL for CRYPTO_CTRL */
#define _CRYPTO_CTRL_DMA0MODE_LENLIMIT               0x00000001UL                               /**< Mode LENLIMIT for CRYPTO_CTRL */
#define _CRYPTO_CTRL_DMA0MODE_FULLBYTE               0x00000002UL                               /**< Mode FULLBYTE for CRYPTO_CTRL */
#define _CRYPTO_CTRL_DMA0MODE_LENLIMITBYTE           0x00000003UL                               /**< Mode LENLIMITBYTE for CRYPTO_CTRL */
#define CRYPTO_CTRL_DMA0MODE_DEFAULT                 (_CRYPTO_CTRL_DMA0MODE_DEFAULT << 16)      /**< Shifted mode DEFAULT for CRYPTO_CTRL */
#define CRYPTO_CTRL_DMA0MODE_FULL                    (_CRYPTO_CTRL_DMA0MODE_FULL << 16)         /**< Shifted mode FULL for CRYPTO_CTRL */
#define CRYPTO_CTRL_DMA0MODE_LENLIMIT                (_CRYPTO_CTRL_DMA0MODE_LENLIMIT << 16)     /**< Shifted mode LENLIMIT for CRYPTO_CTRL */
#define CRYPTO_CTRL_DMA0MODE_FULLBYTE                (_CRYPTO_CTRL_DMA0MODE_FULLBYTE << 16)     /**< Shifted mode FULLBYTE for CRYPTO_CTRL */
#define CRYPTO_CTRL_DMA0MODE_LENLIMITBYTE            (_CRYPTO_CTRL_DMA0MODE_LENLIMITBYTE << 16) /**< Shifted mode LENLIMITBYTE for CRYPTO_CTRL */
#define _CRYPTO_CTRL_DMA0RSEL_SHIFT                  20                                         /**< Shift value for CRYPTO_DMA0RSEL */
#define _CRYPTO_CTRL_DMA0RSEL_MASK                   0x300000UL                                 /**< Bit mask for CRYPTO_DMA0RSEL */
#define _CRYPTO_CTRL_DMA0RSEL_DEFAULT                0x00000000UL                               /**< Mode DEFAULT for CRYPTO_CTRL */
#define _CRYPTO_CTRL_DMA0RSEL_DATA0                  0x00000000UL                               /**< Mode DATA0 for CRYPTO_CTRL */
#define _CRYPTO_CTRL_DMA0RSEL_DDATA0                 0x00000001UL                               /**< Mode DDATA0 for CRYPTO_CTRL */
#define _CRYPTO_CTRL_DMA0RSEL_DDATA0BIG              0x00000002UL                               /**< Mode DDATA0BIG for CRYPTO_CTRL */
#define _CRYPTO_CTRL_DMA0RSEL_QDATA0                 0x00000003UL                               /**< Mode QDATA0 for CRYPTO_CTRL */
#define CRYPTO_CTRL_DMA0RSEL_DEFAULT                 (_CRYPTO_CTRL_DMA0RSEL_DEFAULT << 20)      /**< Shifted mode DEFAULT for CRYPTO_CTRL */
#define CRYPTO_CTRL_DMA0RSEL_DATA0                   (_CRYPTO_CTRL_DMA0RSEL_DATA0 << 20)        /**< Shifted mode DATA0 for CRYPTO_CTRL */
#define CRYPTO_CTRL_DMA0RSEL_DDATA0                  (_CRYPTO_CTRL_DMA0RSEL_DDATA0 << 20)       /**< Shifted mode DDATA0 for CRYPTO_CTRL */
#define CRYPTO_CTRL_DMA0RSEL_DDATA0BIG               (_CRYPTO_CTRL_DMA0RSEL_DDATA0BIG << 20)    /**< Shifted mode DDATA0BIG for CRYPTO_CTRL */
#define CRYPTO_CTRL_DMA0RSEL_QDATA0                  (_CRYPTO_CTRL_DMA0RSEL_QDATA0 << 20)       /**< Shifted mode QDATA0 for CRYPTO_CTRL */
#define _CRYPTO_CTRL_DMA1MODE_SHIFT                  24                                         /**< Shift value for CRYPTO_DMA1MODE */
#define _CRYPTO_CTRL_DMA1MODE_MASK                   0x3000000UL                                /**< Bit mask for CRYPTO_DMA1MODE */
#define _CRYPTO_CTRL_DMA1MODE_DEFAULT                0x00000000UL                               /**< Mode DEFAULT for CRYPTO_CTRL */
#define _CRYPTO_CTRL_DMA1MODE_FULL                   0x00000000UL                               /**< Mode FULL for CRYPTO_CTRL */
#define _CRYPTO_CTRL_DMA1MODE_LENLIMIT               0x00000001UL                               /**< Mode LENLIMIT for CRYPTO_CTRL */
#define _CRYPTO_CTRL_DMA1MODE_FULLBYTE               0x00000002UL                               /**< Mode FULLBYTE for CRYPTO_CTRL */
#define _CRYPTO_CTRL_DMA1MODE_LENLIMITBYTE           0x00000003UL                               /**< Mode LENLIMITBYTE for CRYPTO_CTRL */
#define CRYPTO_CTRL_DMA1MODE_DEFAULT                 (_CRYPTO_CTRL_DMA1MODE_DEFAULT << 24)      /**< Shifted mode DEFAULT for CRYPTO_CTRL */
#define CRYPTO_CTRL_DMA1MODE_FULL                    (_CRYPTO_CTRL_DMA1MODE_FULL << 24)         /**< Shifted mode FULL for CRYPTO_CTRL */
#define CRYPTO_CTRL_DMA1MODE_LENLIMIT                (_CRYPTO_CTRL_DMA1MODE_LENLIMIT << 24)     /**< Shifted mode LENLIMIT for CRYPTO_CTRL */
#define CRYPTO_CTRL_DMA1MODE_FULLBYTE                (_CRYPTO_CTRL_DMA1MODE_FULLBYTE << 24)     /**< Shifted mode FULLBYTE for CRYPTO_CTRL */
#define CRYPTO_CTRL_DMA1MODE_LENLIMITBYTE            (_CRYPTO_CTRL_DMA1MODE_LENLIMITBYTE << 24) /**< Shifted mode LENLIMITBYTE for CRYPTO_CTRL */
#define _CRYPTO_CTRL_DMA1RSEL_SHIFT                  28                                         /**< Shift value for CRYPTO_DMA1RSEL */
#define _CRYPTO_CTRL_DMA1RSEL_MASK                   0x30000000UL                               /**< Bit mask for CRYPTO_DMA1RSEL */
#define _CRYPTO_CTRL_DMA1RSEL_DEFAULT                0x00000000UL                               /**< Mode DEFAULT for CRYPTO_CTRL */
#define _CRYPTO_CTRL_DMA1RSEL_DATA1                  0x00000000UL                               /**< Mode DATA1 for CRYPTO_CTRL */
#define _CRYPTO_CTRL_DMA1RSEL_DDATA1                 0x00000001UL                               /**< Mode DDATA1 for CRYPTO_CTRL */
#define _CRYPTO_CTRL_DMA1RSEL_QDATA1                 0x00000002UL                               /**< Mode QDATA1 for CRYPTO_CTRL */
#define _CRYPTO_CTRL_DMA1RSEL_QDATA1BIG              0x00000003UL                               /**< Mode QDATA1BIG for CRYPTO_CTRL */
#define CRYPTO_CTRL_DMA1RSEL_DEFAULT                 (_CRYPTO_CTRL_DMA1RSEL_DEFAULT << 28)      /**< Shifted mode DEFAULT for CRYPTO_CTRL */
#define CRYPTO_CTRL_DMA1RSEL_DATA1                   (_CRYPTO_CTRL_DMA1RSEL_DATA1 << 28)        /**< Shifted mode DATA1 for CRYPTO_CTRL */
#define CRYPTO_CTRL_DMA1RSEL_DDATA1                  (_CRYPTO_CTRL_DMA1RSEL_DDATA1 << 28)       /**< Shifted mode DDATA1 for CRYPTO_CTRL */
#define CRYPTO_CTRL_DMA1RSEL_QDATA1                  (_CRYPTO_CTRL_DMA1RSEL_QDATA1 << 28)       /**< Shifted mode QDATA1 for CRYPTO_CTRL */
#define CRYPTO_CTRL_DMA1RSEL_QDATA1BIG               (_CRYPTO_CTRL_DMA1RSEL_QDATA1BIG << 28)    /**< Shifted mode QDATA1BIG for CRYPTO_CTRL */
#define CRYPTO_CTRL_COMBDMA0WEREQ                    (0x1UL << 31)                              /**< Combined Data0 Write DMA Request */
#define _CRYPTO_CTRL_COMBDMA0WEREQ_SHIFT             31                                         /**< Shift value for CRYPTO_COMBDMA0WEREQ */
#define _CRYPTO_CTRL_COMBDMA0WEREQ_MASK              0x80000000UL                               /**< Bit mask for CRYPTO_COMBDMA0WEREQ */
#define _CRYPTO_CTRL_COMBDMA0WEREQ_DEFAULT           0x00000000UL                               /**< Mode DEFAULT for CRYPTO_CTRL */
#define CRYPTO_CTRL_COMBDMA0WEREQ_DEFAULT            (_CRYPTO_CTRL_COMBDMA0WEREQ_DEFAULT << 31) /**< Shifted mode DEFAULT for CRYPTO_CTRL */

/* Bit fields for CRYPTO WAC */
#define _CRYPTO_WAC_RESETVALUE                       0x00000000UL                            /**< Default value for CRYPTO_WAC */
#define _CRYPTO_WAC_MASK                             0x00000F1FUL                            /**< Mask for CRYPTO_WAC */
#define _CRYPTO_WAC_MODULUS_SHIFT                    0                                       /**< Shift value for CRYPTO_MODULUS */
#define _CRYPTO_WAC_MODULUS_MASK                     0xFUL                                   /**< Bit mask for CRYPTO_MODULUS */
#define _CRYPTO_WAC_MODULUS_DEFAULT                  0x00000000UL                            /**< Mode DEFAULT for CRYPTO_WAC */
#define _CRYPTO_WAC_MODULUS_BIN256                   0x00000000UL                            /**< Mode BIN256 for CRYPTO_WAC */
#define _CRYPTO_WAC_MODULUS_BIN128                   0x00000001UL                            /**< Mode BIN128 for CRYPTO_WAC */
#define _CRYPTO_WAC_MODULUS_ECCBIN233P               0x00000002UL                            /**< Mode ECCBIN233P for CRYPTO_WAC */
#define _CRYPTO_WAC_MODULUS_ECCBIN163P               0x00000003UL                            /**< Mode ECCBIN163P for CRYPTO_WAC */
#define _CRYPTO_WAC_MODULUS_GCMBIN128                0x00000004UL                            /**< Mode GCMBIN128 for CRYPTO_WAC */
#define _CRYPTO_WAC_MODULUS_ECCPRIME256P             0x00000005UL                            /**< Mode ECCPRIME256P for CRYPTO_WAC */
#define _CRYPTO_WAC_MODULUS_ECCPRIME224P             0x00000006UL                            /**< Mode ECCPRIME224P for CRYPTO_WAC */
#define _CRYPTO_WAC_MODULUS_ECCPRIME192P             0x00000007UL                            /**< Mode ECCPRIME192P for CRYPTO_WAC */
#define _CRYPTO_WAC_MODULUS_ECCBIN233N               0x00000008UL                            /**< Mode ECCBIN233N for CRYPTO_WAC */
#define _CRYPTO_WAC_MODULUS_ECCBIN233KN              0x00000009UL                            /**< Mode ECCBIN233KN for CRYPTO_WAC */
#define _CRYPTO_WAC_MODULUS_ECCBIN163N               0x0000000AUL                            /**< Mode ECCBIN163N for CRYPTO_WAC */
#define _CRYPTO_WAC_MODULUS_ECCBIN163KN              0x0000000BUL                            /**< Mode ECCBIN163KN for CRYPTO_WAC */
#define _CRYPTO_WAC_MODULUS_ECCPRIME256N             0x0000000CUL                            /**< Mode ECCPRIME256N for CRYPTO_WAC */
#define _CRYPTO_WAC_MODULUS_ECCPRIME224N             0x0000000DUL                            /**< Mode ECCPRIME224N for CRYPTO_WAC */
#define _CRYPTO_WAC_MODULUS_ECCPRIME192N             0x0000000EUL                            /**< Mode ECCPRIME192N for CRYPTO_WAC */
#define CRYPTO_WAC_MODULUS_DEFAULT                   (_CRYPTO_WAC_MODULUS_DEFAULT << 0)      /**< Shifted mode DEFAULT for CRYPTO_WAC */
#define CRYPTO_WAC_MODULUS_BIN256                    (_CRYPTO_WAC_MODULUS_BIN256 << 0)       /**< Shifted mode BIN256 for CRYPTO_WAC */
#define CRYPTO_WAC_MODULUS_BIN128                    (_CRYPTO_WAC_MODULUS_BIN128 << 0)       /**< Shifted mode BIN128 for CRYPTO_WAC */
#define CRYPTO_WAC_MODULUS_ECCBIN233P                (_CRYPTO_WAC_MODULUS_ECCBIN233P << 0)   /**< Shifted mode ECCBIN233P for CRYPTO_WAC */
#define CRYPTO_WAC_MODULUS_ECCBIN163P                (_CRYPTO_WAC_MODULUS_ECCBIN163P << 0)   /**< Shifted mode ECCBIN163P for CRYPTO_WAC */
#define CRYPTO_WAC_MODULUS_GCMBIN128                 (_CRYPTO_WAC_MODULUS_GCMBIN128 << 0)    /**< Shifted mode GCMBIN128 for CRYPTO_WAC */
#define CRYPTO_WAC_MODULUS_ECCPRIME256P              (_CRYPTO_WAC_MODULUS_ECCPRIME256P << 0) /**< Shifted mode ECCPRIME256P for CRYPTO_WAC */
#define CRYPTO_WAC_MODULUS_ECCPRIME224P              (_CRYPTO_WAC_MODULUS_ECCPRIME224P << 0) /**< Shifted mode ECCPRIME224P for CRYPTO_WAC */
#define CRYPTO_WAC_MODULUS_ECCPRIME192P              (_CRYPTO_WAC_MODULUS_ECCPRIME192P << 0) /**< Shifted mode ECCPRIME192P for CRYPTO_WAC */
#define CRYPTO_WAC_MODULUS_ECCBIN233N                (_CRYPTO_WAC_MODULUS_ECCBIN233N << 0)   /**< Shifted mode ECCBIN233N for CRYPTO_WAC */
#define CRYPTO_WAC_MODULUS_ECCBIN233KN               (_CRYPTO_WAC_MODULUS_ECCBIN233KN << 0)  /**< Shifted mode ECCBIN233KN for CRYPTO_WAC */
#define CRYPTO_WAC_MODULUS_ECCBIN163N                (_CRYPTO_WAC_MODULUS_ECCBIN163N << 0)   /**< Shifted mode ECCBIN163N for CRYPTO_WAC */
#define CRYPTO_WAC_MODULUS_ECCBIN163KN               (_CRYPTO_WAC_MODULUS_ECCBIN163KN << 0)  /**< Shifted mode ECCBIN163KN for CRYPTO_WAC */
#define CRYPTO_WAC_MODULUS_ECCPRIME256N              (_CRYPTO_WAC_MODULUS_ECCPRIME256N << 0) /**< Shifted mode ECCPRIME256N for CRYPTO_WAC */
#define CRYPTO_WAC_MODULUS_ECCPRIME224N              (_CRYPTO_WAC_MODULUS_ECCPRIME224N << 0) /**< Shifted mode ECCPRIME224N for CRYPTO_WAC */
#define CRYPTO_WAC_MODULUS_ECCPRIME192N              (_CRYPTO_WAC_MODULUS_ECCPRIME192N << 0) /**< Shifted mode ECCPRIME192N for CRYPTO_WAC */
#define CRYPTO_WAC_MODOP                             (0x1UL << 4)                            /**< Modular Operation Field Type */
#define _CRYPTO_WAC_MODOP_SHIFT                      4                                       /**< Shift value for CRYPTO_MODOP */
#define _CRYPTO_WAC_MODOP_MASK                       0x10UL                                  /**< Bit mask for CRYPTO_MODOP */
#define _CRYPTO_WAC_MODOP_DEFAULT                    0x00000000UL                            /**< Mode DEFAULT for CRYPTO_WAC */
#define _CRYPTO_WAC_MODOP_BINARY                     0x00000000UL                            /**< Mode BINARY for CRYPTO_WAC */
#define _CRYPTO_WAC_MODOP_REGULAR                    0x00000001UL                            /**< Mode REGULAR for CRYPTO_WAC */
#define CRYPTO_WAC_MODOP_DEFAULT                     (_CRYPTO_WAC_MODOP_DEFAULT << 4)        /**< Shifted mode DEFAULT for CRYPTO_WAC */
#define CRYPTO_WAC_MODOP_BINARY                      (_CRYPTO_WAC_MODOP_BINARY << 4)         /**< Shifted mode BINARY for CRYPTO_WAC */
#define CRYPTO_WAC_MODOP_REGULAR                     (_CRYPTO_WAC_MODOP_REGULAR << 4)        /**< Shifted mode REGULAR for CRYPTO_WAC */
#define _CRYPTO_WAC_MULWIDTH_SHIFT                   8                                       /**< Shift value for CRYPTO_MULWIDTH */
#define _CRYPTO_WAC_MULWIDTH_MASK                    0x300UL                                 /**< Bit mask for CRYPTO_MULWIDTH */
#define _CRYPTO_WAC_MULWIDTH_DEFAULT                 0x00000000UL                            /**< Mode DEFAULT for CRYPTO_WAC */
#define _CRYPTO_WAC_MULWIDTH_MUL256                  0x00000000UL                            /**< Mode MUL256 for CRYPTO_WAC */
#define _CRYPTO_WAC_MULWIDTH_MUL128                  0x00000001UL                            /**< Mode MUL128 for CRYPTO_WAC */
#define _CRYPTO_WAC_MULWIDTH_MULMOD                  0x00000002UL                            /**< Mode MULMOD for CRYPTO_WAC */
#define CRYPTO_WAC_MULWIDTH_DEFAULT                  (_CRYPTO_WAC_MULWIDTH_DEFAULT << 8)     /**< Shifted mode DEFAULT for CRYPTO_WAC */
#define CRYPTO_WAC_MULWIDTH_MUL256                   (_CRYPTO_WAC_MULWIDTH_MUL256 << 8)      /**< Shifted mode MUL256 for CRYPTO_WAC */
#define CRYPTO_WAC_MULWIDTH_MUL128                   (_CRYPTO_WAC_MULWIDTH_MUL128 << 8)      /**< Shifted mode MUL128 for CRYPTO_WAC */
#define CRYPTO_WAC_MULWIDTH_MULMOD                   (_CRYPTO_WAC_MULWIDTH_MULMOD << 8)      /**< Shifted mode MULMOD for CRYPTO_WAC */
#define _CRYPTO_WAC_RESULTWIDTH_SHIFT                10                                      /**< Shift value for CRYPTO_RESULTWIDTH */
#define _CRYPTO_WAC_RESULTWIDTH_MASK                 0xC00UL                                 /**< Bit mask for CRYPTO_RESULTWIDTH */
#define _CRYPTO_WAC_RESULTWIDTH_DEFAULT              0x00000000UL                            /**< Mode DEFAULT for CRYPTO_WAC */
#define _CRYPTO_WAC_RESULTWIDTH_256BIT               0x00000000UL                            /**< Mode 256BIT for CRYPTO_WAC */
#define _CRYPTO_WAC_RESULTWIDTH_128BIT               0x00000001UL                            /**< Mode 128BIT for CRYPTO_WAC */
#define _CRYPTO_WAC_RESULTWIDTH_260BIT               0x00000002UL                            /**< Mode 260BIT for CRYPTO_WAC */
#define CRYPTO_WAC_RESULTWIDTH_DEFAULT               (_CRYPTO_WAC_RESULTWIDTH_DEFAULT << 10) /**< Shifted mode DEFAULT for CRYPTO_WAC */
#define CRYPTO_WAC_RESULTWIDTH_256BIT                (_CRYPTO_WAC_RESULTWIDTH_256BIT << 10)  /**< Shifted mode 256BIT for CRYPTO_WAC */
#define CRYPTO_WAC_RESULTWIDTH_128BIT                (_CRYPTO_WAC_RESULTWIDTH_128BIT << 10)  /**< Shifted mode 128BIT for CRYPTO_WAC */
#define CRYPTO_WAC_RESULTWIDTH_260BIT                (_CRYPTO_WAC_RESULTWIDTH_260BIT << 10)  /**< Shifted mode 260BIT for CRYPTO_WAC */

/* Bit fields for CRYPTO CMD */
#define _CRYPTO_CMD_RESETVALUE                       0x00000000UL                                /**< Default value for CRYPTO_CMD */
#define _CRYPTO_CMD_MASK                             0x00000EFFUL                                /**< Mask for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_SHIFT                      0                                           /**< Shift value for CRYPTO_INSTR */
#define _CRYPTO_CMD_INSTR_MASK                       0xFFUL                                      /**< Bit mask for CRYPTO_INSTR */
#define _CRYPTO_CMD_INSTR_DEFAULT                    0x00000000UL                                /**< Mode DEFAULT for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_END                        0x00000000UL                                /**< Mode END for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_EXEC                       0x00000001UL                                /**< Mode EXEC for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_DATA1INC                   0x00000003UL                                /**< Mode DATA1INC for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_DATA1INCCLR                0x00000004UL                                /**< Mode DATA1INCCLR for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_AESENC                     0x00000005UL                                /**< Mode AESENC for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_AESDEC                     0x00000006UL                                /**< Mode AESDEC for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_SHA                        0x00000007UL                                /**< Mode SHA for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_ADD                        0x00000008UL                                /**< Mode ADD for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_ADDC                       0x00000009UL                                /**< Mode ADDC for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_MADD                       0x0000000CUL                                /**< Mode MADD for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_MADD32                     0x0000000DUL                                /**< Mode MADD32 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_SUB                        0x00000010UL                                /**< Mode SUB for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_SUBC                       0x00000011UL                                /**< Mode SUBC for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_MSUB                       0x00000014UL                                /**< Mode MSUB for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_MUL                        0x00000018UL                                /**< Mode MUL for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_MULC                       0x00000019UL                                /**< Mode MULC for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_MMUL                       0x0000001CUL                                /**< Mode MMUL for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_MULO                       0x0000001DUL                                /**< Mode MULO for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_SHL                        0x00000020UL                                /**< Mode SHL for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_SHLC                       0x00000021UL                                /**< Mode SHLC for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_SHLB                       0x00000022UL                                /**< Mode SHLB for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_SHL1                       0x00000023UL                                /**< Mode SHL1 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_SHR                        0x00000024UL                                /**< Mode SHR for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_SHRC                       0x00000025UL                                /**< Mode SHRC for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_SHRB                       0x00000026UL                                /**< Mode SHRB for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_SHR1                       0x00000027UL                                /**< Mode SHR1 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_ADDO                       0x00000028UL                                /**< Mode ADDO for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_ADDIC                      0x00000029UL                                /**< Mode ADDIC for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_CLR                        0x00000030UL                                /**< Mode CLR for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_XOR                        0x00000031UL                                /**< Mode XOR for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_INV                        0x00000032UL                                /**< Mode INV for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_CSET                       0x00000034UL                                /**< Mode CSET for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_CCLR                       0x00000035UL                                /**< Mode CCLR for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_BBSWAP128                  0x00000036UL                                /**< Mode BBSWAP128 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_INC                        0x00000038UL                                /**< Mode INC for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_DEC                        0x00000039UL                                /**< Mode DEC for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_SHRA                       0x0000003EUL                                /**< Mode SHRA for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_DATA0TODATA0               0x00000040UL                                /**< Mode DATA0TODATA0 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_DATA0TODATA0XOR            0x00000041UL                                /**< Mode DATA0TODATA0XOR for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_DATA0TODATA0XORLEN         0x00000042UL                                /**< Mode DATA0TODATA0XORLEN for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_DATA0TODATA1               0x00000044UL                                /**< Mode DATA0TODATA1 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_DATA0TODATA2               0x00000045UL                                /**< Mode DATA0TODATA2 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_DATA0TODATA3               0x00000046UL                                /**< Mode DATA0TODATA3 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_DATA1TODATA0               0x00000048UL                                /**< Mode DATA1TODATA0 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_DATA1TODATA0XOR            0x00000049UL                                /**< Mode DATA1TODATA0XOR for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_DATA1TODATA0XORLEN         0x0000004AUL                                /**< Mode DATA1TODATA0XORLEN for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_DATA1TODATA2               0x0000004DUL                                /**< Mode DATA1TODATA2 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_DATA1TODATA3               0x0000004EUL                                /**< Mode DATA1TODATA3 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_DATA2TODATA0               0x00000050UL                                /**< Mode DATA2TODATA0 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_DATA2TODATA0XOR            0x00000051UL                                /**< Mode DATA2TODATA0XOR for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_DATA2TODATA0XORLEN         0x00000052UL                                /**< Mode DATA2TODATA0XORLEN for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_DATA2TODATA1               0x00000054UL                                /**< Mode DATA2TODATA1 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_DATA2TODATA3               0x00000056UL                                /**< Mode DATA2TODATA3 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_DATA3TODATA0               0x00000058UL                                /**< Mode DATA3TODATA0 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_DATA3TODATA0XOR            0x00000059UL                                /**< Mode DATA3TODATA0XOR for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_DATA3TODATA0XORLEN         0x0000005AUL                                /**< Mode DATA3TODATA0XORLEN for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_DATA3TODATA1               0x0000005CUL                                /**< Mode DATA3TODATA1 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_DATA3TODATA2               0x0000005DUL                                /**< Mode DATA3TODATA2 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_DATATODMA0                 0x00000063UL                                /**< Mode DATATODMA0 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_DATA0TOBUF                 0x00000064UL                                /**< Mode DATA0TOBUF for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_DATA0TOBUFXOR              0x00000065UL                                /**< Mode DATA0TOBUFXOR for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_DATATODMA1                 0x0000006BUL                                /**< Mode DATATODMA1 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_DATA1TOBUF                 0x0000006CUL                                /**< Mode DATA1TOBUF for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_DATA1TOBUFXOR              0x0000006DUL                                /**< Mode DATA1TOBUFXOR for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_DMA0TODATA                 0x00000070UL                                /**< Mode DMA0TODATA for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_DMA0TODATAXOR              0x00000071UL                                /**< Mode DMA0TODATAXOR for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_DMA1TODATA                 0x00000072UL                                /**< Mode DMA1TODATA for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_BUFTODATA0                 0x00000078UL                                /**< Mode BUFTODATA0 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_BUFTODATA0XOR              0x00000079UL                                /**< Mode BUFTODATA0XOR for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_BUFTODATA1                 0x0000007AUL                                /**< Mode BUFTODATA1 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_DDATA0TODDATA1             0x00000081UL                                /**< Mode DDATA0TODDATA1 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_DDATA0TODDATA2             0x00000082UL                                /**< Mode DDATA0TODDATA2 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_DDATA0TODDATA3             0x00000083UL                                /**< Mode DDATA0TODDATA3 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_DDATA0TODDATA4             0x00000084UL                                /**< Mode DDATA0TODDATA4 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_DDATA0LTODATA0             0x00000085UL                                /**< Mode DDATA0LTODATA0 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_DDATA0HTODATA1             0x00000086UL                                /**< Mode DDATA0HTODATA1 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_DDATA0LTODATA2             0x00000087UL                                /**< Mode DDATA0LTODATA2 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_DDATA1TODDATA0             0x00000088UL                                /**< Mode DDATA1TODDATA0 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_DDATA1TODDATA2             0x0000008AUL                                /**< Mode DDATA1TODDATA2 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_DDATA1TODDATA3             0x0000008BUL                                /**< Mode DDATA1TODDATA3 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_DDATA1TODDATA4             0x0000008CUL                                /**< Mode DDATA1TODDATA4 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_DDATA1LTODATA0             0x0000008DUL                                /**< Mode DDATA1LTODATA0 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_DDATA1HTODATA1             0x0000008EUL                                /**< Mode DDATA1HTODATA1 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_DDATA1LTODATA2             0x0000008FUL                                /**< Mode DDATA1LTODATA2 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_DDATA2TODDATA0             0x00000090UL                                /**< Mode DDATA2TODDATA0 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_DDATA2TODDATA1             0x00000091UL                                /**< Mode DDATA2TODDATA1 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_DDATA2TODDATA3             0x00000093UL                                /**< Mode DDATA2TODDATA3 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_DDATA2TODDATA4             0x00000094UL                                /**< Mode DDATA2TODDATA4 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_DDATA2LTODATA2             0x00000097UL                                /**< Mode DDATA2LTODATA2 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_DDATA3TODDATA0             0x00000098UL                                /**< Mode DDATA3TODDATA0 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_DDATA3TODDATA1             0x00000099UL                                /**< Mode DDATA3TODDATA1 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_DDATA3TODDATA2             0x0000009AUL                                /**< Mode DDATA3TODDATA2 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_DDATA3TODDATA4             0x0000009CUL                                /**< Mode DDATA3TODDATA4 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_DDATA3LTODATA0             0x0000009DUL                                /**< Mode DDATA3LTODATA0 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_DDATA3HTODATA1             0x0000009EUL                                /**< Mode DDATA3HTODATA1 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_DDATA4TODDATA0             0x000000A0UL                                /**< Mode DDATA4TODDATA0 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_DDATA4TODDATA1             0x000000A1UL                                /**< Mode DDATA4TODDATA1 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_DDATA4TODDATA2             0x000000A2UL                                /**< Mode DDATA4TODDATA2 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_DDATA4TODDATA3             0x000000A3UL                                /**< Mode DDATA4TODDATA3 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_DDATA4LTODATA0             0x000000A5UL                                /**< Mode DDATA4LTODATA0 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_DDATA4HTODATA1             0x000000A6UL                                /**< Mode DDATA4HTODATA1 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_DDATA4LTODATA2             0x000000A7UL                                /**< Mode DDATA4LTODATA2 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_DATA0TODDATA0              0x000000A8UL                                /**< Mode DATA0TODDATA0 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_DATA0TODDATA1              0x000000A9UL                                /**< Mode DATA0TODDATA1 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_DATA1TODDATA0              0x000000B0UL                                /**< Mode DATA1TODDATA0 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_DATA1TODDATA1              0x000000B1UL                                /**< Mode DATA1TODDATA1 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_DATA2TODDATA0              0x000000B8UL                                /**< Mode DATA2TODDATA0 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_DATA2TODDATA1              0x000000B9UL                                /**< Mode DATA2TODDATA1 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_DATA2TODDATA2              0x000000BAUL                                /**< Mode DATA2TODDATA2 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_SELDDATA0DDATA0            0x000000C0UL                                /**< Mode SELDDATA0DDATA0 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_SELDDATA1DDATA0            0x000000C1UL                                /**< Mode SELDDATA1DDATA0 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_SELDDATA2DDATA0            0x000000C2UL                                /**< Mode SELDDATA2DDATA0 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_SELDDATA3DDATA0            0x000000C3UL                                /**< Mode SELDDATA3DDATA0 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_SELDDATA4DDATA0            0x000000C4UL                                /**< Mode SELDDATA4DDATA0 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_SELDATA0DDATA0             0x000000C5UL                                /**< Mode SELDATA0DDATA0 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_SELDATA1DDATA0             0x000000C6UL                                /**< Mode SELDATA1DDATA0 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_SELDATA2DDATA0             0x000000C7UL                                /**< Mode SELDATA2DDATA0 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_SELDDATA0DDATA1            0x000000C8UL                                /**< Mode SELDDATA0DDATA1 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_SELDDATA1DDATA1            0x000000C9UL                                /**< Mode SELDDATA1DDATA1 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_SELDDATA2DDATA1            0x000000CAUL                                /**< Mode SELDDATA2DDATA1 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_SELDDATA3DDATA1            0x000000CBUL                                /**< Mode SELDDATA3DDATA1 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_SELDDATA4DDATA1            0x000000CCUL                                /**< Mode SELDDATA4DDATA1 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_SELDATA0DDATA1             0x000000CDUL                                /**< Mode SELDATA0DDATA1 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_SELDATA1DDATA1             0x000000CEUL                                /**< Mode SELDATA1DDATA1 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_SELDATA2DDATA1             0x000000CFUL                                /**< Mode SELDATA2DDATA1 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_SELDDATA0DDATA2            0x000000D0UL                                /**< Mode SELDDATA0DDATA2 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_SELDDATA1DDATA2            0x000000D1UL                                /**< Mode SELDDATA1DDATA2 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_SELDDATA2DDATA2            0x000000D2UL                                /**< Mode SELDDATA2DDATA2 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_SELDDATA3DDATA2            0x000000D3UL                                /**< Mode SELDDATA3DDATA2 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_SELDDATA4DDATA2            0x000000D4UL                                /**< Mode SELDDATA4DDATA2 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_SELDATA0DDATA2             0x000000D5UL                                /**< Mode SELDATA0DDATA2 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_SELDATA1DDATA2             0x000000D6UL                                /**< Mode SELDATA1DDATA2 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_SELDATA2DDATA2             0x000000D7UL                                /**< Mode SELDATA2DDATA2 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_SELDDATA0DDATA3            0x000000D8UL                                /**< Mode SELDDATA0DDATA3 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_SELDDATA1DDATA3            0x000000D9UL                                /**< Mode SELDDATA1DDATA3 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_SELDDATA2DDATA3            0x000000DAUL                                /**< Mode SELDDATA2DDATA3 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_SELDDATA3DDATA3            0x000000DBUL                                /**< Mode SELDDATA3DDATA3 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_SELDDATA4DDATA3            0x000000DCUL                                /**< Mode SELDDATA4DDATA3 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_SELDATA0DDATA3             0x000000DDUL                                /**< Mode SELDATA0DDATA3 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_SELDATA1DDATA3             0x000000DEUL                                /**< Mode SELDATA1DDATA3 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_SELDATA2DDATA3             0x000000DFUL                                /**< Mode SELDATA2DDATA3 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_SELDDATA0DDATA4            0x000000E0UL                                /**< Mode SELDDATA0DDATA4 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_SELDDATA1DDATA4            0x000000E1UL                                /**< Mode SELDDATA1DDATA4 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_SELDDATA2DDATA4            0x000000E2UL                                /**< Mode SELDDATA2DDATA4 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_SELDDATA3DDATA4            0x000000E3UL                                /**< Mode SELDDATA3DDATA4 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_SELDDATA4DDATA4            0x000000E4UL                                /**< Mode SELDDATA4DDATA4 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_SELDATA0DDATA4             0x000000E5UL                                /**< Mode SELDATA0DDATA4 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_SELDATA1DDATA4             0x000000E6UL                                /**< Mode SELDATA1DDATA4 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_SELDATA2DDATA4             0x000000E7UL                                /**< Mode SELDATA2DDATA4 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_SELDDATA0DATA0             0x000000E8UL                                /**< Mode SELDDATA0DATA0 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_SELDDATA1DATA0             0x000000E9UL                                /**< Mode SELDDATA1DATA0 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_SELDDATA2DATA0             0x000000EAUL                                /**< Mode SELDDATA2DATA0 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_SELDDATA3DATA0             0x000000EBUL                                /**< Mode SELDDATA3DATA0 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_SELDDATA4DATA0             0x000000ECUL                                /**< Mode SELDDATA4DATA0 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_SELDATA0DATA0              0x000000EDUL                                /**< Mode SELDATA0DATA0 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_SELDATA1DATA0              0x000000EEUL                                /**< Mode SELDATA1DATA0 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_SELDATA2DATA0              0x000000EFUL                                /**< Mode SELDATA2DATA0 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_SELDDATA0DATA1             0x000000F0UL                                /**< Mode SELDDATA0DATA1 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_SELDDATA1DATA1             0x000000F1UL                                /**< Mode SELDDATA1DATA1 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_SELDDATA2DATA1             0x000000F2UL                                /**< Mode SELDDATA2DATA1 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_SELDDATA3DATA1             0x000000F3UL                                /**< Mode SELDDATA3DATA1 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_SELDDATA4DATA1             0x000000F4UL                                /**< Mode SELDDATA4DATA1 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_SELDATA0DATA1              0x000000F5UL                                /**< Mode SELDATA0DATA1 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_SELDATA1DATA1              0x000000F6UL                                /**< Mode SELDATA1DATA1 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_SELDATA2DATA1              0x000000F7UL                                /**< Mode SELDATA2DATA1 for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_EXECIFA                    0x000000F8UL                                /**< Mode EXECIFA for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_EXECIFB                    0x000000F9UL                                /**< Mode EXECIFB for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_EXECIFNLAST                0x000000FAUL                                /**< Mode EXECIFNLAST for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_EXECIFLAST                 0x000000FBUL                                /**< Mode EXECIFLAST for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_EXECIFCARRY                0x000000FCUL                                /**< Mode EXECIFCARRY for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_EXECIFNCARRY               0x000000FDUL                                /**< Mode EXECIFNCARRY for CRYPTO_CMD */
#define _CRYPTO_CMD_INSTR_EXECALWAYS                 0x000000FEUL                                /**< Mode EXECALWAYS for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_DEFAULT                     (_CRYPTO_CMD_INSTR_DEFAULT << 0)            /**< Shifted mode DEFAULT for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_END                         (_CRYPTO_CMD_INSTR_END << 0)                /**< Shifted mode END for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_EXEC                        (_CRYPTO_CMD_INSTR_EXEC << 0)               /**< Shifted mode EXEC for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_DATA1INC                    (_CRYPTO_CMD_INSTR_DATA1INC << 0)           /**< Shifted mode DATA1INC for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_DATA1INCCLR                 (_CRYPTO_CMD_INSTR_DATA1INCCLR << 0)        /**< Shifted mode DATA1INCCLR for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_AESENC                      (_CRYPTO_CMD_INSTR_AESENC << 0)             /**< Shifted mode AESENC for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_AESDEC                      (_CRYPTO_CMD_INSTR_AESDEC << 0)             /**< Shifted mode AESDEC for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_SHA                         (_CRYPTO_CMD_INSTR_SHA << 0)                /**< Shifted mode SHA for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_ADD                         (_CRYPTO_CMD_INSTR_ADD << 0)                /**< Shifted mode ADD for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_ADDC                        (_CRYPTO_CMD_INSTR_ADDC << 0)               /**< Shifted mode ADDC for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_MADD                        (_CRYPTO_CMD_INSTR_MADD << 0)               /**< Shifted mode MADD for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_MADD32                      (_CRYPTO_CMD_INSTR_MADD32 << 0)             /**< Shifted mode MADD32 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_SUB                         (_CRYPTO_CMD_INSTR_SUB << 0)                /**< Shifted mode SUB for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_SUBC                        (_CRYPTO_CMD_INSTR_SUBC << 0)               /**< Shifted mode SUBC for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_MSUB                        (_CRYPTO_CMD_INSTR_MSUB << 0)               /**< Shifted mode MSUB for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_MUL                         (_CRYPTO_CMD_INSTR_MUL << 0)                /**< Shifted mode MUL for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_MULC                        (_CRYPTO_CMD_INSTR_MULC << 0)               /**< Shifted mode MULC for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_MMUL                        (_CRYPTO_CMD_INSTR_MMUL << 0)               /**< Shifted mode MMUL for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_MULO                        (_CRYPTO_CMD_INSTR_MULO << 0)               /**< Shifted mode MULO for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_SHL                         (_CRYPTO_CMD_INSTR_SHL << 0)                /**< Shifted mode SHL for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_SHLC                        (_CRYPTO_CMD_INSTR_SHLC << 0)               /**< Shifted mode SHLC for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_SHLB                        (_CRYPTO_CMD_INSTR_SHLB << 0)               /**< Shifted mode SHLB for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_SHL1                        (_CRYPTO_CMD_INSTR_SHL1 << 0)               /**< Shifted mode SHL1 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_SHR                         (_CRYPTO_CMD_INSTR_SHR << 0)                /**< Shifted mode SHR for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_SHRC                        (_CRYPTO_CMD_INSTR_SHRC << 0)               /**< Shifted mode SHRC for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_SHRB                        (_CRYPTO_CMD_INSTR_SHRB << 0)               /**< Shifted mode SHRB for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_SHR1                        (_CRYPTO_CMD_INSTR_SHR1 << 0)               /**< Shifted mode SHR1 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_ADDO                        (_CRYPTO_CMD_INSTR_ADDO << 0)               /**< Shifted mode ADDO for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_ADDIC                       (_CRYPTO_CMD_INSTR_ADDIC << 0)              /**< Shifted mode ADDIC for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_CLR                         (_CRYPTO_CMD_INSTR_CLR << 0)                /**< Shifted mode CLR for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_XOR                         (_CRYPTO_CMD_INSTR_XOR << 0)                /**< Shifted mode XOR for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_INV                         (_CRYPTO_CMD_INSTR_INV << 0)                /**< Shifted mode INV for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_CSET                        (_CRYPTO_CMD_INSTR_CSET << 0)               /**< Shifted mode CSET for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_CCLR                        (_CRYPTO_CMD_INSTR_CCLR << 0)               /**< Shifted mode CCLR for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_BBSWAP128                   (_CRYPTO_CMD_INSTR_BBSWAP128 << 0)          /**< Shifted mode BBSWAP128 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_INC                         (_CRYPTO_CMD_INSTR_INC << 0)                /**< Shifted mode INC for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_DEC                         (_CRYPTO_CMD_INSTR_DEC << 0)                /**< Shifted mode DEC for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_SHRA                        (_CRYPTO_CMD_INSTR_SHRA << 0)               /**< Shifted mode SHRA for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_DATA0TODATA0                (_CRYPTO_CMD_INSTR_DATA0TODATA0 << 0)       /**< Shifted mode DATA0TODATA0 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_DATA0TODATA0XOR             (_CRYPTO_CMD_INSTR_DATA0TODATA0XOR << 0)    /**< Shifted mode DATA0TODATA0XOR for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_DATA0TODATA0XORLEN          (_CRYPTO_CMD_INSTR_DATA0TODATA0XORLEN << 0) /**< Shifted mode DATA0TODATA0XORLEN for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_DATA0TODATA1                (_CRYPTO_CMD_INSTR_DATA0TODATA1 << 0)       /**< Shifted mode DATA0TODATA1 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_DATA0TODATA2                (_CRYPTO_CMD_INSTR_DATA0TODATA2 << 0)       /**< Shifted mode DATA0TODATA2 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_DATA0TODATA3                (_CRYPTO_CMD_INSTR_DATA0TODATA3 << 0)       /**< Shifted mode DATA0TODATA3 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_DATA1TODATA0                (_CRYPTO_CMD_INSTR_DATA1TODATA0 << 0)       /**< Shifted mode DATA1TODATA0 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_DATA1TODATA0XOR             (_CRYPTO_CMD_INSTR_DATA1TODATA0XOR << 0)    /**< Shifted mode DATA1TODATA0XOR for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_DATA1TODATA0XORLEN          (_CRYPTO_CMD_INSTR_DATA1TODATA0XORLEN << 0) /**< Shifted mode DATA1TODATA0XORLEN for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_DATA1TODATA2                (_CRYPTO_CMD_INSTR_DATA1TODATA2 << 0)       /**< Shifted mode DATA1TODATA2 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_DATA1TODATA3                (_CRYPTO_CMD_INSTR_DATA1TODATA3 << 0)       /**< Shifted mode DATA1TODATA3 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_DATA2TODATA0                (_CRYPTO_CMD_INSTR_DATA2TODATA0 << 0)       /**< Shifted mode DATA2TODATA0 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_DATA2TODATA0XOR             (_CRYPTO_CMD_INSTR_DATA2TODATA0XOR << 0)    /**< Shifted mode DATA2TODATA0XOR for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_DATA2TODATA0XORLEN          (_CRYPTO_CMD_INSTR_DATA2TODATA0XORLEN << 0) /**< Shifted mode DATA2TODATA0XORLEN for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_DATA2TODATA1                (_CRYPTO_CMD_INSTR_DATA2TODATA1 << 0)       /**< Shifted mode DATA2TODATA1 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_DATA2TODATA3                (_CRYPTO_CMD_INSTR_DATA2TODATA3 << 0)       /**< Shifted mode DATA2TODATA3 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_DATA3TODATA0                (_CRYPTO_CMD_INSTR_DATA3TODATA0 << 0)       /**< Shifted mode DATA3TODATA0 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_DATA3TODATA0XOR             (_CRYPTO_CMD_INSTR_DATA3TODATA0XOR << 0)    /**< Shifted mode DATA3TODATA0XOR for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_DATA3TODATA0XORLEN          (_CRYPTO_CMD_INSTR_DATA3TODATA0XORLEN << 0) /**< Shifted mode DATA3TODATA0XORLEN for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_DATA3TODATA1                (_CRYPTO_CMD_INSTR_DATA3TODATA1 << 0)       /**< Shifted mode DATA3TODATA1 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_DATA3TODATA2                (_CRYPTO_CMD_INSTR_DATA3TODATA2 << 0)       /**< Shifted mode DATA3TODATA2 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_DATATODMA0                  (_CRYPTO_CMD_INSTR_DATATODMA0 << 0)         /**< Shifted mode DATATODMA0 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_DATA0TOBUF                  (_CRYPTO_CMD_INSTR_DATA0TOBUF << 0)         /**< Shifted mode DATA0TOBUF for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_DATA0TOBUFXOR               (_CRYPTO_CMD_INSTR_DATA0TOBUFXOR << 0)      /**< Shifted mode DATA0TOBUFXOR for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_DATATODMA1                  (_CRYPTO_CMD_INSTR_DATATODMA1 << 0)         /**< Shifted mode DATATODMA1 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_DATA1TOBUF                  (_CRYPTO_CMD_INSTR_DATA1TOBUF << 0)         /**< Shifted mode DATA1TOBUF for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_DATA1TOBUFXOR               (_CRYPTO_CMD_INSTR_DATA1TOBUFXOR << 0)      /**< Shifted mode DATA1TOBUFXOR for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_DMA0TODATA                  (_CRYPTO_CMD_INSTR_DMA0TODATA << 0)         /**< Shifted mode DMA0TODATA for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_DMA0TODATAXOR               (_CRYPTO_CMD_INSTR_DMA0TODATAXOR << 0)      /**< Shifted mode DMA0TODATAXOR for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_DMA1TODATA                  (_CRYPTO_CMD_INSTR_DMA1TODATA << 0)         /**< Shifted mode DMA1TODATA for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_BUFTODATA0                  (_CRYPTO_CMD_INSTR_BUFTODATA0 << 0)         /**< Shifted mode BUFTODATA0 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_BUFTODATA0XOR               (_CRYPTO_CMD_INSTR_BUFTODATA0XOR << 0)      /**< Shifted mode BUFTODATA0XOR for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_BUFTODATA1                  (_CRYPTO_CMD_INSTR_BUFTODATA1 << 0)         /**< Shifted mode BUFTODATA1 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_DDATA0TODDATA1              (_CRYPTO_CMD_INSTR_DDATA0TODDATA1 << 0)     /**< Shifted mode DDATA0TODDATA1 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_DDATA0TODDATA2              (_CRYPTO_CMD_INSTR_DDATA0TODDATA2 << 0)     /**< Shifted mode DDATA0TODDATA2 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_DDATA0TODDATA3              (_CRYPTO_CMD_INSTR_DDATA0TODDATA3 << 0)     /**< Shifted mode DDATA0TODDATA3 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_DDATA0TODDATA4              (_CRYPTO_CMD_INSTR_DDATA0TODDATA4 << 0)     /**< Shifted mode DDATA0TODDATA4 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_DDATA0LTODATA0              (_CRYPTO_CMD_INSTR_DDATA0LTODATA0 << 0)     /**< Shifted mode DDATA0LTODATA0 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_DDATA0HTODATA1              (_CRYPTO_CMD_INSTR_DDATA0HTODATA1 << 0)     /**< Shifted mode DDATA0HTODATA1 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_DDATA0LTODATA2              (_CRYPTO_CMD_INSTR_DDATA0LTODATA2 << 0)     /**< Shifted mode DDATA0LTODATA2 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_DDATA1TODDATA0              (_CRYPTO_CMD_INSTR_DDATA1TODDATA0 << 0)     /**< Shifted mode DDATA1TODDATA0 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_DDATA1TODDATA2              (_CRYPTO_CMD_INSTR_DDATA1TODDATA2 << 0)     /**< Shifted mode DDATA1TODDATA2 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_DDATA1TODDATA3              (_CRYPTO_CMD_INSTR_DDATA1TODDATA3 << 0)     /**< Shifted mode DDATA1TODDATA3 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_DDATA1TODDATA4              (_CRYPTO_CMD_INSTR_DDATA1TODDATA4 << 0)     /**< Shifted mode DDATA1TODDATA4 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_DDATA1LTODATA0              (_CRYPTO_CMD_INSTR_DDATA1LTODATA0 << 0)     /**< Shifted mode DDATA1LTODATA0 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_DDATA1HTODATA1              (_CRYPTO_CMD_INSTR_DDATA1HTODATA1 << 0)     /**< Shifted mode DDATA1HTODATA1 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_DDATA1LTODATA2              (_CRYPTO_CMD_INSTR_DDATA1LTODATA2 << 0)     /**< Shifted mode DDATA1LTODATA2 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_DDATA2TODDATA0              (_CRYPTO_CMD_INSTR_DDATA2TODDATA0 << 0)     /**< Shifted mode DDATA2TODDATA0 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_DDATA2TODDATA1              (_CRYPTO_CMD_INSTR_DDATA2TODDATA1 << 0)     /**< Shifted mode DDATA2TODDATA1 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_DDATA2TODDATA3              (_CRYPTO_CMD_INSTR_DDATA2TODDATA3 << 0)     /**< Shifted mode DDATA2TODDATA3 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_DDATA2TODDATA4              (_CRYPTO_CMD_INSTR_DDATA2TODDATA4 << 0)     /**< Shifted mode DDATA2TODDATA4 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_DDATA2LTODATA2              (_CRYPTO_CMD_INSTR_DDATA2LTODATA2 << 0)     /**< Shifted mode DDATA2LTODATA2 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_DDATA3TODDATA0              (_CRYPTO_CMD_INSTR_DDATA3TODDATA0 << 0)     /**< Shifted mode DDATA3TODDATA0 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_DDATA3TODDATA1              (_CRYPTO_CMD_INSTR_DDATA3TODDATA1 << 0)     /**< Shifted mode DDATA3TODDATA1 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_DDATA3TODDATA2              (_CRYPTO_CMD_INSTR_DDATA3TODDATA2 << 0)     /**< Shifted mode DDATA3TODDATA2 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_DDATA3TODDATA4              (_CRYPTO_CMD_INSTR_DDATA3TODDATA4 << 0)     /**< Shifted mode DDATA3TODDATA4 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_DDATA3LTODATA0              (_CRYPTO_CMD_INSTR_DDATA3LTODATA0 << 0)     /**< Shifted mode DDATA3LTODATA0 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_DDATA3HTODATA1              (_CRYPTO_CMD_INSTR_DDATA3HTODATA1 << 0)     /**< Shifted mode DDATA3HTODATA1 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_DDATA4TODDATA0              (_CRYPTO_CMD_INSTR_DDATA4TODDATA0 << 0)     /**< Shifted mode DDATA4TODDATA0 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_DDATA4TODDATA1              (_CRYPTO_CMD_INSTR_DDATA4TODDATA1 << 0)     /**< Shifted mode DDATA4TODDATA1 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_DDATA4TODDATA2              (_CRYPTO_CMD_INSTR_DDATA4TODDATA2 << 0)     /**< Shifted mode DDATA4TODDATA2 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_DDATA4TODDATA3              (_CRYPTO_CMD_INSTR_DDATA4TODDATA3 << 0)     /**< Shifted mode DDATA4TODDATA3 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_DDATA4LTODATA0              (_CRYPTO_CMD_INSTR_DDATA4LTODATA0 << 0)     /**< Shifted mode DDATA4LTODATA0 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_DDATA4HTODATA1              (_CRYPTO_CMD_INSTR_DDATA4HTODATA1 << 0)     /**< Shifted mode DDATA4HTODATA1 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_DDATA4LTODATA2              (_CRYPTO_CMD_INSTR_DDATA4LTODATA2 << 0)     /**< Shifted mode DDATA4LTODATA2 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_DATA0TODDATA0               (_CRYPTO_CMD_INSTR_DATA0TODDATA0 << 0)      /**< Shifted mode DATA0TODDATA0 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_DATA0TODDATA1               (_CRYPTO_CMD_INSTR_DATA0TODDATA1 << 0)      /**< Shifted mode DATA0TODDATA1 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_DATA1TODDATA0               (_CRYPTO_CMD_INSTR_DATA1TODDATA0 << 0)      /**< Shifted mode DATA1TODDATA0 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_DATA1TODDATA1               (_CRYPTO_CMD_INSTR_DATA1TODDATA1 << 0)      /**< Shifted mode DATA1TODDATA1 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_DATA2TODDATA0               (_CRYPTO_CMD_INSTR_DATA2TODDATA0 << 0)      /**< Shifted mode DATA2TODDATA0 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_DATA2TODDATA1               (_CRYPTO_CMD_INSTR_DATA2TODDATA1 << 0)      /**< Shifted mode DATA2TODDATA1 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_DATA2TODDATA2               (_CRYPTO_CMD_INSTR_DATA2TODDATA2 << 0)      /**< Shifted mode DATA2TODDATA2 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_SELDDATA0DDATA0             (_CRYPTO_CMD_INSTR_SELDDATA0DDATA0 << 0)    /**< Shifted mode SELDDATA0DDATA0 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_SELDDATA1DDATA0             (_CRYPTO_CMD_INSTR_SELDDATA1DDATA0 << 0)    /**< Shifted mode SELDDATA1DDATA0 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_SELDDATA2DDATA0             (_CRYPTO_CMD_INSTR_SELDDATA2DDATA0 << 0)    /**< Shifted mode SELDDATA2DDATA0 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_SELDDATA3DDATA0             (_CRYPTO_CMD_INSTR_SELDDATA3DDATA0 << 0)    /**< Shifted mode SELDDATA3DDATA0 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_SELDDATA4DDATA0             (_CRYPTO_CMD_INSTR_SELDDATA4DDATA0 << 0)    /**< Shifted mode SELDDATA4DDATA0 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_SELDATA0DDATA0              (_CRYPTO_CMD_INSTR_SELDATA0DDATA0 << 0)     /**< Shifted mode SELDATA0DDATA0 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_SELDATA1DDATA0              (_CRYPTO_CMD_INSTR_SELDATA1DDATA0 << 0)     /**< Shifted mode SELDATA1DDATA0 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_SELDATA2DDATA0              (_CRYPTO_CMD_INSTR_SELDATA2DDATA0 << 0)     /**< Shifted mode SELDATA2DDATA0 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_SELDDATA0DDATA1             (_CRYPTO_CMD_INSTR_SELDDATA0DDATA1 << 0)    /**< Shifted mode SELDDATA0DDATA1 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_SELDDATA1DDATA1             (_CRYPTO_CMD_INSTR_SELDDATA1DDATA1 << 0)    /**< Shifted mode SELDDATA1DDATA1 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_SELDDATA2DDATA1             (_CRYPTO_CMD_INSTR_SELDDATA2DDATA1 << 0)    /**< Shifted mode SELDDATA2DDATA1 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_SELDDATA3DDATA1             (_CRYPTO_CMD_INSTR_SELDDATA3DDATA1 << 0)    /**< Shifted mode SELDDATA3DDATA1 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_SELDDATA4DDATA1             (_CRYPTO_CMD_INSTR_SELDDATA4DDATA1 << 0)    /**< Shifted mode SELDDATA4DDATA1 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_SELDATA0DDATA1              (_CRYPTO_CMD_INSTR_SELDATA0DDATA1 << 0)     /**< Shifted mode SELDATA0DDATA1 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_SELDATA1DDATA1              (_CRYPTO_CMD_INSTR_SELDATA1DDATA1 << 0)     /**< Shifted mode SELDATA1DDATA1 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_SELDATA2DDATA1              (_CRYPTO_CMD_INSTR_SELDATA2DDATA1 << 0)     /**< Shifted mode SELDATA2DDATA1 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_SELDDATA0DDATA2             (_CRYPTO_CMD_INSTR_SELDDATA0DDATA2 << 0)    /**< Shifted mode SELDDATA0DDATA2 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_SELDDATA1DDATA2             (_CRYPTO_CMD_INSTR_SELDDATA1DDATA2 << 0)    /**< Shifted mode SELDDATA1DDATA2 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_SELDDATA2DDATA2             (_CRYPTO_CMD_INSTR_SELDDATA2DDATA2 << 0)    /**< Shifted mode SELDDATA2DDATA2 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_SELDDATA3DDATA2             (_CRYPTO_CMD_INSTR_SELDDATA3DDATA2 << 0)    /**< Shifted mode SELDDATA3DDATA2 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_SELDDATA4DDATA2             (_CRYPTO_CMD_INSTR_SELDDATA4DDATA2 << 0)    /**< Shifted mode SELDDATA4DDATA2 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_SELDATA0DDATA2              (_CRYPTO_CMD_INSTR_SELDATA0DDATA2 << 0)     /**< Shifted mode SELDATA0DDATA2 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_SELDATA1DDATA2              (_CRYPTO_CMD_INSTR_SELDATA1DDATA2 << 0)     /**< Shifted mode SELDATA1DDATA2 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_SELDATA2DDATA2              (_CRYPTO_CMD_INSTR_SELDATA2DDATA2 << 0)     /**< Shifted mode SELDATA2DDATA2 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_SELDDATA0DDATA3             (_CRYPTO_CMD_INSTR_SELDDATA0DDATA3 << 0)    /**< Shifted mode SELDDATA0DDATA3 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_SELDDATA1DDATA3             (_CRYPTO_CMD_INSTR_SELDDATA1DDATA3 << 0)    /**< Shifted mode SELDDATA1DDATA3 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_SELDDATA2DDATA3             (_CRYPTO_CMD_INSTR_SELDDATA2DDATA3 << 0)    /**< Shifted mode SELDDATA2DDATA3 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_SELDDATA3DDATA3             (_CRYPTO_CMD_INSTR_SELDDATA3DDATA3 << 0)    /**< Shifted mode SELDDATA3DDATA3 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_SELDDATA4DDATA3             (_CRYPTO_CMD_INSTR_SELDDATA4DDATA3 << 0)    /**< Shifted mode SELDDATA4DDATA3 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_SELDATA0DDATA3              (_CRYPTO_CMD_INSTR_SELDATA0DDATA3 << 0)     /**< Shifted mode SELDATA0DDATA3 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_SELDATA1DDATA3              (_CRYPTO_CMD_INSTR_SELDATA1DDATA3 << 0)     /**< Shifted mode SELDATA1DDATA3 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_SELDATA2DDATA3              (_CRYPTO_CMD_INSTR_SELDATA2DDATA3 << 0)     /**< Shifted mode SELDATA2DDATA3 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_SELDDATA0DDATA4             (_CRYPTO_CMD_INSTR_SELDDATA0DDATA4 << 0)    /**< Shifted mode SELDDATA0DDATA4 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_SELDDATA1DDATA4             (_CRYPTO_CMD_INSTR_SELDDATA1DDATA4 << 0)    /**< Shifted mode SELDDATA1DDATA4 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_SELDDATA2DDATA4             (_CRYPTO_CMD_INSTR_SELDDATA2DDATA4 << 0)    /**< Shifted mode SELDDATA2DDATA4 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_SELDDATA3DDATA4             (_CRYPTO_CMD_INSTR_SELDDATA3DDATA4 << 0)    /**< Shifted mode SELDDATA3DDATA4 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_SELDDATA4DDATA4             (_CRYPTO_CMD_INSTR_SELDDATA4DDATA4 << 0)    /**< Shifted mode SELDDATA4DDATA4 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_SELDATA0DDATA4              (_CRYPTO_CMD_INSTR_SELDATA0DDATA4 << 0)     /**< Shifted mode SELDATA0DDATA4 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_SELDATA1DDATA4              (_CRYPTO_CMD_INSTR_SELDATA1DDATA4 << 0)     /**< Shifted mode SELDATA1DDATA4 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_SELDATA2DDATA4              (_CRYPTO_CMD_INSTR_SELDATA2DDATA4 << 0)     /**< Shifted mode SELDATA2DDATA4 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_SELDDATA0DATA0              (_CRYPTO_CMD_INSTR_SELDDATA0DATA0 << 0)     /**< Shifted mode SELDDATA0DATA0 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_SELDDATA1DATA0              (_CRYPTO_CMD_INSTR_SELDDATA1DATA0 << 0)     /**< Shifted mode SELDDATA1DATA0 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_SELDDATA2DATA0              (_CRYPTO_CMD_INSTR_SELDDATA2DATA0 << 0)     /**< Shifted mode SELDDATA2DATA0 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_SELDDATA3DATA0              (_CRYPTO_CMD_INSTR_SELDDATA3DATA0 << 0)     /**< Shifted mode SELDDATA3DATA0 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_SELDDATA4DATA0              (_CRYPTO_CMD_INSTR_SELDDATA4DATA0 << 0)     /**< Shifted mode SELDDATA4DATA0 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_SELDATA0DATA0               (_CRYPTO_CMD_INSTR_SELDATA0DATA0 << 0)      /**< Shifted mode SELDATA0DATA0 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_SELDATA1DATA0               (_CRYPTO_CMD_INSTR_SELDATA1DATA0 << 0)      /**< Shifted mode SELDATA1DATA0 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_SELDATA2DATA0               (_CRYPTO_CMD_INSTR_SELDATA2DATA0 << 0)      /**< Shifted mode SELDATA2DATA0 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_SELDDATA0DATA1              (_CRYPTO_CMD_INSTR_SELDDATA0DATA1 << 0)     /**< Shifted mode SELDDATA0DATA1 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_SELDDATA1DATA1              (_CRYPTO_CMD_INSTR_SELDDATA1DATA1 << 0)     /**< Shifted mode SELDDATA1DATA1 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_SELDDATA2DATA1              (_CRYPTO_CMD_INSTR_SELDDATA2DATA1 << 0)     /**< Shifted mode SELDDATA2DATA1 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_SELDDATA3DATA1              (_CRYPTO_CMD_INSTR_SELDDATA3DATA1 << 0)     /**< Shifted mode SELDDATA3DATA1 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_SELDDATA4DATA1              (_CRYPTO_CMD_INSTR_SELDDATA4DATA1 << 0)     /**< Shifted mode SELDDATA4DATA1 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_SELDATA0DATA1               (_CRYPTO_CMD_INSTR_SELDATA0DATA1 << 0)      /**< Shifted mode SELDATA0DATA1 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_SELDATA1DATA1               (_CRYPTO_CMD_INSTR_SELDATA1DATA1 << 0)      /**< Shifted mode SELDATA1DATA1 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_SELDATA2DATA1               (_CRYPTO_CMD_INSTR_SELDATA2DATA1 << 0)      /**< Shifted mode SELDATA2DATA1 for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_EXECIFA                     (_CRYPTO_CMD_INSTR_EXECIFA << 0)            /**< Shifted mode EXECIFA for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_EXECIFB                     (_CRYPTO_CMD_INSTR_EXECIFB << 0)            /**< Shifted mode EXECIFB for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_EXECIFNLAST                 (_CRYPTO_CMD_INSTR_EXECIFNLAST << 0)        /**< Shifted mode EXECIFNLAST for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_EXECIFLAST                  (_CRYPTO_CMD_INSTR_EXECIFLAST << 0)         /**< Shifted mode EXECIFLAST for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_EXECIFCARRY                 (_CRYPTO_CMD_INSTR_EXECIFCARRY << 0)        /**< Shifted mode EXECIFCARRY for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_EXECIFNCARRY                (_CRYPTO_CMD_INSTR_EXECIFNCARRY << 0)       /**< Shifted mode EXECIFNCARRY for CRYPTO_CMD */
#define CRYPTO_CMD_INSTR_EXECALWAYS                  (_CRYPTO_CMD_INSTR_EXECALWAYS << 0)         /**< Shifted mode EXECALWAYS for CRYPTO_CMD */
#define CRYPTO_CMD_SEQSTART                          (0x1UL << 9)                                /**< Encryption/Decryption SEQUENCE Start */
#define _CRYPTO_CMD_SEQSTART_SHIFT                   9                                           /**< Shift value for CRYPTO_SEQSTART */
#define _CRYPTO_CMD_SEQSTART_MASK                    0x200UL                                     /**< Bit mask for CRYPTO_SEQSTART */
#define _CRYPTO_CMD_SEQSTART_DEFAULT                 0x00000000UL                                /**< Mode DEFAULT for CRYPTO_CMD */
#define CRYPTO_CMD_SEQSTART_DEFAULT                  (_CRYPTO_CMD_SEQSTART_DEFAULT << 9)         /**< Shifted mode DEFAULT for CRYPTO_CMD */
#define CRYPTO_CMD_SEQSTOP                           (0x1UL << 10)                               /**< Sequence Stop */
#define _CRYPTO_CMD_SEQSTOP_SHIFT                    10                                          /**< Shift value for CRYPTO_SEQSTOP */
#define _CRYPTO_CMD_SEQSTOP_MASK                     0x400UL                                     /**< Bit mask for CRYPTO_SEQSTOP */
#define _CRYPTO_CMD_SEQSTOP_DEFAULT                  0x00000000UL                                /**< Mode DEFAULT for CRYPTO_CMD */
#define CRYPTO_CMD_SEQSTOP_DEFAULT                   (_CRYPTO_CMD_SEQSTOP_DEFAULT << 10)         /**< Shifted mode DEFAULT for CRYPTO_CMD */
#define CRYPTO_CMD_SEQSTEP                           (0x1UL << 11)                               /**< Sequence Step */
#define _CRYPTO_CMD_SEQSTEP_SHIFT                    11                                          /**< Shift value for CRYPTO_SEQSTEP */
#define _CRYPTO_CMD_SEQSTEP_MASK                     0x800UL                                     /**< Bit mask for CRYPTO_SEQSTEP */
#define _CRYPTO_CMD_SEQSTEP_DEFAULT                  0x00000000UL                                /**< Mode DEFAULT for CRYPTO_CMD */
#define CRYPTO_CMD_SEQSTEP_DEFAULT                   (_CRYPTO_CMD_SEQSTEP_DEFAULT << 11)         /**< Shifted mode DEFAULT for CRYPTO_CMD */

/* Bit fields for CRYPTO STATUS */
#define _CRYPTO_STATUS_RESETVALUE                    0x00000000UL                               /**< Default value for CRYPTO_STATUS */
#define _CRYPTO_STATUS_MASK                          0x00000007UL                               /**< Mask for CRYPTO_STATUS */
#define CRYPTO_STATUS_SEQRUNNING                     (0x1UL << 0)                               /**< AES SEQUENCE Running */
#define _CRYPTO_STATUS_SEQRUNNING_SHIFT              0                                          /**< Shift value for CRYPTO_SEQRUNNING */
#define _CRYPTO_STATUS_SEQRUNNING_MASK               0x1UL                                      /**< Bit mask for CRYPTO_SEQRUNNING */
#define _CRYPTO_STATUS_SEQRUNNING_DEFAULT            0x00000000UL                               /**< Mode DEFAULT for CRYPTO_STATUS */
#define CRYPTO_STATUS_SEQRUNNING_DEFAULT             (_CRYPTO_STATUS_SEQRUNNING_DEFAULT << 0)   /**< Shifted mode DEFAULT for CRYPTO_STATUS */
#define CRYPTO_STATUS_INSTRRUNNING                   (0x1UL << 1)                               /**< Action is active */
#define _CRYPTO_STATUS_INSTRRUNNING_SHIFT            1                                          /**< Shift value for CRYPTO_INSTRRUNNING */
#define _CRYPTO_STATUS_INSTRRUNNING_MASK             0x2UL                                      /**< Bit mask for CRYPTO_INSTRRUNNING */
#define _CRYPTO_STATUS_INSTRRUNNING_DEFAULT          0x00000000UL                               /**< Mode DEFAULT for CRYPTO_STATUS */
#define CRYPTO_STATUS_INSTRRUNNING_DEFAULT           (_CRYPTO_STATUS_INSTRRUNNING_DEFAULT << 1) /**< Shifted mode DEFAULT for CRYPTO_STATUS */
#define CRYPTO_STATUS_DMAACTIVE                      (0x1UL << 2)                               /**< DMA Action is active */
#define _CRYPTO_STATUS_DMAACTIVE_SHIFT               2                                          /**< Shift value for CRYPTO_DMAACTIVE */
#define _CRYPTO_STATUS_DMAACTIVE_MASK                0x4UL                                      /**< Bit mask for CRYPTO_DMAACTIVE */
#define _CRYPTO_STATUS_DMAACTIVE_DEFAULT             0x00000000UL                               /**< Mode DEFAULT for CRYPTO_STATUS */
#define CRYPTO_STATUS_DMAACTIVE_DEFAULT              (_CRYPTO_STATUS_DMAACTIVE_DEFAULT << 2)    /**< Shifted mode DEFAULT for CRYPTO_STATUS */

/* Bit fields for CRYPTO DSTATUS */
#define _CRYPTO_DSTATUS_RESETVALUE                   0x00000000UL                                 /**< Default value for CRYPTO_DSTATUS */
#define _CRYPTO_DSTATUS_MASK                         0x011F0F0FUL                                 /**< Mask for CRYPTO_DSTATUS */
#define _CRYPTO_DSTATUS_DATA0ZERO_SHIFT              0                                            /**< Shift value for CRYPTO_DATA0ZERO */
#define _CRYPTO_DSTATUS_DATA0ZERO_MASK               0xFUL                                        /**< Bit mask for CRYPTO_DATA0ZERO */
#define _CRYPTO_DSTATUS_DATA0ZERO_DEFAULT            0x00000000UL                                 /**< Mode DEFAULT for CRYPTO_DSTATUS */
#define _CRYPTO_DSTATUS_DATA0ZERO_ZERO0TO31          0x00000001UL                                 /**< Mode ZERO0TO31 for CRYPTO_DSTATUS */
#define _CRYPTO_DSTATUS_DATA0ZERO_ZERO32TO63         0x00000002UL                                 /**< Mode ZERO32TO63 for CRYPTO_DSTATUS */
#define _CRYPTO_DSTATUS_DATA0ZERO_ZERO64TO95         0x00000004UL                                 /**< Mode ZERO64TO95 for CRYPTO_DSTATUS */
#define _CRYPTO_DSTATUS_DATA0ZERO_ZERO96TO127        0x00000008UL                                 /**< Mode ZERO96TO127 for CRYPTO_DSTATUS */
#define CRYPTO_DSTATUS_DATA0ZERO_DEFAULT             (_CRYPTO_DSTATUS_DATA0ZERO_DEFAULT << 0)     /**< Shifted mode DEFAULT for CRYPTO_DSTATUS */
#define CRYPTO_DSTATUS_DATA0ZERO_ZERO0TO31           (_CRYPTO_DSTATUS_DATA0ZERO_ZERO0TO31 << 0)   /**< Shifted mode ZERO0TO31 for CRYPTO_DSTATUS */
#define CRYPTO_DSTATUS_DATA0ZERO_ZERO32TO63          (_CRYPTO_DSTATUS_DATA0ZERO_ZERO32TO63 << 0)  /**< Shifted mode ZERO32TO63 for CRYPTO_DSTATUS */
#define CRYPTO_DSTATUS_DATA0ZERO_ZERO64TO95          (_CRYPTO_DSTATUS_DATA0ZERO_ZERO64TO95 << 0)  /**< Shifted mode ZERO64TO95 for CRYPTO_DSTATUS */
#define CRYPTO_DSTATUS_DATA0ZERO_ZERO96TO127         (_CRYPTO_DSTATUS_DATA0ZERO_ZERO96TO127 << 0) /**< Shifted mode ZERO96TO127 for CRYPTO_DSTATUS */
#define _CRYPTO_DSTATUS_DDATA0LSBS_SHIFT             8                                            /**< Shift value for CRYPTO_DDATA0LSBS */
#define _CRYPTO_DSTATUS_DDATA0LSBS_MASK              0xF00UL                                      /**< Bit mask for CRYPTO_DDATA0LSBS */
#define _CRYPTO_DSTATUS_DDATA0LSBS_DEFAULT           0x00000000UL                                 /**< Mode DEFAULT for CRYPTO_DSTATUS */
#define CRYPTO_DSTATUS_DDATA0LSBS_DEFAULT            (_CRYPTO_DSTATUS_DDATA0LSBS_DEFAULT << 8)    /**< Shifted mode DEFAULT for CRYPTO_DSTATUS */
#define _CRYPTO_DSTATUS_DDATA0MSBS_SHIFT             16                                           /**< Shift value for CRYPTO_DDATA0MSBS */
#define _CRYPTO_DSTATUS_DDATA0MSBS_MASK              0xF0000UL                                    /**< Bit mask for CRYPTO_DDATA0MSBS */
#define _CRYPTO_DSTATUS_DDATA0MSBS_DEFAULT           0x00000000UL                                 /**< Mode DEFAULT for CRYPTO_DSTATUS */
#define CRYPTO_DSTATUS_DDATA0MSBS_DEFAULT            (_CRYPTO_DSTATUS_DDATA0MSBS_DEFAULT << 16)   /**< Shifted mode DEFAULT for CRYPTO_DSTATUS */
#define CRYPTO_DSTATUS_DDATA1MSB                     (0x1UL << 20)                                /**< MSB in DDATA1 */
#define _CRYPTO_DSTATUS_DDATA1MSB_SHIFT              20                                           /**< Shift value for CRYPTO_DDATA1MSB */
#define _CRYPTO_DSTATUS_DDATA1MSB_MASK               0x100000UL                                   /**< Bit mask for CRYPTO_DDATA1MSB */
#define _CRYPTO_DSTATUS_DDATA1MSB_DEFAULT            0x00000000UL                                 /**< Mode DEFAULT for CRYPTO_DSTATUS */
#define CRYPTO_DSTATUS_DDATA1MSB_DEFAULT             (_CRYPTO_DSTATUS_DDATA1MSB_DEFAULT << 20)    /**< Shifted mode DEFAULT for CRYPTO_DSTATUS */
#define CRYPTO_DSTATUS_CARRY                         (0x1UL << 24)                                /**< Carry From Arithmetic Operation */
#define _CRYPTO_DSTATUS_CARRY_SHIFT                  24                                           /**< Shift value for CRYPTO_CARRY */
#define _CRYPTO_DSTATUS_CARRY_MASK                   0x1000000UL                                  /**< Bit mask for CRYPTO_CARRY */
#define _CRYPTO_DSTATUS_CARRY_DEFAULT                0x00000000UL                                 /**< Mode DEFAULT for CRYPTO_DSTATUS */
#define CRYPTO_DSTATUS_CARRY_DEFAULT                 (_CRYPTO_DSTATUS_CARRY_DEFAULT << 24)        /**< Shifted mode DEFAULT for CRYPTO_DSTATUS */

/* Bit fields for CRYPTO CSTATUS */
#define _CRYPTO_CSTATUS_RESETVALUE                   0x00000201UL                            /**< Default value for CRYPTO_CSTATUS */
#define _CRYPTO_CSTATUS_MASK                         0x01F30707UL                            /**< Mask for CRYPTO_CSTATUS */
#define _CRYPTO_CSTATUS_V0_SHIFT                     0                                       /**< Shift value for CRYPTO_V0 */
#define _CRYPTO_CSTATUS_V0_MASK                      0x7UL                                   /**< Bit mask for CRYPTO_V0 */
#define _CRYPTO_CSTATUS_V0_DDATA0                    0x00000000UL                            /**< Mode DDATA0 for CRYPTO_CSTATUS */
#define _CRYPTO_CSTATUS_V0_DEFAULT                   0x00000001UL                            /**< Mode DEFAULT for CRYPTO_CSTATUS */
#define _CRYPTO_CSTATUS_V0_DDATA1                    0x00000001UL                            /**< Mode DDATA1 for CRYPTO_CSTATUS */
#define _CRYPTO_CSTATUS_V0_DDATA2                    0x00000002UL                            /**< Mode DDATA2 for CRYPTO_CSTATUS */
#define _CRYPTO_CSTATUS_V0_DDATA3                    0x00000003UL                            /**< Mode DDATA3 for CRYPTO_CSTATUS */
#define _CRYPTO_CSTATUS_V0_DDATA4                    0x00000004UL                            /**< Mode DDATA4 for CRYPTO_CSTATUS */
#define _CRYPTO_CSTATUS_V0_DATA0                     0x00000005UL                            /**< Mode DATA0 for CRYPTO_CSTATUS */
#define _CRYPTO_CSTATUS_V0_DATA1                     0x00000006UL                            /**< Mode DATA1 for CRYPTO_CSTATUS */
#define _CRYPTO_CSTATUS_V0_DATA2                     0x00000007UL                            /**< Mode DATA2 for CRYPTO_CSTATUS */
#define CRYPTO_CSTATUS_V0_DDATA0                     (_CRYPTO_CSTATUS_V0_DDATA0 << 0)        /**< Shifted mode DDATA0 for CRYPTO_CSTATUS */
#define CRYPTO_CSTATUS_V0_DEFAULT                    (_CRYPTO_CSTATUS_V0_DEFAULT << 0)       /**< Shifted mode DEFAULT for CRYPTO_CSTATUS */
#define CRYPTO_CSTATUS_V0_DDATA1                     (_CRYPTO_CSTATUS_V0_DDATA1 << 0)        /**< Shifted mode DDATA1 for CRYPTO_CSTATUS */
#define CRYPTO_CSTATUS_V0_DDATA2                     (_CRYPTO_CSTATUS_V0_DDATA2 << 0)        /**< Shifted mode DDATA2 for CRYPTO_CSTATUS */
#define CRYPTO_CSTATUS_V0_DDATA3                     (_CRYPTO_CSTATUS_V0_DDATA3 << 0)        /**< Shifted mode DDATA3 for CRYPTO_CSTATUS */
#define CRYPTO_CSTATUS_V0_DDATA4                     (_CRYPTO_CSTATUS_V0_DDATA4 << 0)        /**< Shifted mode DDATA4 for CRYPTO_CSTATUS */
#define CRYPTO_CSTATUS_V0_DATA0                      (_CRYPTO_CSTATUS_V0_DATA0 << 0)         /**< Shifted mode DATA0 for CRYPTO_CSTATUS */
#define CRYPTO_CSTATUS_V0_DATA1                      (_CRYPTO_CSTATUS_V0_DATA1 << 0)         /**< Shifted mode DATA1 for CRYPTO_CSTATUS */
#define CRYPTO_CSTATUS_V0_DATA2                      (_CRYPTO_CSTATUS_V0_DATA2 << 0)         /**< Shifted mode DATA2 for CRYPTO_CSTATUS */
#define _CRYPTO_CSTATUS_V1_SHIFT                     8                                       /**< Shift value for CRYPTO_V1 */
#define _CRYPTO_CSTATUS_V1_MASK                      0x700UL                                 /**< Bit mask for CRYPTO_V1 */
#define _CRYPTO_CSTATUS_V1_DDATA0                    0x00000000UL                            /**< Mode DDATA0 for CRYPTO_CSTATUS */
#define _CRYPTO_CSTATUS_V1_DDATA1                    0x00000001UL                            /**< Mode DDATA1 for CRYPTO_CSTATUS */
#define _CRYPTO_CSTATUS_V1_DEFAULT                   0x00000002UL                            /**< Mode DEFAULT for CRYPTO_CSTATUS */
#define _CRYPTO_CSTATUS_V1_DDATA2                    0x00000002UL                            /**< Mode DDATA2 for CRYPTO_CSTATUS */
#define _CRYPTO_CSTATUS_V1_DDATA3                    0x00000003UL                            /**< Mode DDATA3 for CRYPTO_CSTATUS */
#define _CRYPTO_CSTATUS_V1_DDATA4                    0x00000004UL                            /**< Mode DDATA4 for CRYPTO_CSTATUS */
#define _CRYPTO_CSTATUS_V1_DATA0                     0x00000005UL                            /**< Mode DATA0 for CRYPTO_CSTATUS */
#define _CRYPTO_CSTATUS_V1_DATA1                     0x00000006UL                            /**< Mode DATA1 for CRYPTO_CSTATUS */
#define _CRYPTO_CSTATUS_V1_DATA2                     0x00000007UL                            /**< Mode DATA2 for CRYPTO_CSTATUS */
#define CRYPTO_CSTATUS_V1_DDATA0                     (_CRYPTO_CSTATUS_V1_DDATA0 << 8)        /**< Shifted mode DDATA0 for CRYPTO_CSTATUS */
#define CRYPTO_CSTATUS_V1_DDATA1                     (_CRYPTO_CSTATUS_V1_DDATA1 << 8)        /**< Shifted mode DDATA1 for CRYPTO_CSTATUS */
#define CRYPTO_CSTATUS_V1_DEFAULT                    (_CRYPTO_CSTATUS_V1_DEFAULT << 8)       /**< Shifted mode DEFAULT for CRYPTO_CSTATUS */
#define CRYPTO_CSTATUS_V1_DDATA2                     (_CRYPTO_CSTATUS_V1_DDATA2 << 8)        /**< Shifted mode DDATA2 for CRYPTO_CSTATUS */
#define CRYPTO_CSTATUS_V1_DDATA3                     (_CRYPTO_CSTATUS_V1_DDATA3 << 8)        /**< Shifted mode DDATA3 for CRYPTO_CSTATUS */
#define CRYPTO_CSTATUS_V1_DDATA4                     (_CRYPTO_CSTATUS_V1_DDATA4 << 8)        /**< Shifted mode DDATA4 for CRYPTO_CSTATUS */
#define CRYPTO_CSTATUS_V1_DATA0                      (_CRYPTO_CSTATUS_V1_DATA0 << 8)         /**< Shifted mode DATA0 for CRYPTO_CSTATUS */
#define CRYPTO_CSTATUS_V1_DATA1                      (_CRYPTO_CSTATUS_V1_DATA1 << 8)         /**< Shifted mode DATA1 for CRYPTO_CSTATUS */
#define CRYPTO_CSTATUS_V1_DATA2                      (_CRYPTO_CSTATUS_V1_DATA2 << 8)         /**< Shifted mode DATA2 for CRYPTO_CSTATUS */
#define CRYPTO_CSTATUS_SEQPART                       (0x1UL << 16)                           /**< Sequence Part */
#define _CRYPTO_CSTATUS_SEQPART_SHIFT                16                                      /**< Shift value for CRYPTO_SEQPART */
#define _CRYPTO_CSTATUS_SEQPART_MASK                 0x10000UL                               /**< Bit mask for CRYPTO_SEQPART */
#define _CRYPTO_CSTATUS_SEQPART_DEFAULT              0x00000000UL                            /**< Mode DEFAULT for CRYPTO_CSTATUS */
#define _CRYPTO_CSTATUS_SEQPART_SEQA                 0x00000000UL                            /**< Mode SEQA for CRYPTO_CSTATUS */
#define _CRYPTO_CSTATUS_SEQPART_SEQB                 0x00000001UL                            /**< Mode SEQB for CRYPTO_CSTATUS */
#define CRYPTO_CSTATUS_SEQPART_DEFAULT               (_CRYPTO_CSTATUS_SEQPART_DEFAULT << 16) /**< Shifted mode DEFAULT for CRYPTO_CSTATUS */
#define CRYPTO_CSTATUS_SEQPART_SEQA                  (_CRYPTO_CSTATUS_SEQPART_SEQA << 16)    /**< Shifted mode SEQA for CRYPTO_CSTATUS */
#define CRYPTO_CSTATUS_SEQPART_SEQB                  (_CRYPTO_CSTATUS_SEQPART_SEQB << 16)    /**< Shifted mode SEQB for CRYPTO_CSTATUS */
#define CRYPTO_CSTATUS_SEQSKIP                       (0x1UL << 17)                           /**< Sequence Skip Next Instruction */
#define _CRYPTO_CSTATUS_SEQSKIP_SHIFT                17                                      /**< Shift value for CRYPTO_SEQSKIP */
#define _CRYPTO_CSTATUS_SEQSKIP_MASK                 0x20000UL                               /**< Bit mask for CRYPTO_SEQSKIP */
#define _CRYPTO_CSTATUS_SEQSKIP_DEFAULT              0x00000000UL                            /**< Mode DEFAULT for CRYPTO_CSTATUS */
#define CRYPTO_CSTATUS_SEQSKIP_DEFAULT               (_CRYPTO_CSTATUS_SEQSKIP_DEFAULT << 17) /**< Shifted mode DEFAULT for CRYPTO_CSTATUS */
#define _CRYPTO_CSTATUS_SEQIP_SHIFT                  20                                      /**< Shift value for CRYPTO_SEQIP */
#define _CRYPTO_CSTATUS_SEQIP_MASK                   0x1F00000UL                             /**< Bit mask for CRYPTO_SEQIP */
#define _CRYPTO_CSTATUS_SEQIP_DEFAULT                0x00000000UL                            /**< Mode DEFAULT for CRYPTO_CSTATUS */
#define CRYPTO_CSTATUS_SEQIP_DEFAULT                 (_CRYPTO_CSTATUS_SEQIP_DEFAULT << 20)   /**< Shifted mode DEFAULT for CRYPTO_CSTATUS */

/* Bit fields for CRYPTO KEY */
#define _CRYPTO_KEY_RESETVALUE                       0x00000000UL                   /**< Default value for CRYPTO_KEY */
#define _CRYPTO_KEY_MASK                             0xFFFFFFFFUL                   /**< Mask for CRYPTO_KEY */
#define _CRYPTO_KEY_KEY_SHIFT                        0                              /**< Shift value for CRYPTO_KEY */
#define _CRYPTO_KEY_KEY_MASK                         0xFFFFFFFFUL                   /**< Bit mask for CRYPTO_KEY */
#define _CRYPTO_KEY_KEY_DEFAULT                      0x00000000UL                   /**< Mode DEFAULT for CRYPTO_KEY */
#define CRYPTO_KEY_KEY_DEFAULT                       (_CRYPTO_KEY_KEY_DEFAULT << 0) /**< Shifted mode DEFAULT for CRYPTO_KEY */

/* Bit fields for CRYPTO KEYBUF */
#define _CRYPTO_KEYBUF_RESETVALUE                    0x00000000UL                         /**< Default value for CRYPTO_KEYBUF */
#define _CRYPTO_KEYBUF_MASK                          0xFFFFFFFFUL                         /**< Mask for CRYPTO_KEYBUF */
#define _CRYPTO_KEYBUF_KEYBUF_SHIFT                  0                                    /**< Shift value for CRYPTO_KEYBUF */
#define _CRYPTO_KEYBUF_KEYBUF_MASK                   0xFFFFFFFFUL                         /**< Bit mask for CRYPTO_KEYBUF */
#define _CRYPTO_KEYBUF_KEYBUF_DEFAULT                0x00000000UL                         /**< Mode DEFAULT for CRYPTO_KEYBUF */
#define CRYPTO_KEYBUF_KEYBUF_DEFAULT                 (_CRYPTO_KEYBUF_KEYBUF_DEFAULT << 0) /**< Shifted mode DEFAULT for CRYPTO_KEYBUF */

/* Bit fields for CRYPTO SEQCTRL */
#define _CRYPTO_SEQCTRL_RESETVALUE                   0x00000000UL                              /**< Default value for CRYPTO_SEQCTRL */
#define _CRYPTO_SEQCTRL_MASK                         0xBF303FFFUL                              /**< Mask for CRYPTO_SEQCTRL */
#define _CRYPTO_SEQCTRL_LENGTHA_SHIFT                0                                         /**< Shift value for CRYPTO_LENGTHA */
#define _CRYPTO_SEQCTRL_LENGTHA_MASK                 0x3FFFUL                                  /**< Bit mask for CRYPTO_LENGTHA */
#define _CRYPTO_SEQCTRL_LENGTHA_DEFAULT              0x00000000UL                              /**< Mode DEFAULT for CRYPTO_SEQCTRL */
#define CRYPTO_SEQCTRL_LENGTHA_DEFAULT               (_CRYPTO_SEQCTRL_LENGTHA_DEFAULT << 0)    /**< Shifted mode DEFAULT for CRYPTO_SEQCTRL */
#define _CRYPTO_SEQCTRL_BLOCKSIZE_SHIFT              20                                        /**< Shift value for CRYPTO_BLOCKSIZE */
#define _CRYPTO_SEQCTRL_BLOCKSIZE_MASK               0x300000UL                                /**< Bit mask for CRYPTO_BLOCKSIZE */
#define _CRYPTO_SEQCTRL_BLOCKSIZE_DEFAULT            0x00000000UL                              /**< Mode DEFAULT for CRYPTO_SEQCTRL */
#define _CRYPTO_SEQCTRL_BLOCKSIZE_16BYTES            0x00000000UL                              /**< Mode 16BYTES for CRYPTO_SEQCTRL */
#define _CRYPTO_SEQCTRL_BLOCKSIZE_32BYTES            0x00000001UL                              /**< Mode 32BYTES for CRYPTO_SEQCTRL */
#define _CRYPTO_SEQCTRL_BLOCKSIZE_64BYTES            0x00000002UL                              /**< Mode 64BYTES for CRYPTO_SEQCTRL */
#define CRYPTO_SEQCTRL_BLOCKSIZE_DEFAULT             (_CRYPTO_SEQCTRL_BLOCKSIZE_DEFAULT << 20) /**< Shifted mode DEFAULT for CRYPTO_SEQCTRL */
#define CRYPTO_SEQCTRL_BLOCKSIZE_16BYTES             (_CRYPTO_SEQCTRL_BLOCKSIZE_16BYTES << 20) /**< Shifted mode 16BYTES for CRYPTO_SEQCTRL */
#define CRYPTO_SEQCTRL_BLOCKSIZE_32BYTES             (_CRYPTO_SEQCTRL_BLOCKSIZE_32BYTES << 20) /**< Shifted mode 32BYTES for CRYPTO_SEQCTRL */
#define CRYPTO_SEQCTRL_BLOCKSIZE_64BYTES             (_CRYPTO_SEQCTRL_BLOCKSIZE_64BYTES << 20) /**< Shifted mode 64BYTES for CRYPTO_SEQCTRL */
#define _CRYPTO_SEQCTRL_DMA0SKIP_SHIFT               24                                        /**< Shift value for CRYPTO_DMA0SKIP */
#define _CRYPTO_SEQCTRL_DMA0SKIP_MASK                0x3000000UL                               /**< Bit mask for CRYPTO_DMA0SKIP */
#define _CRYPTO_SEQCTRL_DMA0SKIP_DEFAULT             0x00000000UL                              /**< Mode DEFAULT for CRYPTO_SEQCTRL */
#define CRYPTO_SEQCTRL_DMA0SKIP_DEFAULT              (_CRYPTO_SEQCTRL_DMA0SKIP_DEFAULT << 24)  /**< Shifted mode DEFAULT for CRYPTO_SEQCTRL */
#define _CRYPTO_SEQCTRL_DMA1SKIP_SHIFT               26                                        /**< Shift value for CRYPTO_DMA1SKIP */
#define _CRYPTO_SEQCTRL_DMA1SKIP_MASK                0xC000000UL                               /**< Bit mask for CRYPTO_DMA1SKIP */
#define _CRYPTO_SEQCTRL_DMA1SKIP_DEFAULT             0x00000000UL                              /**< Mode DEFAULT for CRYPTO_SEQCTRL */
#define CRYPTO_SEQCTRL_DMA1SKIP_DEFAULT              (_CRYPTO_SEQCTRL_DMA1SKIP_DEFAULT << 26)  /**< Shifted mode DEFAULT for CRYPTO_SEQCTRL */
#define CRYPTO_SEQCTRL_DMA0PRESA                     (0x1UL << 28)                             /**< DMA0 Preserve A */
#define _CRYPTO_SEQCTRL_DMA0PRESA_SHIFT              28                                        /**< Shift value for CRYPTO_DMA0PRESA */
#define _CRYPTO_SEQCTRL_DMA0PRESA_MASK               0x10000000UL                              /**< Bit mask for CRYPTO_DMA0PRESA */
#define _CRYPTO_SEQCTRL_DMA0PRESA_DEFAULT            0x00000000UL                              /**< Mode DEFAULT for CRYPTO_SEQCTRL */
#define CRYPTO_SEQCTRL_DMA0PRESA_DEFAULT             (_CRYPTO_SEQCTRL_DMA0PRESA_DEFAULT << 28) /**< Shifted mode DEFAULT for CRYPTO_SEQCTRL */
#define CRYPTO_SEQCTRL_DMA1PRESA                     (0x1UL << 29)                             /**< DMA1 Preserve A */
#define _CRYPTO_SEQCTRL_DMA1PRESA_SHIFT              29                                        /**< Shift value for CRYPTO_DMA1PRESA */
#define _CRYPTO_SEQCTRL_DMA1PRESA_MASK               0x20000000UL                              /**< Bit mask for CRYPTO_DMA1PRESA */
#define _CRYPTO_SEQCTRL_DMA1PRESA_DEFAULT            0x00000000UL                              /**< Mode DEFAULT for CRYPTO_SEQCTRL */
#define CRYPTO_SEQCTRL_DMA1PRESA_DEFAULT             (_CRYPTO_SEQCTRL_DMA1PRESA_DEFAULT << 29) /**< Shifted mode DEFAULT for CRYPTO_SEQCTRL */
#define CRYPTO_SEQCTRL_HALT                          (0x1UL << 31)                             /**< Halt Sequence */
#define _CRYPTO_SEQCTRL_HALT_SHIFT                   31                                        /**< Shift value for CRYPTO_HALT */
#define _CRYPTO_SEQCTRL_HALT_MASK                    0x80000000UL                              /**< Bit mask for CRYPTO_HALT */
#define _CRYPTO_SEQCTRL_HALT_DEFAULT                 0x00000000UL                              /**< Mode DEFAULT for CRYPTO_SEQCTRL */
#define CRYPTO_SEQCTRL_HALT_DEFAULT                  (_CRYPTO_SEQCTRL_HALT_DEFAULT << 31)      /**< Shifted mode DEFAULT for CRYPTO_SEQCTRL */

/* Bit fields for CRYPTO SEQCTRLB */
#define _CRYPTO_SEQCTRLB_RESETVALUE                  0x00000000UL                               /**< Default value for CRYPTO_SEQCTRLB */
#define _CRYPTO_SEQCTRLB_MASK                        0x30003FFFUL                               /**< Mask for CRYPTO_SEQCTRLB */
#define _CRYPTO_SEQCTRLB_LENGTHB_SHIFT               0                                          /**< Shift value for CRYPTO_LENGTHB */
#define _CRYPTO_SEQCTRLB_LENGTHB_MASK                0x3FFFUL                                   /**< Bit mask for CRYPTO_LENGTHB */
#define _CRYPTO_SEQCTRLB_LENGTHB_DEFAULT             0x00000000UL                               /**< Mode DEFAULT for CRYPTO_SEQCTRLB */
#define CRYPTO_SEQCTRLB_LENGTHB_DEFAULT              (_CRYPTO_SEQCTRLB_LENGTHB_DEFAULT << 0)    /**< Shifted mode DEFAULT for CRYPTO_SEQCTRLB */
#define CRYPTO_SEQCTRLB_DMA0PRESB                    (0x1UL << 28)                              /**< DMA0 Preserve B */
#define _CRYPTO_SEQCTRLB_DMA0PRESB_SHIFT             28                                         /**< Shift value for CRYPTO_DMA0PRESB */
#define _CRYPTO_SEQCTRLB_DMA0PRESB_MASK              0x10000000UL                               /**< Bit mask for CRYPTO_DMA0PRESB */
#define _CRYPTO_SEQCTRLB_DMA0PRESB_DEFAULT           0x00000000UL                               /**< Mode DEFAULT for CRYPTO_SEQCTRLB */
#define CRYPTO_SEQCTRLB_DMA0PRESB_DEFAULT            (_CRYPTO_SEQCTRLB_DMA0PRESB_DEFAULT << 28) /**< Shifted mode DEFAULT for CRYPTO_SEQCTRLB */
#define CRYPTO_SEQCTRLB_DMA1PRESB                    (0x1UL << 29)                              /**< DMA1 Preserve B */
#define _CRYPTO_SEQCTRLB_DMA1PRESB_SHIFT             29                                         /**< Shift value for CRYPTO_DMA1PRESB */
#define _CRYPTO_SEQCTRLB_DMA1PRESB_MASK              0x20000000UL                               /**< Bit mask for CRYPTO_DMA1PRESB */
#define _CRYPTO_SEQCTRLB_DMA1PRESB_DEFAULT           0x00000000UL                               /**< Mode DEFAULT for CRYPTO_SEQCTRLB */
#define CRYPTO_SEQCTRLB_DMA1PRESB_DEFAULT            (_CRYPTO_SEQCTRLB_DMA1PRESB_DEFAULT << 29) /**< Shifted mode DEFAULT for CRYPTO_SEQCTRLB */

/* Bit fields for CRYPTO IF */
#define _CRYPTO_IF_RESETVALUE                        0x00000000UL                        /**< Default value for CRYPTO_IF */
#define _CRYPTO_IF_MASK                              0x00000003UL                        /**< Mask for CRYPTO_IF */
#define CRYPTO_IF_INSTRDONE                          (0x1UL << 0)                        /**< Instruction done */
#define _CRYPTO_IF_INSTRDONE_SHIFT                   0                                   /**< Shift value for CRYPTO_INSTRDONE */
#define _CRYPTO_IF_INSTRDONE_MASK                    0x1UL                               /**< Bit mask for CRYPTO_INSTRDONE */
#define _CRYPTO_IF_INSTRDONE_DEFAULT                 0x00000000UL                        /**< Mode DEFAULT for CRYPTO_IF */
#define CRYPTO_IF_INSTRDONE_DEFAULT                  (_CRYPTO_IF_INSTRDONE_DEFAULT << 0) /**< Shifted mode DEFAULT for CRYPTO_IF */
#define CRYPTO_IF_SEQDONE                            (0x1UL << 1)                        /**< Sequence Done */
#define _CRYPTO_IF_SEQDONE_SHIFT                     1                                   /**< Shift value for CRYPTO_SEQDONE */
#define _CRYPTO_IF_SEQDONE_MASK                      0x2UL                               /**< Bit mask for CRYPTO_SEQDONE */
#define _CRYPTO_IF_SEQDONE_DEFAULT                   0x00000000UL                        /**< Mode DEFAULT for CRYPTO_IF */
#define CRYPTO_IF_SEQDONE_DEFAULT                    (_CRYPTO_IF_SEQDONE_DEFAULT << 1)   /**< Shifted mode DEFAULT for CRYPTO_IF */

/* Bit fields for CRYPTO IFS */
#define _CRYPTO_IFS_RESETVALUE                       0x00000000UL                         /**< Default value for CRYPTO_IFS */
#define _CRYPTO_IFS_MASK                             0x00000003UL                         /**< Mask for CRYPTO_IFS */
#define CRYPTO_IFS_INSTRDONE                         (0x1UL << 0)                         /**< Set INSTRDONE Interrupt Flag */
#define _CRYPTO_IFS_INSTRDONE_SHIFT                  0                                    /**< Shift value for CRYPTO_INSTRDONE */
#define _CRYPTO_IFS_INSTRDONE_MASK                   0x1UL                                /**< Bit mask for CRYPTO_INSTRDONE */
#define _CRYPTO_IFS_INSTRDONE_DEFAULT                0x00000000UL                         /**< Mode DEFAULT for CRYPTO_IFS */
#define CRYPTO_IFS_INSTRDONE_DEFAULT                 (_CRYPTO_IFS_INSTRDONE_DEFAULT << 0) /**< Shifted mode DEFAULT for CRYPTO_IFS */
#define CRYPTO_IFS_SEQDONE                           (0x1UL << 1)                         /**< Set SEQDONE Interrupt Flag */
#define _CRYPTO_IFS_SEQDONE_SHIFT                    1                                    /**< Shift value for CRYPTO_SEQDONE */
#define _CRYPTO_IFS_SEQDONE_MASK                     0x2UL                                /**< Bit mask for CRYPTO_SEQDONE */
#define _CRYPTO_IFS_SEQDONE_DEFAULT                  0x00000000UL                         /**< Mode DEFAULT for CRYPTO_IFS */
#define CRYPTO_IFS_SEQDONE_DEFAULT                   (_CRYPTO_IFS_SEQDONE_DEFAULT << 1)   /**< Shifted mode DEFAULT for CRYPTO_IFS */

/* Bit fields for CRYPTO IFC */
#define _CRYPTO_IFC_RESETVALUE                       0x00000000UL                         /**< Default value for CRYPTO_IFC */
#define _CRYPTO_IFC_MASK                             0x00000003UL                         /**< Mask for CRYPTO_IFC */
#define CRYPTO_IFC_INSTRDONE                         (0x1UL << 0)                         /**< Clear INSTRDONE Interrupt Flag */
#define _CRYPTO_IFC_INSTRDONE_SHIFT                  0                                    /**< Shift value for CRYPTO_INSTRDONE */
#define _CRYPTO_IFC_INSTRDONE_MASK                   0x1UL                                /**< Bit mask for CRYPTO_INSTRDONE */
#define _CRYPTO_IFC_INSTRDONE_DEFAULT                0x00000000UL                         /**< Mode DEFAULT for CRYPTO_IFC */
#define CRYPTO_IFC_INSTRDONE_DEFAULT                 (_CRYPTO_IFC_INSTRDONE_DEFAULT << 0) /**< Shifted mode DEFAULT for CRYPTO_IFC */
#define CRYPTO_IFC_SEQDONE                           (0x1UL << 1)                         /**< Clear SEQDONE Interrupt Flag */
#define _CRYPTO_IFC_SEQDONE_SHIFT                    1                                    /**< Shift value for CRYPTO_SEQDONE */
#define _CRYPTO_IFC_SEQDONE_MASK                     0x2UL                                /**< Bit mask for CRYPTO_SEQDONE */
#define _CRYPTO_IFC_SEQDONE_DEFAULT                  0x00000000UL                         /**< Mode DEFAULT for CRYPTO_IFC */
#define CRYPTO_IFC_SEQDONE_DEFAULT                   (_CRYPTO_IFC_SEQDONE_DEFAULT << 1)   /**< Shifted mode DEFAULT for CRYPTO_IFC */

/* Bit fields for CRYPTO IEN */
#define _CRYPTO_IEN_RESETVALUE                       0x00000000UL                         /**< Default value for CRYPTO_IEN */
#define _CRYPTO_IEN_MASK                             0x00000003UL                         /**< Mask for CRYPTO_IEN */
#define CRYPTO_IEN_INSTRDONE                         (0x1UL << 0)                         /**< INSTRDONE Interrupt Enable */
#define _CRYPTO_IEN_INSTRDONE_SHIFT                  0                                    /**< Shift value for CRYPTO_INSTRDONE */
#define _CRYPTO_IEN_INSTRDONE_MASK                   0x1UL                                /**< Bit mask for CRYPTO_INSTRDONE */
#define _CRYPTO_IEN_INSTRDONE_DEFAULT                0x00000000UL                         /**< Mode DEFAULT for CRYPTO_IEN */
#define CRYPTO_IEN_INSTRDONE_DEFAULT                 (_CRYPTO_IEN_INSTRDONE_DEFAULT << 0) /**< Shifted mode DEFAULT for CRYPTO_IEN */
#define CRYPTO_IEN_SEQDONE                           (0x1UL << 1)                         /**< SEQDONE Interrupt Enable */
#define _CRYPTO_IEN_SEQDONE_SHIFT                    1                                    /**< Shift value for CRYPTO_SEQDONE */
#define _CRYPTO_IEN_SEQDONE_MASK                     0x2UL                                /**< Bit mask for CRYPTO_SEQDONE */
#define _CRYPTO_IEN_SEQDONE_DEFAULT                  0x00000000UL                         /**< Mode DEFAULT for CRYPTO_IEN */
#define CRYPTO_IEN_SEQDONE_DEFAULT                   (_CRYPTO_IEN_SEQDONE_DEFAULT << 1)   /**< Shifted mode DEFAULT for CRYPTO_IEN */

/* Bit fields for CRYPTO SEQ0 */
#define _CRYPTO_SEQ0_RESETVALUE                      0x00000000UL                        /**< Default value for CRYPTO_SEQ0 */
#define _CRYPTO_SEQ0_MASK                            0xFFFFFFFFUL                        /**< Mask for CRYPTO_SEQ0 */
#define _CRYPTO_SEQ0_INSTR0_SHIFT                    0                                   /**< Shift value for CRYPTO_INSTR0 */
#define _CRYPTO_SEQ0_INSTR0_MASK                     0xFFUL                              /**< Bit mask for CRYPTO_INSTR0 */
#define _CRYPTO_SEQ0_INSTR0_DEFAULT                  0x00000000UL                        /**< Mode DEFAULT for CRYPTO_SEQ0 */
#define CRYPTO_SEQ0_INSTR0_DEFAULT                   (_CRYPTO_SEQ0_INSTR0_DEFAULT << 0)  /**< Shifted mode DEFAULT for CRYPTO_SEQ0 */
#define _CRYPTO_SEQ0_INSTR1_SHIFT                    8                                   /**< Shift value for CRYPTO_INSTR1 */
#define _CRYPTO_SEQ0_INSTR1_MASK                     0xFF00UL                            /**< Bit mask for CRYPTO_INSTR1 */
#define _CRYPTO_SEQ0_INSTR1_DEFAULT                  0x00000000UL                        /**< Mode DEFAULT for CRYPTO_SEQ0 */
#define CRYPTO_SEQ0_INSTR1_DEFAULT                   (_CRYPTO_SEQ0_INSTR1_DEFAULT << 8)  /**< Shifted mode DEFAULT for CRYPTO_SEQ0 */
#define _CRYPTO_SEQ0_INSTR2_SHIFT                    16                                  /**< Shift value for CRYPTO_INSTR2 */
#define _CRYPTO_SEQ0_INSTR2_MASK                     0xFF0000UL                          /**< Bit mask for CRYPTO_INSTR2 */
#define _CRYPTO_SEQ0_INSTR2_DEFAULT                  0x00000000UL                        /**< Mode DEFAULT for CRYPTO_SEQ0 */
#define CRYPTO_SEQ0_INSTR2_DEFAULT                   (_CRYPTO_SEQ0_INSTR2_DEFAULT << 16) /**< Shifted mode DEFAULT for CRYPTO_SEQ0 */
#define _CRYPTO_SEQ0_INSTR3_SHIFT                    24                                  /**< Shift value for CRYPTO_INSTR3 */
#define _CRYPTO_SEQ0_INSTR3_MASK                     0xFF000000UL                        /**< Bit mask for CRYPTO_INSTR3 */
#define _CRYPTO_SEQ0_INSTR3_DEFAULT                  0x00000000UL                        /**< Mode DEFAULT for CRYPTO_SEQ0 */
#define CRYPTO_SEQ0_INSTR3_DEFAULT                   (_CRYPTO_SEQ0_INSTR3_DEFAULT << 24) /**< Shifted mode DEFAULT for CRYPTO_SEQ0 */

/* Bit fields for CRYPTO SEQ1 */
#define _CRYPTO_SEQ1_RESETVALUE                      0x00000000UL                        /**< Default value for CRYPTO_SEQ1 */
#define _CRYPTO_SEQ1_MASK                            0xFFFFFFFFUL                        /**< Mask for CRYPTO_SEQ1 */
#define _CRYPTO_SEQ1_INSTR4_SHIFT                    0                                   /**< Shift value for CRYPTO_INSTR4 */
#define _CRYPTO_SEQ1_INSTR4_MASK                     0xFFUL                              /**< Bit mask for CRYPTO_INSTR4 */
#define _CRYPTO_SEQ1_INSTR4_DEFAULT                  0x00000000UL                        /**< Mode DEFAULT for CRYPTO_SEQ1 */
#define CRYPTO_SEQ1_INSTR4_DEFAULT                   (_CRYPTO_SEQ1_INSTR4_DEFAULT << 0)  /**< Shifted mode DEFAULT for CRYPTO_SEQ1 */
#define _CRYPTO_SEQ1_INSTR5_SHIFT                    8                                   /**< Shift value for CRYPTO_INSTR5 */
#define _CRYPTO_SEQ1_INSTR5_MASK                     0xFF00UL                            /**< Bit mask for CRYPTO_INSTR5 */
#define _CRYPTO_SEQ1_INSTR5_DEFAULT                  0x00000000UL                        /**< Mode DEFAULT for CRYPTO_SEQ1 */
#define CRYPTO_SEQ1_INSTR5_DEFAULT                   (_CRYPTO_SEQ1_INSTR5_DEFAULT << 8)  /**< Shifted mode DEFAULT for CRYPTO_SEQ1 */
#define _CRYPTO_SEQ1_INSTR6_SHIFT                    16                                  /**< Shift value for CRYPTO_INSTR6 */
#define _CRYPTO_SEQ1_INSTR6_MASK                     0xFF0000UL                          /**< Bit mask for CRYPTO_INSTR6 */
#define _CRYPTO_SEQ1_INSTR6_DEFAULT                  0x00000000UL                        /**< Mode DEFAULT for CRYPTO_SEQ1 */
#define CRYPTO_SEQ1_INSTR6_DEFAULT                   (_CRYPTO_SEQ1_INSTR6_DEFAULT << 16) /**< Shifted mode DEFAULT for CRYPTO_SEQ1 */
#define _CRYPTO_SEQ1_INSTR7_SHIFT                    24                                  /**< Shift value for CRYPTO_INSTR7 */
#define _CRYPTO_SEQ1_INSTR7_MASK                     0xFF000000UL                        /**< Bit mask for CRYPTO_INSTR7 */
#define _CRYPTO_SEQ1_INSTR7_DEFAULT                  0x00000000UL                        /**< Mode DEFAULT for CRYPTO_SEQ1 */
#define CRYPTO_SEQ1_INSTR7_DEFAULT                   (_CRYPTO_SEQ1_INSTR7_DEFAULT << 24) /**< Shifted mode DEFAULT for CRYPTO_SEQ1 */

/* Bit fields for CRYPTO SEQ2 */
#define _CRYPTO_SEQ2_RESETVALUE                      0x00000000UL                         /**< Default value for CRYPTO_SEQ2 */
#define _CRYPTO_SEQ2_MASK                            0xFFFFFFFFUL                         /**< Mask for CRYPTO_SEQ2 */
#define _CRYPTO_SEQ2_INSTR8_SHIFT                    0                                    /**< Shift value for CRYPTO_INSTR8 */
#define _CRYPTO_SEQ2_INSTR8_MASK                     0xFFUL                               /**< Bit mask for CRYPTO_INSTR8 */
#define _CRYPTO_SEQ2_INSTR8_DEFAULT                  0x00000000UL                         /**< Mode DEFAULT for CRYPTO_SEQ2 */
#define CRYPTO_SEQ2_INSTR8_DEFAULT                   (_CRYPTO_SEQ2_INSTR8_DEFAULT << 0)   /**< Shifted mode DEFAULT for CRYPTO_SEQ2 */
#define _CRYPTO_SEQ2_INSTR9_SHIFT                    8                                    /**< Shift value for CRYPTO_INSTR9 */
#define _CRYPTO_SEQ2_INSTR9_MASK                     0xFF00UL                             /**< Bit mask for CRYPTO_INSTR9 */
#define _CRYPTO_SEQ2_INSTR9_DEFAULT                  0x00000000UL                         /**< Mode DEFAULT for CRYPTO_SEQ2 */
#define CRYPTO_SEQ2_INSTR9_DEFAULT                   (_CRYPTO_SEQ2_INSTR9_DEFAULT << 8)   /**< Shifted mode DEFAULT for CRYPTO_SEQ2 */
#define _CRYPTO_SEQ2_INSTR10_SHIFT                   16                                   /**< Shift value for CRYPTO_INSTR10 */
#define _CRYPTO_SEQ2_INSTR10_MASK                    0xFF0000UL                           /**< Bit mask for CRYPTO_INSTR10 */
#define _CRYPTO_SEQ2_INSTR10_DEFAULT                 0x00000000UL                         /**< Mode DEFAULT for CRYPTO_SEQ2 */
#define CRYPTO_SEQ2_INSTR10_DEFAULT                  (_CRYPTO_SEQ2_INSTR10_DEFAULT << 16) /**< Shifted mode DEFAULT for CRYPTO_SEQ2 */
#define _CRYPTO_SEQ2_INSTR11_SHIFT                   24                                   /**< Shift value for CRYPTO_INSTR11 */
#define _CRYPTO_SEQ2_INSTR11_MASK                    0xFF000000UL                         /**< Bit mask for CRYPTO_INSTR11 */
#define _CRYPTO_SEQ2_INSTR11_DEFAULT                 0x00000000UL                         /**< Mode DEFAULT for CRYPTO_SEQ2 */
#define CRYPTO_SEQ2_INSTR11_DEFAULT                  (_CRYPTO_SEQ2_INSTR11_DEFAULT << 24) /**< Shifted mode DEFAULT for CRYPTO_SEQ2 */

/* Bit fields for CRYPTO SEQ3 */
#define _CRYPTO_SEQ3_RESETVALUE                      0x00000000UL                         /**< Default value for CRYPTO_SEQ3 */
#define _CRYPTO_SEQ3_MASK                            0xFFFFFFFFUL                         /**< Mask for CRYPTO_SEQ3 */
#define _CRYPTO_SEQ3_INSTR12_SHIFT                   0                                    /**< Shift value for CRYPTO_INSTR12 */
#define _CRYPTO_SEQ3_INSTR12_MASK                    0xFFUL                               /**< Bit mask for CRYPTO_INSTR12 */
#define _CRYPTO_SEQ3_INSTR12_DEFAULT                 0x00000000UL                         /**< Mode DEFAULT for CRYPTO_SEQ3 */
#define CRYPTO_SEQ3_INSTR12_DEFAULT                  (_CRYPTO_SEQ3_INSTR12_DEFAULT << 0)  /**< Shifted mode DEFAULT for CRYPTO_SEQ3 */
#define _CRYPTO_SEQ3_INSTR13_SHIFT                   8                                    /**< Shift value for CRYPTO_INSTR13 */
#define _CRYPTO_SEQ3_INSTR13_MASK                    0xFF00UL                             /**< Bit mask for CRYPTO_INSTR13 */
#define _CRYPTO_SEQ3_INSTR13_DEFAULT                 0x00000000UL                         /**< Mode DEFAULT for CRYPTO_SEQ3 */
#define CRYPTO_SEQ3_INSTR13_DEFAULT                  (_CRYPTO_SEQ3_INSTR13_DEFAULT << 8)  /**< Shifted mode DEFAULT for CRYPTO_SEQ3 */
#define _CRYPTO_SEQ3_INSTR14_SHIFT                   16                                   /**< Shift value for CRYPTO_INSTR14 */
#define _CRYPTO_SEQ3_INSTR14_MASK                    0xFF0000UL                           /**< Bit mask for CRYPTO_INSTR14 */
#define _CRYPTO_SEQ3_INSTR14_DEFAULT                 0x00000000UL                         /**< Mode DEFAULT for CRYPTO_SEQ3 */
#define CRYPTO_SEQ3_INSTR14_DEFAULT                  (_CRYPTO_SEQ3_INSTR14_DEFAULT << 16) /**< Shifted mode DEFAULT for CRYPTO_SEQ3 */
#define _CRYPTO_SEQ3_INSTR15_SHIFT                   24                                   /**< Shift value for CRYPTO_INSTR15 */
#define _CRYPTO_SEQ3_INSTR15_MASK                    0xFF000000UL                         /**< Bit mask for CRYPTO_INSTR15 */
#define _CRYPTO_SEQ3_INSTR15_DEFAULT                 0x00000000UL                         /**< Mode DEFAULT for CRYPTO_SEQ3 */
#define CRYPTO_SEQ3_INSTR15_DEFAULT                  (_CRYPTO_SEQ3_INSTR15_DEFAULT << 24) /**< Shifted mode DEFAULT for CRYPTO_SEQ3 */

/* Bit fields for CRYPTO SEQ4 */
#define _CRYPTO_SEQ4_RESETVALUE                      0x00000000UL                         /**< Default value for CRYPTO_SEQ4 */
#define _CRYPTO_SEQ4_MASK                            0xFFFFFFFFUL                         /**< Mask for CRYPTO_SEQ4 */
#define _CRYPTO_SEQ4_INSTR16_SHIFT                   0                                    /**< Shift value for CRYPTO_INSTR16 */
#define _CRYPTO_SEQ4_INSTR16_MASK                    0xFFUL                               /**< Bit mask for CRYPTO_INSTR16 */
#define _CRYPTO_SEQ4_INSTR16_DEFAULT                 0x00000000UL                         /**< Mode DEFAULT for CRYPTO_SEQ4 */
#define CRYPTO_SEQ4_INSTR16_DEFAULT                  (_CRYPTO_SEQ4_INSTR16_DEFAULT << 0)  /**< Shifted mode DEFAULT for CRYPTO_SEQ4 */
#define _CRYPTO_SEQ4_INSTR17_SHIFT                   8                                    /**< Shift value for CRYPTO_INSTR17 */
#define _CRYPTO_SEQ4_INSTR17_MASK                    0xFF00UL                             /**< Bit mask for CRYPTO_INSTR17 */
#define _CRYPTO_SEQ4_INSTR17_DEFAULT                 0x00000000UL                         /**< Mode DEFAULT for CRYPTO_SEQ4 */
#define CRYPTO_SEQ4_INSTR17_DEFAULT                  (_CRYPTO_SEQ4_INSTR17_DEFAULT << 8)  /**< Shifted mode DEFAULT for CRYPTO_SEQ4 */
#define _CRYPTO_SEQ4_INSTR18_SHIFT                   16                                   /**< Shift value for CRYPTO_INSTR18 */
#define _CRYPTO_SEQ4_INSTR18_MASK                    0xFF0000UL                           /**< Bit mask for CRYPTO_INSTR18 */
#define _CRYPTO_SEQ4_INSTR18_DEFAULT                 0x00000000UL                         /**< Mode DEFAULT for CRYPTO_SEQ4 */
#define CRYPTO_SEQ4_INSTR18_DEFAULT                  (_CRYPTO_SEQ4_INSTR18_DEFAULT << 16) /**< Shifted mode DEFAULT for CRYPTO_SEQ4 */
#define _CRYPTO_SEQ4_INSTR19_SHIFT                   24                                   /**< Shift value for CRYPTO_INSTR19 */
#define _CRYPTO_SEQ4_INSTR19_MASK                    0xFF000000UL                         /**< Bit mask for CRYPTO_INSTR19 */
#define _CRYPTO_SEQ4_INSTR19_DEFAULT                 0x00000000UL                         /**< Mode DEFAULT for CRYPTO_SEQ4 */
#define CRYPTO_SEQ4_INSTR19_DEFAULT                  (_CRYPTO_SEQ4_INSTR19_DEFAULT << 24) /**< Shifted mode DEFAULT for CRYPTO_SEQ4 */

/* Bit fields for CRYPTO DATA0 */
#define _CRYPTO_DATA0_RESETVALUE                     0x00000000UL                       /**< Default value for CRYPTO_DATA0 */
#define _CRYPTO_DATA0_MASK                           0xFFFFFFFFUL                       /**< Mask for CRYPTO_DATA0 */
#define _CRYPTO_DATA0_DATA0_SHIFT                    0                                  /**< Shift value for CRYPTO_DATA0 */
#define _CRYPTO_DATA0_DATA0_MASK                     0xFFFFFFFFUL                       /**< Bit mask for CRYPTO_DATA0 */
#define _CRYPTO_DATA0_DATA0_DEFAULT                  0x00000000UL                       /**< Mode DEFAULT for CRYPTO_DATA0 */
#define CRYPTO_DATA0_DATA0_DEFAULT                   (_CRYPTO_DATA0_DATA0_DEFAULT << 0) /**< Shifted mode DEFAULT for CRYPTO_DATA0 */

/* Bit fields for CRYPTO DATA1 */
#define _CRYPTO_DATA1_RESETVALUE                     0x00000000UL                       /**< Default value for CRYPTO_DATA1 */
#define _CRYPTO_DATA1_MASK                           0xFFFFFFFFUL                       /**< Mask for CRYPTO_DATA1 */
#define _CRYPTO_DATA1_DATA1_SHIFT                    0                                  /**< Shift value for CRYPTO_DATA1 */
#define _CRYPTO_DATA1_DATA1_MASK                     0xFFFFFFFFUL                       /**< Bit mask for CRYPTO_DATA1 */
#define _CRYPTO_DATA1_DATA1_DEFAULT                  0x00000000UL                       /**< Mode DEFAULT for CRYPTO_DATA1 */
#define CRYPTO_DATA1_DATA1_DEFAULT                   (_CRYPTO_DATA1_DATA1_DEFAULT << 0) /**< Shifted mode DEFAULT for CRYPTO_DATA1 */

/* Bit fields for CRYPTO DATA2 */
#define _CRYPTO_DATA2_RESETVALUE                     0x00000000UL                       /**< Default value for CRYPTO_DATA2 */
#define _CRYPTO_DATA2_MASK                           0xFFFFFFFFUL                       /**< Mask for CRYPTO_DATA2 */
#define _CRYPTO_DATA2_DATA2_SHIFT                    0                                  /**< Shift value for CRYPTO_DATA2 */
#define _CRYPTO_DATA2_DATA2_MASK                     0xFFFFFFFFUL                       /**< Bit mask for CRYPTO_DATA2 */
#define _CRYPTO_DATA2_DATA2_DEFAULT                  0x00000000UL                       /**< Mode DEFAULT for CRYPTO_DATA2 */
#define CRYPTO_DATA2_DATA2_DEFAULT                   (_CRYPTO_DATA2_DATA2_DEFAULT << 0) /**< Shifted mode DEFAULT for CRYPTO_DATA2 */

/* Bit fields for CRYPTO DATA3 */
#define _CRYPTO_DATA3_RESETVALUE                     0x00000000UL                       /**< Default value for CRYPTO_DATA3 */
#define _CRYPTO_DATA3_MASK                           0xFFFFFFFFUL                       /**< Mask for CRYPTO_DATA3 */
#define _CRYPTO_DATA3_DATA3_SHIFT                    0                                  /**< Shift value for CRYPTO_DATA3 */
#define _CRYPTO_DATA3_DATA3_MASK                     0xFFFFFFFFUL                       /**< Bit mask for CRYPTO_DATA3 */
#define _CRYPTO_DATA3_DATA3_DEFAULT                  0x00000000UL                       /**< Mode DEFAULT for CRYPTO_DATA3 */
#define CRYPTO_DATA3_DATA3_DEFAULT                   (_CRYPTO_DATA3_DATA3_DEFAULT << 0) /**< Shifted mode DEFAULT for CRYPTO_DATA3 */

/* Bit fields for CRYPTO DATA0XOR */
#define _CRYPTO_DATA0XOR_RESETVALUE                  0x00000000UL                             /**< Default value for CRYPTO_DATA0XOR */
#define _CRYPTO_DATA0XOR_MASK                        0xFFFFFFFFUL                             /**< Mask for CRYPTO_DATA0XOR */
#define _CRYPTO_DATA0XOR_DATA0XOR_SHIFT              0                                        /**< Shift value for CRYPTO_DATA0XOR */
#define _CRYPTO_DATA0XOR_DATA0XOR_MASK               0xFFFFFFFFUL                             /**< Bit mask for CRYPTO_DATA0XOR */
#define _CRYPTO_DATA0XOR_DATA0XOR_DEFAULT            0x00000000UL                             /**< Mode DEFAULT for CRYPTO_DATA0XOR */
#define CRYPTO_DATA0XOR_DATA0XOR_DEFAULT             (_CRYPTO_DATA0XOR_DATA0XOR_DEFAULT << 0) /**< Shifted mode DEFAULT for CRYPTO_DATA0XOR */

/* Bit fields for CRYPTO DATA0BYTE */
#define _CRYPTO_DATA0BYTE_RESETVALUE                 0x00000000UL                               /**< Default value for CRYPTO_DATA0BYTE */
#define _CRYPTO_DATA0BYTE_MASK                       0x000000FFUL                               /**< Mask for CRYPTO_DATA0BYTE */
#define _CRYPTO_DATA0BYTE_DATA0BYTE_SHIFT            0                                          /**< Shift value for CRYPTO_DATA0BYTE */
#define _CRYPTO_DATA0BYTE_DATA0BYTE_MASK             0xFFUL                                     /**< Bit mask for CRYPTO_DATA0BYTE */
#define _CRYPTO_DATA0BYTE_DATA0BYTE_DEFAULT          0x00000000UL                               /**< Mode DEFAULT for CRYPTO_DATA0BYTE */
#define CRYPTO_DATA0BYTE_DATA0BYTE_DEFAULT           (_CRYPTO_DATA0BYTE_DATA0BYTE_DEFAULT << 0) /**< Shifted mode DEFAULT for CRYPTO_DATA0BYTE */

/* Bit fields for CRYPTO DATA1BYTE */
#define _CRYPTO_DATA1BYTE_RESETVALUE                 0x00000000UL                               /**< Default value for CRYPTO_DATA1BYTE */
#define _CRYPTO_DATA1BYTE_MASK                       0x000000FFUL                               /**< Mask for CRYPTO_DATA1BYTE */
#define _CRYPTO_DATA1BYTE_DATA1BYTE_SHIFT            0                                          /**< Shift value for CRYPTO_DATA1BYTE */
#define _CRYPTO_DATA1BYTE_DATA1BYTE_MASK             0xFFUL                                     /**< Bit mask for CRYPTO_DATA1BYTE */
#define _CRYPTO_DATA1BYTE_DATA1BYTE_DEFAULT          0x00000000UL                               /**< Mode DEFAULT for CRYPTO_DATA1BYTE */
#define CRYPTO_DATA1BYTE_DATA1BYTE_DEFAULT           (_CRYPTO_DATA1BYTE_DATA1BYTE_DEFAULT << 0) /**< Shifted mode DEFAULT for CRYPTO_DATA1BYTE */

/* Bit fields for CRYPTO DATA0XORBYTE */
#define _CRYPTO_DATA0XORBYTE_RESETVALUE              0x00000000UL                                     /**< Default value for CRYPTO_DATA0XORBYTE */
#define _CRYPTO_DATA0XORBYTE_MASK                    0x000000FFUL                                     /**< Mask for CRYPTO_DATA0XORBYTE */
#define _CRYPTO_DATA0XORBYTE_DATA0XORBYTE_SHIFT      0                                                /**< Shift value for CRYPTO_DATA0XORBYTE */
#define _CRYPTO_DATA0XORBYTE_DATA0XORBYTE_MASK       0xFFUL                                           /**< Bit mask for CRYPTO_DATA0XORBYTE */
#define _CRYPTO_DATA0XORBYTE_DATA0XORBYTE_DEFAULT    0x00000000UL                                     /**< Mode DEFAULT for CRYPTO_DATA0XORBYTE */
#define CRYPTO_DATA0XORBYTE_DATA0XORBYTE_DEFAULT     (_CRYPTO_DATA0XORBYTE_DATA0XORBYTE_DEFAULT << 0) /**< Shifted mode DEFAULT for CRYPTO_DATA0XORBYTE */

/* Bit fields for CRYPTO DATA0BYTE12 */
#define _CRYPTO_DATA0BYTE12_RESETVALUE               0x00000000UL                                   /**< Default value for CRYPTO_DATA0BYTE12 */
#define _CRYPTO_DATA0BYTE12_MASK                     0x000000FFUL                                   /**< Mask for CRYPTO_DATA0BYTE12 */
#define _CRYPTO_DATA0BYTE12_DATA0BYTE12_SHIFT        0                                              /**< Shift value for CRYPTO_DATA0BYTE12 */
#define _CRYPTO_DATA0BYTE12_DATA0BYTE12_MASK         0xFFUL                                         /**< Bit mask for CRYPTO_DATA0BYTE12 */
#define _CRYPTO_DATA0BYTE12_DATA0BYTE12_DEFAULT      0x00000000UL                                   /**< Mode DEFAULT for CRYPTO_DATA0BYTE12 */
#define CRYPTO_DATA0BYTE12_DATA0BYTE12_DEFAULT       (_CRYPTO_DATA0BYTE12_DATA0BYTE12_DEFAULT << 0) /**< Shifted mode DEFAULT for CRYPTO_DATA0BYTE12 */

/* Bit fields for CRYPTO DATA0BYTE13 */
#define _CRYPTO_DATA0BYTE13_RESETVALUE               0x00000000UL                                   /**< Default value for CRYPTO_DATA0BYTE13 */
#define _CRYPTO_DATA0BYTE13_MASK                     0x000000FFUL                                   /**< Mask for CRYPTO_DATA0BYTE13 */
#define _CRYPTO_DATA0BYTE13_DATA0BYTE13_SHIFT        0                                              /**< Shift value for CRYPTO_DATA0BYTE13 */
#define _CRYPTO_DATA0BYTE13_DATA0BYTE13_MASK         0xFFUL                                         /**< Bit mask for CRYPTO_DATA0BYTE13 */
#define _CRYPTO_DATA0BYTE13_DATA0BYTE13_DEFAULT      0x00000000UL                                   /**< Mode DEFAULT for CRYPTO_DATA0BYTE13 */
#define CRYPTO_DATA0BYTE13_DATA0BYTE13_DEFAULT       (_CRYPTO_DATA0BYTE13_DATA0BYTE13_DEFAULT << 0) /**< Shifted mode DEFAULT for CRYPTO_DATA0BYTE13 */

/* Bit fields for CRYPTO DATA0BYTE14 */
#define _CRYPTO_DATA0BYTE14_RESETVALUE               0x00000000UL                                   /**< Default value for CRYPTO_DATA0BYTE14 */
#define _CRYPTO_DATA0BYTE14_MASK                     0x000000FFUL                                   /**< Mask for CRYPTO_DATA0BYTE14 */
#define _CRYPTO_DATA0BYTE14_DATA0BYTE14_SHIFT        0                                              /**< Shift value for CRYPTO_DATA0BYTE14 */
#define _CRYPTO_DATA0BYTE14_DATA0BYTE14_MASK         0xFFUL                                         /**< Bit mask for CRYPTO_DATA0BYTE14 */
#define _CRYPTO_DATA0BYTE14_DATA0BYTE14_DEFAULT      0x00000000UL                                   /**< Mode DEFAULT for CRYPTO_DATA0BYTE14 */
#define CRYPTO_DATA0BYTE14_DATA0BYTE14_DEFAULT       (_CRYPTO_DATA0BYTE14_DATA0BYTE14_DEFAULT << 0) /**< Shifted mode DEFAULT for CRYPTO_DATA0BYTE14 */

/* Bit fields for CRYPTO DATA0BYTE15 */
#define _CRYPTO_DATA0BYTE15_RESETVALUE               0x00000000UL                                   /**< Default value for CRYPTO_DATA0BYTE15 */
#define _CRYPTO_DATA0BYTE15_MASK                     0x000000FFUL                                   /**< Mask for CRYPTO_DATA0BYTE15 */
#define _CRYPTO_DATA0BYTE15_DATA0BYTE15_SHIFT        0                                              /**< Shift value for CRYPTO_DATA0BYTE15 */
#define _CRYPTO_DATA0BYTE15_DATA0BYTE15_MASK         0xFFUL                                         /**< Bit mask for CRYPTO_DATA0BYTE15 */
#define _CRYPTO_DATA0BYTE15_DATA0BYTE15_DEFAULT      0x00000000UL                                   /**< Mode DEFAULT for CRYPTO_DATA0BYTE15 */
#define CRYPTO_DATA0BYTE15_DATA0BYTE15_DEFAULT       (_CRYPTO_DATA0BYTE15_DATA0BYTE15_DEFAULT << 0) /**< Shifted mode DEFAULT for CRYPTO_DATA0BYTE15 */

/* Bit fields for CRYPTO DDATA0 */
#define _CRYPTO_DDATA0_RESETVALUE                    0x00000000UL                         /**< Default value for CRYPTO_DDATA0 */
#define _CRYPTO_DDATA0_MASK                          0xFFFFFFFFUL                         /**< Mask for CRYPTO_DDATA0 */
#define _CRYPTO_DDATA0_DDATA0_SHIFT                  0                                    /**< Shift value for CRYPTO_DDATA0 */
#define _CRYPTO_DDATA0_DDATA0_MASK                   0xFFFFFFFFUL                         /**< Bit mask for CRYPTO_DDATA0 */
#define _CRYPTO_DDATA0_DDATA0_DEFAULT                0x00000000UL                         /**< Mode DEFAULT for CRYPTO_DDATA0 */
#define CRYPTO_DDATA0_DDATA0_DEFAULT                 (_CRYPTO_DDATA0_DDATA0_DEFAULT << 0) /**< Shifted mode DEFAULT for CRYPTO_DDATA0 */

/* Bit fields for CRYPTO DDATA1 */
#define _CRYPTO_DDATA1_RESETVALUE                    0x00000000UL                         /**< Default value for CRYPTO_DDATA1 */
#define _CRYPTO_DDATA1_MASK                          0xFFFFFFFFUL                         /**< Mask for CRYPTO_DDATA1 */
#define _CRYPTO_DDATA1_DDATA1_SHIFT                  0                                    /**< Shift value for CRYPTO_DDATA1 */
#define _CRYPTO_DDATA1_DDATA1_MASK                   0xFFFFFFFFUL                         /**< Bit mask for CRYPTO_DDATA1 */
#define _CRYPTO_DDATA1_DDATA1_DEFAULT                0x00000000UL                         /**< Mode DEFAULT for CRYPTO_DDATA1 */
#define CRYPTO_DDATA1_DDATA1_DEFAULT                 (_CRYPTO_DDATA1_DDATA1_DEFAULT << 0) /**< Shifted mode DEFAULT for CRYPTO_DDATA1 */

/* Bit fields for CRYPTO DDATA2 */
#define _CRYPTO_DDATA2_RESETVALUE                    0x00000000UL                         /**< Default value for CRYPTO_DDATA2 */
#define _CRYPTO_DDATA2_MASK                          0xFFFFFFFFUL                         /**< Mask for CRYPTO_DDATA2 */
#define _CRYPTO_DDATA2_DDATA2_SHIFT                  0                                    /**< Shift value for CRYPTO_DDATA2 */
#define _CRYPTO_DDATA2_DDATA2_MASK                   0xFFFFFFFFUL                         /**< Bit mask for CRYPTO_DDATA2 */
#define _CRYPTO_DDATA2_DDATA2_DEFAULT                0x00000000UL                         /**< Mode DEFAULT for CRYPTO_DDATA2 */
#define CRYPTO_DDATA2_DDATA2_DEFAULT                 (_CRYPTO_DDATA2_DDATA2_DEFAULT << 0) /**< Shifted mode DEFAULT for CRYPTO_DDATA2 */

/* Bit fields for CRYPTO DDATA3 */
#define _CRYPTO_DDATA3_RESETVALUE                    0x00000000UL                         /**< Default value for CRYPTO_DDATA3 */
#define _CRYPTO_DDATA3_MASK                          0xFFFFFFFFUL                         /**< Mask for CRYPTO_DDATA3 */
#define _CRYPTO_DDATA3_DDATA3_SHIFT                  0                                    /**< Shift value for CRYPTO_DDATA3 */
#define _CRYPTO_DDATA3_DDATA3_MASK                   0xFFFFFFFFUL                         /**< Bit mask for CRYPTO_DDATA3 */
#define _CRYPTO_DDATA3_DDATA3_DEFAULT                0x00000000UL                         /**< Mode DEFAULT for CRYPTO_DDATA3 */
#define CRYPTO_DDATA3_DDATA3_DEFAULT                 (_CRYPTO_DDATA3_DDATA3_DEFAULT << 0) /**< Shifted mode DEFAULT for CRYPTO_DDATA3 */

/* Bit fields for CRYPTO DDATA4 */
#define _CRYPTO_DDATA4_RESETVALUE                    0x00000000UL                         /**< Default value for CRYPTO_DDATA4 */
#define _CRYPTO_DDATA4_MASK                          0xFFFFFFFFUL                         /**< Mask for CRYPTO_DDATA4 */
#define _CRYPTO_DDATA4_DDATA4_SHIFT                  0                                    /**< Shift value for CRYPTO_DDATA4 */
#define _CRYPTO_DDATA4_DDATA4_MASK                   0xFFFFFFFFUL                         /**< Bit mask for CRYPTO_DDATA4 */
#define _CRYPTO_DDATA4_DDATA4_DEFAULT                0x00000000UL                         /**< Mode DEFAULT for CRYPTO_DDATA4 */
#define CRYPTO_DDATA4_DDATA4_DEFAULT                 (_CRYPTO_DDATA4_DDATA4_DEFAULT << 0) /**< Shifted mode DEFAULT for CRYPTO_DDATA4 */

/* Bit fields for CRYPTO DDATA0BIG */
#define _CRYPTO_DDATA0BIG_RESETVALUE                 0x00000000UL                               /**< Default value for CRYPTO_DDATA0BIG */
#define _CRYPTO_DDATA0BIG_MASK                       0xFFFFFFFFUL                               /**< Mask for CRYPTO_DDATA0BIG */
#define _CRYPTO_DDATA0BIG_DDATA0BIG_SHIFT            0                                          /**< Shift value for CRYPTO_DDATA0BIG */
#define _CRYPTO_DDATA0BIG_DDATA0BIG_MASK             0xFFFFFFFFUL                               /**< Bit mask for CRYPTO_DDATA0BIG */
#define _CRYPTO_DDATA0BIG_DDATA0BIG_DEFAULT          0x00000000UL                               /**< Mode DEFAULT for CRYPTO_DDATA0BIG */
#define CRYPTO_DDATA0BIG_DDATA0BIG_DEFAULT           (_CRYPTO_DDATA0BIG_DDATA0BIG_DEFAULT << 0) /**< Shifted mode DEFAULT for CRYPTO_DDATA0BIG */

/* Bit fields for CRYPTO DDATA0BYTE */
#define _CRYPTO_DDATA0BYTE_RESETVALUE                0x00000000UL                                 /**< Default value for CRYPTO_DDATA0BYTE */
#define _CRYPTO_DDATA0BYTE_MASK                      0x000000FFUL                                 /**< Mask for CRYPTO_DDATA0BYTE */
#define _CRYPTO_DDATA0BYTE_DDATA0BYTE_SHIFT          0                                            /**< Shift value for CRYPTO_DDATA0BYTE */
#define _CRYPTO_DDATA0BYTE_DDATA0BYTE_MASK           0xFFUL                                       /**< Bit mask for CRYPTO_DDATA0BYTE */
#define _CRYPTO_DDATA0BYTE_DDATA0BYTE_DEFAULT        0x00000000UL                                 /**< Mode DEFAULT for CRYPTO_DDATA0BYTE */
#define CRYPTO_DDATA0BYTE_DDATA0BYTE_DEFAULT         (_CRYPTO_DDATA0BYTE_DDATA0BYTE_DEFAULT << 0) /**< Shifted mode DEFAULT for CRYPTO_DDATA0BYTE */

/* Bit fields for CRYPTO DDATA1BYTE */
#define _CRYPTO_DDATA1BYTE_RESETVALUE                0x00000000UL                                 /**< Default value for CRYPTO_DDATA1BYTE */
#define _CRYPTO_DDATA1BYTE_MASK                      0x000000FFUL                                 /**< Mask for CRYPTO_DDATA1BYTE */
#define _CRYPTO_DDATA1BYTE_DDATA1BYTE_SHIFT          0                                            /**< Shift value for CRYPTO_DDATA1BYTE */
#define _CRYPTO_DDATA1BYTE_DDATA1BYTE_MASK           0xFFUL                                       /**< Bit mask for CRYPTO_DDATA1BYTE */
#define _CRYPTO_DDATA1BYTE_DDATA1BYTE_DEFAULT        0x00000000UL                                 /**< Mode DEFAULT for CRYPTO_DDATA1BYTE */
#define CRYPTO_DDATA1BYTE_DDATA1BYTE_DEFAULT         (_CRYPTO_DDATA1BYTE_DDATA1BYTE_DEFAULT << 0) /**< Shifted mode DEFAULT for CRYPTO_DDATA1BYTE */

/* Bit fields for CRYPTO DDATA0BYTE32 */
#define _CRYPTO_DDATA0BYTE32_RESETVALUE              0x00000000UL                                     /**< Default value for CRYPTO_DDATA0BYTE32 */
#define _CRYPTO_DDATA0BYTE32_MASK                    0x0000000FUL                                     /**< Mask for CRYPTO_DDATA0BYTE32 */
#define _CRYPTO_DDATA0BYTE32_DDATA0BYTE32_SHIFT      0                                                /**< Shift value for CRYPTO_DDATA0BYTE32 */
#define _CRYPTO_DDATA0BYTE32_DDATA0BYTE32_MASK       0xFUL                                            /**< Bit mask for CRYPTO_DDATA0BYTE32 */
#define _CRYPTO_DDATA0BYTE32_DDATA0BYTE32_DEFAULT    0x00000000UL                                     /**< Mode DEFAULT for CRYPTO_DDATA0BYTE32 */
#define CRYPTO_DDATA0BYTE32_DDATA0BYTE32_DEFAULT     (_CRYPTO_DDATA0BYTE32_DDATA0BYTE32_DEFAULT << 0) /**< Shifted mode DEFAULT for CRYPTO_DDATA0BYTE32 */

/* Bit fields for CRYPTO QDATA0 */
#define _CRYPTO_QDATA0_RESETVALUE                    0x00000000UL                         /**< Default value for CRYPTO_QDATA0 */
#define _CRYPTO_QDATA0_MASK                          0xFFFFFFFFUL                         /**< Mask for CRYPTO_QDATA0 */
#define _CRYPTO_QDATA0_QDATA0_SHIFT                  0                                    /**< Shift value for CRYPTO_QDATA0 */
#define _CRYPTO_QDATA0_QDATA0_MASK                   0xFFFFFFFFUL                         /**< Bit mask for CRYPTO_QDATA0 */
#define _CRYPTO_QDATA0_QDATA0_DEFAULT                0x00000000UL                         /**< Mode DEFAULT for CRYPTO_QDATA0 */
#define CRYPTO_QDATA0_QDATA0_DEFAULT                 (_CRYPTO_QDATA0_QDATA0_DEFAULT << 0) /**< Shifted mode DEFAULT for CRYPTO_QDATA0 */

/* Bit fields for CRYPTO QDATA1 */
#define _CRYPTO_QDATA1_RESETVALUE                    0x00000000UL                         /**< Default value for CRYPTO_QDATA1 */
#define _CRYPTO_QDATA1_MASK                          0xFFFFFFFFUL                         /**< Mask for CRYPTO_QDATA1 */
#define _CRYPTO_QDATA1_QDATA1_SHIFT                  0                                    /**< Shift value for CRYPTO_QDATA1 */
#define _CRYPTO_QDATA1_QDATA1_MASK                   0xFFFFFFFFUL                         /**< Bit mask for CRYPTO_QDATA1 */
#define _CRYPTO_QDATA1_QDATA1_DEFAULT                0x00000000UL                         /**< Mode DEFAULT for CRYPTO_QDATA1 */
#define CRYPTO_QDATA1_QDATA1_DEFAULT                 (_CRYPTO_QDATA1_QDATA1_DEFAULT << 0) /**< Shifted mode DEFAULT for CRYPTO_QDATA1 */

/* Bit fields for CRYPTO QDATA1BIG */
#define _CRYPTO_QDATA1BIG_RESETVALUE                 0x00000000UL                               /**< Default value for CRYPTO_QDATA1BIG */
#define _CRYPTO_QDATA1BIG_MASK                       0xFFFFFFFFUL                               /**< Mask for CRYPTO_QDATA1BIG */
#define _CRYPTO_QDATA1BIG_QDATA1BIG_SHIFT            0                                          /**< Shift value for CRYPTO_QDATA1BIG */
#define _CRYPTO_QDATA1BIG_QDATA1BIG_MASK             0xFFFFFFFFUL                               /**< Bit mask for CRYPTO_QDATA1BIG */
#define _CRYPTO_QDATA1BIG_QDATA1BIG_DEFAULT          0x00000000UL                               /**< Mode DEFAULT for CRYPTO_QDATA1BIG */
#define CRYPTO_QDATA1BIG_QDATA1BIG_DEFAULT           (_CRYPTO_QDATA1BIG_QDATA1BIG_DEFAULT << 0) /**< Shifted mode DEFAULT for CRYPTO_QDATA1BIG */

/* Bit fields for CRYPTO QDATA0BYTE */
#define _CRYPTO_QDATA0BYTE_RESETVALUE                0x00000000UL                                 /**< Default value for CRYPTO_QDATA0BYTE */
#define _CRYPTO_QDATA0BYTE_MASK                      0x000000FFUL                                 /**< Mask for CRYPTO_QDATA0BYTE */
#define _CRYPTO_QDATA0BYTE_QDATA0BYTE_SHIFT          0                                            /**< Shift value for CRYPTO_QDATA0BYTE */
#define _CRYPTO_QDATA0BYTE_QDATA0BYTE_MASK           0xFFUL                                       /**< Bit mask for CRYPTO_QDATA0BYTE */
#define _CRYPTO_QDATA0BYTE_QDATA0BYTE_DEFAULT        0x00000000UL                                 /**< Mode DEFAULT for CRYPTO_QDATA0BYTE */
#define CRYPTO_QDATA0BYTE_QDATA0BYTE_DEFAULT         (_CRYPTO_QDATA0BYTE_QDATA0BYTE_DEFAULT << 0) /**< Shifted mode DEFAULT for CRYPTO_QDATA0BYTE */

/* Bit fields for CRYPTO QDATA1BYTE */
#define _CRYPTO_QDATA1BYTE_RESETVALUE                0x00000000UL                                 /**< Default value for CRYPTO_QDATA1BYTE */
#define _CRYPTO_QDATA1BYTE_MASK                      0x000000FFUL                                 /**< Mask for CRYPTO_QDATA1BYTE */
#define _CRYPTO_QDATA1BYTE_QDATA1BYTE_SHIFT          0                                            /**< Shift value for CRYPTO_QDATA1BYTE */
#define _CRYPTO_QDATA1BYTE_QDATA1BYTE_MASK           0xFFUL                                       /**< Bit mask for CRYPTO_QDATA1BYTE */
#define _CRYPTO_QDATA1BYTE_QDATA1BYTE_DEFAULT        0x00000000UL                                 /**< Mode DEFAULT for CRYPTO_QDATA1BYTE */
#define CRYPTO_QDATA1BYTE_QDATA1BYTE_DEFAULT         (_CRYPTO_QDATA1BYTE_QDATA1BYTE_DEFAULT << 0) /**< Shifted mode DEFAULT for CRYPTO_QDATA1BYTE */

/** @} End of group EFR32FG1P_CRYPTO */
/** @} End of group Parts */

