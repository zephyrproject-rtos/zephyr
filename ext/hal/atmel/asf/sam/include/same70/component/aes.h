/**
 * \file
 *
 * \brief Component description for AES
 *
 * Copyright (c) 2016 Atmel Corporation, a wholly owned subsidiary of Microchip Technology Inc.
 *
 * \license_start
 *
 * \page License
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * \license_stop
 *
 */

#ifndef _SAME70_AES_COMPONENT_H_
#define _SAME70_AES_COMPONENT_H_
#define _SAME70_AES_COMPONENT_         /**< \deprecated  Backward compatibility for ASF */

/** \addtogroup SAME70_AES Advanced Encryption Standard
 *  @{
 */
/* ========================================================================== */
/**  SOFTWARE API DEFINITION FOR AES */
/* ========================================================================== */
#ifndef COMPONENT_TYPEDEF_STYLE
  #define COMPONENT_TYPEDEF_STYLE 'R'  /**< Defines default style of typedefs for the component header files ('R' = RFO, 'N' = NTO)*/
#endif

#define AES_6149                       /**< (AES) Module ID */
#define REV_AES T                      /**< (AES) Module revision */

/* -------- AES_CR : (AES Offset: 0x00) (/W 32) Control Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t START:1;                   /**< bit:      0  Start Processing                         */
    uint32_t :7;                        /**< bit:   1..7  Reserved */
    uint32_t SWRST:1;                   /**< bit:      8  Software Reset                           */
    uint32_t :23;                       /**< bit:  9..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} AES_CR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define AES_CR_OFFSET                       (0x00)                                        /**<  (AES_CR) Control Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define AES_CR_START_Pos                    0                                              /**< (AES_CR) Start Processing Position */
#define AES_CR_START_Msk                    (0x1U << AES_CR_START_Pos)                     /**< (AES_CR) Start Processing Mask */
#define AES_CR_START                        AES_CR_START_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use AES_CR_START_Msk instead */
#define AES_CR_SWRST_Pos                    8                                              /**< (AES_CR) Software Reset Position */
#define AES_CR_SWRST_Msk                    (0x1U << AES_CR_SWRST_Pos)                     /**< (AES_CR) Software Reset Mask */
#define AES_CR_SWRST                        AES_CR_SWRST_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use AES_CR_SWRST_Msk instead */
#define AES_CR_MASK                         (0x101U)                                       /**< \deprecated (AES_CR) Register MASK  (Use AES_CR_Msk instead)  */
#define AES_CR_Msk                          (0x101U)                                       /**< (AES_CR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- AES_MR : (AES Offset: 0x04) (R/W 32) Mode Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t CIPHER:1;                  /**< bit:      0  Processing Mode                          */
    uint32_t GTAGEN:1;                  /**< bit:      1  GCM Automatic Tag Generation Enable      */
    uint32_t :1;                        /**< bit:      2  Reserved */
    uint32_t DUALBUFF:1;                /**< bit:      3  Dual Input Buffer                        */
    uint32_t PROCDLY:4;                 /**< bit:   4..7  Processing Delay                         */
    uint32_t SMOD:2;                    /**< bit:   8..9  Start Mode                               */
    uint32_t KEYSIZE:2;                 /**< bit: 10..11  Key Size                                 */
    uint32_t OPMOD:3;                   /**< bit: 12..14  Operation Mode                           */
    uint32_t LOD:1;                     /**< bit:     15  Last Output Data Mode                    */
    uint32_t CFBS:3;                    /**< bit: 16..18  Cipher Feedback Data Size                */
    uint32_t :1;                        /**< bit:     19  Reserved */
    uint32_t CKEY:4;                    /**< bit: 20..23  Countermeasure Key                       */
    uint32_t :8;                        /**< bit: 24..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} AES_MR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define AES_MR_OFFSET                       (0x04)                                        /**<  (AES_MR) Mode Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define AES_MR_CIPHER_Pos                   0                                              /**< (AES_MR) Processing Mode Position */
#define AES_MR_CIPHER_Msk                   (0x1U << AES_MR_CIPHER_Pos)                    /**< (AES_MR) Processing Mode Mask */
#define AES_MR_CIPHER                       AES_MR_CIPHER_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use AES_MR_CIPHER_Msk instead */
#define AES_MR_GTAGEN_Pos                   1                                              /**< (AES_MR) GCM Automatic Tag Generation Enable Position */
#define AES_MR_GTAGEN_Msk                   (0x1U << AES_MR_GTAGEN_Pos)                    /**< (AES_MR) GCM Automatic Tag Generation Enable Mask */
#define AES_MR_GTAGEN                       AES_MR_GTAGEN_Msk                              /**< \deprecated Old style mask definition for 1 bit bitfield. Use AES_MR_GTAGEN_Msk instead */
#define AES_MR_DUALBUFF_Pos                 3                                              /**< (AES_MR) Dual Input Buffer Position */
#define AES_MR_DUALBUFF_Msk                 (0x1U << AES_MR_DUALBUFF_Pos)                  /**< (AES_MR) Dual Input Buffer Mask */
#define AES_MR_DUALBUFF                     AES_MR_DUALBUFF_Msk                            /**< \deprecated Old style mask definition for 1 bit bitfield. Use AES_MR_DUALBUFF_Msk instead */
#define   AES_MR_DUALBUFF_INACTIVE_Val      (0x0U)                                         /**< (AES_MR) AES_IDATARx cannot be written during processing of previous block.  */
#define   AES_MR_DUALBUFF_ACTIVE_Val        (0x1U)                                         /**< (AES_MR) AES_IDATARx can be written during processing of previous block when SMOD = 2. It speeds up the overall runtime of large files.  */
#define AES_MR_DUALBUFF_INACTIVE            (AES_MR_DUALBUFF_INACTIVE_Val << AES_MR_DUALBUFF_Pos)  /**< (AES_MR) AES_IDATARx cannot be written during processing of previous block. Position  */
#define AES_MR_DUALBUFF_ACTIVE              (AES_MR_DUALBUFF_ACTIVE_Val << AES_MR_DUALBUFF_Pos)  /**< (AES_MR) AES_IDATARx can be written during processing of previous block when SMOD = 2. It speeds up the overall runtime of large files. Position  */
#define AES_MR_PROCDLY_Pos                  4                                              /**< (AES_MR) Processing Delay Position */
#define AES_MR_PROCDLY_Msk                  (0xFU << AES_MR_PROCDLY_Pos)                   /**< (AES_MR) Processing Delay Mask */
#define AES_MR_PROCDLY(value)               (AES_MR_PROCDLY_Msk & ((value) << AES_MR_PROCDLY_Pos))
#define AES_MR_SMOD_Pos                     8                                              /**< (AES_MR) Start Mode Position */
#define AES_MR_SMOD_Msk                     (0x3U << AES_MR_SMOD_Pos)                      /**< (AES_MR) Start Mode Mask */
#define AES_MR_SMOD(value)                  (AES_MR_SMOD_Msk & ((value) << AES_MR_SMOD_Pos))
#define   AES_MR_SMOD_MANUAL_START_Val      (0x0U)                                         /**< (AES_MR) Manual Mode  */
#define   AES_MR_SMOD_AUTO_START_Val        (0x1U)                                         /**< (AES_MR) Auto Mode  */
#define   AES_MR_SMOD_IDATAR0_START_Val     (0x2U)                                         /**< (AES_MR) AES_IDATAR0 access only Auto Mode (DMA)  */
#define AES_MR_SMOD_MANUAL_START            (AES_MR_SMOD_MANUAL_START_Val << AES_MR_SMOD_Pos)  /**< (AES_MR) Manual Mode Position  */
#define AES_MR_SMOD_AUTO_START              (AES_MR_SMOD_AUTO_START_Val << AES_MR_SMOD_Pos)  /**< (AES_MR) Auto Mode Position  */
#define AES_MR_SMOD_IDATAR0_START           (AES_MR_SMOD_IDATAR0_START_Val << AES_MR_SMOD_Pos)  /**< (AES_MR) AES_IDATAR0 access only Auto Mode (DMA) Position  */
#define AES_MR_KEYSIZE_Pos                  10                                             /**< (AES_MR) Key Size Position */
#define AES_MR_KEYSIZE_Msk                  (0x3U << AES_MR_KEYSIZE_Pos)                   /**< (AES_MR) Key Size Mask */
#define AES_MR_KEYSIZE(value)               (AES_MR_KEYSIZE_Msk & ((value) << AES_MR_KEYSIZE_Pos))
#define   AES_MR_KEYSIZE_AES128_Val         (0x0U)                                         /**< (AES_MR) AES Key Size is 128 bits  */
#define   AES_MR_KEYSIZE_AES192_Val         (0x1U)                                         /**< (AES_MR) AES Key Size is 192 bits  */
#define   AES_MR_KEYSIZE_AES256_Val         (0x2U)                                         /**< (AES_MR) AES Key Size is 256 bits  */
#define AES_MR_KEYSIZE_AES128               (AES_MR_KEYSIZE_AES128_Val << AES_MR_KEYSIZE_Pos)  /**< (AES_MR) AES Key Size is 128 bits Position  */
#define AES_MR_KEYSIZE_AES192               (AES_MR_KEYSIZE_AES192_Val << AES_MR_KEYSIZE_Pos)  /**< (AES_MR) AES Key Size is 192 bits Position  */
#define AES_MR_KEYSIZE_AES256               (AES_MR_KEYSIZE_AES256_Val << AES_MR_KEYSIZE_Pos)  /**< (AES_MR) AES Key Size is 256 bits Position  */
#define AES_MR_OPMOD_Pos                    12                                             /**< (AES_MR) Operation Mode Position */
#define AES_MR_OPMOD_Msk                    (0x7U << AES_MR_OPMOD_Pos)                     /**< (AES_MR) Operation Mode Mask */
#define AES_MR_OPMOD(value)                 (AES_MR_OPMOD_Msk & ((value) << AES_MR_OPMOD_Pos))
#define   AES_MR_OPMOD_ECB_Val              (0x0U)                                         /**< (AES_MR) ECB: Electronic Code Book mode  */
#define   AES_MR_OPMOD_CBC_Val              (0x1U)                                         /**< (AES_MR) CBC: Cipher Block Chaining mode  */
#define   AES_MR_OPMOD_OFB_Val              (0x2U)                                         /**< (AES_MR) OFB: Output Feedback mode  */
#define   AES_MR_OPMOD_CFB_Val              (0x3U)                                         /**< (AES_MR) CFB: Cipher Feedback mode  */
#define   AES_MR_OPMOD_CTR_Val              (0x4U)                                         /**< (AES_MR) CTR: Counter mode (16-bit internal counter)  */
#define   AES_MR_OPMOD_GCM_Val              (0x5U)                                         /**< (AES_MR) GCM: Galois/Counter mode  */
#define AES_MR_OPMOD_ECB                    (AES_MR_OPMOD_ECB_Val << AES_MR_OPMOD_Pos)     /**< (AES_MR) ECB: Electronic Code Book mode Position  */
#define AES_MR_OPMOD_CBC                    (AES_MR_OPMOD_CBC_Val << AES_MR_OPMOD_Pos)     /**< (AES_MR) CBC: Cipher Block Chaining mode Position  */
#define AES_MR_OPMOD_OFB                    (AES_MR_OPMOD_OFB_Val << AES_MR_OPMOD_Pos)     /**< (AES_MR) OFB: Output Feedback mode Position  */
#define AES_MR_OPMOD_CFB                    (AES_MR_OPMOD_CFB_Val << AES_MR_OPMOD_Pos)     /**< (AES_MR) CFB: Cipher Feedback mode Position  */
#define AES_MR_OPMOD_CTR                    (AES_MR_OPMOD_CTR_Val << AES_MR_OPMOD_Pos)     /**< (AES_MR) CTR: Counter mode (16-bit internal counter) Position  */
#define AES_MR_OPMOD_GCM                    (AES_MR_OPMOD_GCM_Val << AES_MR_OPMOD_Pos)     /**< (AES_MR) GCM: Galois/Counter mode Position  */
#define AES_MR_LOD_Pos                      15                                             /**< (AES_MR) Last Output Data Mode Position */
#define AES_MR_LOD_Msk                      (0x1U << AES_MR_LOD_Pos)                       /**< (AES_MR) Last Output Data Mode Mask */
#define AES_MR_LOD                          AES_MR_LOD_Msk                                 /**< \deprecated Old style mask definition for 1 bit bitfield. Use AES_MR_LOD_Msk instead */
#define AES_MR_CFBS_Pos                     16                                             /**< (AES_MR) Cipher Feedback Data Size Position */
#define AES_MR_CFBS_Msk                     (0x7U << AES_MR_CFBS_Pos)                      /**< (AES_MR) Cipher Feedback Data Size Mask */
#define AES_MR_CFBS(value)                  (AES_MR_CFBS_Msk & ((value) << AES_MR_CFBS_Pos))
#define   AES_MR_CFBS_SIZE_128BIT_Val       (0x0U)                                         /**< (AES_MR) 128-bit  */
#define   AES_MR_CFBS_SIZE_64BIT_Val        (0x1U)                                         /**< (AES_MR) 64-bit  */
#define   AES_MR_CFBS_SIZE_32BIT_Val        (0x2U)                                         /**< (AES_MR) 32-bit  */
#define   AES_MR_CFBS_SIZE_16BIT_Val        (0x3U)                                         /**< (AES_MR) 16-bit  */
#define   AES_MR_CFBS_SIZE_8BIT_Val         (0x4U)                                         /**< (AES_MR) 8-bit  */
#define AES_MR_CFBS_SIZE_128BIT             (AES_MR_CFBS_SIZE_128BIT_Val << AES_MR_CFBS_Pos)  /**< (AES_MR) 128-bit Position  */
#define AES_MR_CFBS_SIZE_64BIT              (AES_MR_CFBS_SIZE_64BIT_Val << AES_MR_CFBS_Pos)  /**< (AES_MR) 64-bit Position  */
#define AES_MR_CFBS_SIZE_32BIT              (AES_MR_CFBS_SIZE_32BIT_Val << AES_MR_CFBS_Pos)  /**< (AES_MR) 32-bit Position  */
#define AES_MR_CFBS_SIZE_16BIT              (AES_MR_CFBS_SIZE_16BIT_Val << AES_MR_CFBS_Pos)  /**< (AES_MR) 16-bit Position  */
#define AES_MR_CFBS_SIZE_8BIT               (AES_MR_CFBS_SIZE_8BIT_Val << AES_MR_CFBS_Pos)  /**< (AES_MR) 8-bit Position  */
#define AES_MR_CKEY_Pos                     20                                             /**< (AES_MR) Countermeasure Key Position */
#define AES_MR_CKEY_Msk                     (0xFU << AES_MR_CKEY_Pos)                      /**< (AES_MR) Countermeasure Key Mask */
#define AES_MR_CKEY(value)                  (AES_MR_CKEY_Msk & ((value) << AES_MR_CKEY_Pos))
#define   AES_MR_CKEY_PASSWD_Val            (0xEU)                                         /**< (AES_MR) This field must be written with 0xE to allow CMTYPx bit configuration changes. Any other values will abort the write operation in CMTYPx bits.Always reads as 0.  */
#define AES_MR_CKEY_PASSWD                  (AES_MR_CKEY_PASSWD_Val << AES_MR_CKEY_Pos)    /**< (AES_MR) This field must be written with 0xE to allow CMTYPx bit configuration changes. Any other values will abort the write operation in CMTYPx bits.Always reads as 0. Position  */
#define AES_MR_MASK                         (0xF7FFFBU)                                    /**< \deprecated (AES_MR) Register MASK  (Use AES_MR_Msk instead)  */
#define AES_MR_Msk                          (0xF7FFFBU)                                    /**< (AES_MR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- AES_IER : (AES Offset: 0x10) (/W 32) Interrupt Enable Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t DATRDY:1;                  /**< bit:      0  Data Ready Interrupt Enable              */
    uint32_t :7;                        /**< bit:   1..7  Reserved */
    uint32_t URAD:1;                    /**< bit:      8  Unspecified Register Access Detection Interrupt Enable */
    uint32_t :7;                        /**< bit:  9..15  Reserved */
    uint32_t TAGRDY:1;                  /**< bit:     16  GCM Tag Ready Interrupt Enable           */
    uint32_t :15;                       /**< bit: 17..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} AES_IER_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define AES_IER_OFFSET                      (0x10)                                        /**<  (AES_IER) Interrupt Enable Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define AES_IER_DATRDY_Pos                  0                                              /**< (AES_IER) Data Ready Interrupt Enable Position */
#define AES_IER_DATRDY_Msk                  (0x1U << AES_IER_DATRDY_Pos)                   /**< (AES_IER) Data Ready Interrupt Enable Mask */
#define AES_IER_DATRDY                      AES_IER_DATRDY_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use AES_IER_DATRDY_Msk instead */
#define AES_IER_URAD_Pos                    8                                              /**< (AES_IER) Unspecified Register Access Detection Interrupt Enable Position */
#define AES_IER_URAD_Msk                    (0x1U << AES_IER_URAD_Pos)                     /**< (AES_IER) Unspecified Register Access Detection Interrupt Enable Mask */
#define AES_IER_URAD                        AES_IER_URAD_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use AES_IER_URAD_Msk instead */
#define AES_IER_TAGRDY_Pos                  16                                             /**< (AES_IER) GCM Tag Ready Interrupt Enable Position */
#define AES_IER_TAGRDY_Msk                  (0x1U << AES_IER_TAGRDY_Pos)                   /**< (AES_IER) GCM Tag Ready Interrupt Enable Mask */
#define AES_IER_TAGRDY                      AES_IER_TAGRDY_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use AES_IER_TAGRDY_Msk instead */
#define AES_IER_MASK                        (0x10101U)                                     /**< \deprecated (AES_IER) Register MASK  (Use AES_IER_Msk instead)  */
#define AES_IER_Msk                         (0x10101U)                                     /**< (AES_IER) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- AES_IDR : (AES Offset: 0x14) (/W 32) Interrupt Disable Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t DATRDY:1;                  /**< bit:      0  Data Ready Interrupt Disable             */
    uint32_t :7;                        /**< bit:   1..7  Reserved */
    uint32_t URAD:1;                    /**< bit:      8  Unspecified Register Access Detection Interrupt Disable */
    uint32_t :7;                        /**< bit:  9..15  Reserved */
    uint32_t TAGRDY:1;                  /**< bit:     16  GCM Tag Ready Interrupt Disable          */
    uint32_t :15;                       /**< bit: 17..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} AES_IDR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define AES_IDR_OFFSET                      (0x14)                                        /**<  (AES_IDR) Interrupt Disable Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define AES_IDR_DATRDY_Pos                  0                                              /**< (AES_IDR) Data Ready Interrupt Disable Position */
#define AES_IDR_DATRDY_Msk                  (0x1U << AES_IDR_DATRDY_Pos)                   /**< (AES_IDR) Data Ready Interrupt Disable Mask */
#define AES_IDR_DATRDY                      AES_IDR_DATRDY_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use AES_IDR_DATRDY_Msk instead */
#define AES_IDR_URAD_Pos                    8                                              /**< (AES_IDR) Unspecified Register Access Detection Interrupt Disable Position */
#define AES_IDR_URAD_Msk                    (0x1U << AES_IDR_URAD_Pos)                     /**< (AES_IDR) Unspecified Register Access Detection Interrupt Disable Mask */
#define AES_IDR_URAD                        AES_IDR_URAD_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use AES_IDR_URAD_Msk instead */
#define AES_IDR_TAGRDY_Pos                  16                                             /**< (AES_IDR) GCM Tag Ready Interrupt Disable Position */
#define AES_IDR_TAGRDY_Msk                  (0x1U << AES_IDR_TAGRDY_Pos)                   /**< (AES_IDR) GCM Tag Ready Interrupt Disable Mask */
#define AES_IDR_TAGRDY                      AES_IDR_TAGRDY_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use AES_IDR_TAGRDY_Msk instead */
#define AES_IDR_MASK                        (0x10101U)                                     /**< \deprecated (AES_IDR) Register MASK  (Use AES_IDR_Msk instead)  */
#define AES_IDR_Msk                         (0x10101U)                                     /**< (AES_IDR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- AES_IMR : (AES Offset: 0x18) (R/ 32) Interrupt Mask Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t DATRDY:1;                  /**< bit:      0  Data Ready Interrupt Mask                */
    uint32_t :7;                        /**< bit:   1..7  Reserved */
    uint32_t URAD:1;                    /**< bit:      8  Unspecified Register Access Detection Interrupt Mask */
    uint32_t :7;                        /**< bit:  9..15  Reserved */
    uint32_t TAGRDY:1;                  /**< bit:     16  GCM Tag Ready Interrupt Mask             */
    uint32_t :15;                       /**< bit: 17..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} AES_IMR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define AES_IMR_OFFSET                      (0x18)                                        /**<  (AES_IMR) Interrupt Mask Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define AES_IMR_DATRDY_Pos                  0                                              /**< (AES_IMR) Data Ready Interrupt Mask Position */
#define AES_IMR_DATRDY_Msk                  (0x1U << AES_IMR_DATRDY_Pos)                   /**< (AES_IMR) Data Ready Interrupt Mask Mask */
#define AES_IMR_DATRDY                      AES_IMR_DATRDY_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use AES_IMR_DATRDY_Msk instead */
#define AES_IMR_URAD_Pos                    8                                              /**< (AES_IMR) Unspecified Register Access Detection Interrupt Mask Position */
#define AES_IMR_URAD_Msk                    (0x1U << AES_IMR_URAD_Pos)                     /**< (AES_IMR) Unspecified Register Access Detection Interrupt Mask Mask */
#define AES_IMR_URAD                        AES_IMR_URAD_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use AES_IMR_URAD_Msk instead */
#define AES_IMR_TAGRDY_Pos                  16                                             /**< (AES_IMR) GCM Tag Ready Interrupt Mask Position */
#define AES_IMR_TAGRDY_Msk                  (0x1U << AES_IMR_TAGRDY_Pos)                   /**< (AES_IMR) GCM Tag Ready Interrupt Mask Mask */
#define AES_IMR_TAGRDY                      AES_IMR_TAGRDY_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use AES_IMR_TAGRDY_Msk instead */
#define AES_IMR_MASK                        (0x10101U)                                     /**< \deprecated (AES_IMR) Register MASK  (Use AES_IMR_Msk instead)  */
#define AES_IMR_Msk                         (0x10101U)                                     /**< (AES_IMR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- AES_ISR : (AES Offset: 0x1c) (R/ 32) Interrupt Status Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t DATRDY:1;                  /**< bit:      0  Data Ready (cleared by setting bit START or bit SWRST in AES_CR or by reading AES_ODATARx) */
    uint32_t :7;                        /**< bit:   1..7  Reserved */
    uint32_t URAD:1;                    /**< bit:      8  Unspecified Register Access Detection Status (cleared by writing SWRST in AES_CR) */
    uint32_t :3;                        /**< bit:  9..11  Reserved */
    uint32_t URAT:4;                    /**< bit: 12..15  Unspecified Register Access (cleared by writing SWRST in AES_CR) */
    uint32_t TAGRDY:1;                  /**< bit:     16  GCM Tag Ready                            */
    uint32_t :15;                       /**< bit: 17..31  Reserved */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} AES_ISR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define AES_ISR_OFFSET                      (0x1C)                                        /**<  (AES_ISR) Interrupt Status Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define AES_ISR_DATRDY_Pos                  0                                              /**< (AES_ISR) Data Ready (cleared by setting bit START or bit SWRST in AES_CR or by reading AES_ODATARx) Position */
#define AES_ISR_DATRDY_Msk                  (0x1U << AES_ISR_DATRDY_Pos)                   /**< (AES_ISR) Data Ready (cleared by setting bit START or bit SWRST in AES_CR or by reading AES_ODATARx) Mask */
#define AES_ISR_DATRDY                      AES_ISR_DATRDY_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use AES_ISR_DATRDY_Msk instead */
#define AES_ISR_URAD_Pos                    8                                              /**< (AES_ISR) Unspecified Register Access Detection Status (cleared by writing SWRST in AES_CR) Position */
#define AES_ISR_URAD_Msk                    (0x1U << AES_ISR_URAD_Pos)                     /**< (AES_ISR) Unspecified Register Access Detection Status (cleared by writing SWRST in AES_CR) Mask */
#define AES_ISR_URAD                        AES_ISR_URAD_Msk                               /**< \deprecated Old style mask definition for 1 bit bitfield. Use AES_ISR_URAD_Msk instead */
#define AES_ISR_URAT_Pos                    12                                             /**< (AES_ISR) Unspecified Register Access (cleared by writing SWRST in AES_CR) Position */
#define AES_ISR_URAT_Msk                    (0xFU << AES_ISR_URAT_Pos)                     /**< (AES_ISR) Unspecified Register Access (cleared by writing SWRST in AES_CR) Mask */
#define AES_ISR_URAT(value)                 (AES_ISR_URAT_Msk & ((value) << AES_ISR_URAT_Pos))
#define   AES_ISR_URAT_IDR_WR_PROCESSING_Val (0x0U)                                         /**< (AES_ISR) Input Data Register written during the data processing when SMOD = 0x2 mode.  */
#define   AES_ISR_URAT_ODR_RD_PROCESSING_Val (0x1U)                                         /**< (AES_ISR) Output Data Register read during the data processing.  */
#define   AES_ISR_URAT_MR_WR_PROCESSING_Val (0x2U)                                         /**< (AES_ISR) Mode Register written during the data processing.  */
#define   AES_ISR_URAT_ODR_RD_SUBKGEN_Val   (0x3U)                                         /**< (AES_ISR) Output Data Register read during the sub-keys generation.  */
#define   AES_ISR_URAT_MR_WR_SUBKGEN_Val    (0x4U)                                         /**< (AES_ISR) Mode Register written during the sub-keys generation.  */
#define   AES_ISR_URAT_WOR_RD_ACCESS_Val    (0x5U)                                         /**< (AES_ISR) Write-only register read access.  */
#define AES_ISR_URAT_IDR_WR_PROCESSING      (AES_ISR_URAT_IDR_WR_PROCESSING_Val << AES_ISR_URAT_Pos)  /**< (AES_ISR) Input Data Register written during the data processing when SMOD = 0x2 mode. Position  */
#define AES_ISR_URAT_ODR_RD_PROCESSING      (AES_ISR_URAT_ODR_RD_PROCESSING_Val << AES_ISR_URAT_Pos)  /**< (AES_ISR) Output Data Register read during the data processing. Position  */
#define AES_ISR_URAT_MR_WR_PROCESSING       (AES_ISR_URAT_MR_WR_PROCESSING_Val << AES_ISR_URAT_Pos)  /**< (AES_ISR) Mode Register written during the data processing. Position  */
#define AES_ISR_URAT_ODR_RD_SUBKGEN         (AES_ISR_URAT_ODR_RD_SUBKGEN_Val << AES_ISR_URAT_Pos)  /**< (AES_ISR) Output Data Register read during the sub-keys generation. Position  */
#define AES_ISR_URAT_MR_WR_SUBKGEN          (AES_ISR_URAT_MR_WR_SUBKGEN_Val << AES_ISR_URAT_Pos)  /**< (AES_ISR) Mode Register written during the sub-keys generation. Position  */
#define AES_ISR_URAT_WOR_RD_ACCESS          (AES_ISR_URAT_WOR_RD_ACCESS_Val << AES_ISR_URAT_Pos)  /**< (AES_ISR) Write-only register read access. Position  */
#define AES_ISR_TAGRDY_Pos                  16                                             /**< (AES_ISR) GCM Tag Ready Position */
#define AES_ISR_TAGRDY_Msk                  (0x1U << AES_ISR_TAGRDY_Pos)                   /**< (AES_ISR) GCM Tag Ready Mask */
#define AES_ISR_TAGRDY                      AES_ISR_TAGRDY_Msk                             /**< \deprecated Old style mask definition for 1 bit bitfield. Use AES_ISR_TAGRDY_Msk instead */
#define AES_ISR_MASK                        (0x1F101U)                                     /**< \deprecated (AES_ISR) Register MASK  (Use AES_ISR_Msk instead)  */
#define AES_ISR_Msk                         (0x1F101U)                                     /**< (AES_ISR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- AES_KEYWR : (AES Offset: 0x20) (/W 32) Key Word Register 0 -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t KEYW:32;                   /**< bit:  0..31  Key Word                                 */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} AES_KEYWR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define AES_KEYWR_OFFSET                    (0x20)                                        /**<  (AES_KEYWR) Key Word Register 0  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define AES_KEYWR_KEYW_Pos                  0                                              /**< (AES_KEYWR) Key Word Position */
#define AES_KEYWR_KEYW_Msk                  (0xFFFFFFFFU << AES_KEYWR_KEYW_Pos)            /**< (AES_KEYWR) Key Word Mask */
#define AES_KEYWR_KEYW(value)               (AES_KEYWR_KEYW_Msk & ((value) << AES_KEYWR_KEYW_Pos))
#define AES_KEYWR_MASK                      (0xFFFFFFFFU)                                  /**< \deprecated (AES_KEYWR) Register MASK  (Use AES_KEYWR_Msk instead)  */
#define AES_KEYWR_Msk                       (0xFFFFFFFFU)                                  /**< (AES_KEYWR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- AES_IDATAR : (AES Offset: 0x40) (/W 32) Input Data Register 0 -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t IDATA:32;                  /**< bit:  0..31  Input Data Word                          */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} AES_IDATAR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define AES_IDATAR_OFFSET                   (0x40)                                        /**<  (AES_IDATAR) Input Data Register 0  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define AES_IDATAR_IDATA_Pos                0                                              /**< (AES_IDATAR) Input Data Word Position */
#define AES_IDATAR_IDATA_Msk                (0xFFFFFFFFU << AES_IDATAR_IDATA_Pos)          /**< (AES_IDATAR) Input Data Word Mask */
#define AES_IDATAR_IDATA(value)             (AES_IDATAR_IDATA_Msk & ((value) << AES_IDATAR_IDATA_Pos))
#define AES_IDATAR_MASK                     (0xFFFFFFFFU)                                  /**< \deprecated (AES_IDATAR) Register MASK  (Use AES_IDATAR_Msk instead)  */
#define AES_IDATAR_Msk                      (0xFFFFFFFFU)                                  /**< (AES_IDATAR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- AES_ODATAR : (AES Offset: 0x50) (R/ 32) Output Data Register 0 -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t ODATA:32;                  /**< bit:  0..31  Output Data                              */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} AES_ODATAR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define AES_ODATAR_OFFSET                   (0x50)                                        /**<  (AES_ODATAR) Output Data Register 0  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define AES_ODATAR_ODATA_Pos                0                                              /**< (AES_ODATAR) Output Data Position */
#define AES_ODATAR_ODATA_Msk                (0xFFFFFFFFU << AES_ODATAR_ODATA_Pos)          /**< (AES_ODATAR) Output Data Mask */
#define AES_ODATAR_ODATA(value)             (AES_ODATAR_ODATA_Msk & ((value) << AES_ODATAR_ODATA_Pos))
#define AES_ODATAR_MASK                     (0xFFFFFFFFU)                                  /**< \deprecated (AES_ODATAR) Register MASK  (Use AES_ODATAR_Msk instead)  */
#define AES_ODATAR_Msk                      (0xFFFFFFFFU)                                  /**< (AES_ODATAR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- AES_IVR : (AES Offset: 0x60) (/W 32) Initialization Vector Register 0 -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t IV:32;                     /**< bit:  0..31  Initialization Vector                    */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} AES_IVR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define AES_IVR_OFFSET                      (0x60)                                        /**<  (AES_IVR) Initialization Vector Register 0  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define AES_IVR_IV_Pos                      0                                              /**< (AES_IVR) Initialization Vector Position */
#define AES_IVR_IV_Msk                      (0xFFFFFFFFU << AES_IVR_IV_Pos)                /**< (AES_IVR) Initialization Vector Mask */
#define AES_IVR_IV(value)                   (AES_IVR_IV_Msk & ((value) << AES_IVR_IV_Pos))
#define AES_IVR_MASK                        (0xFFFFFFFFU)                                  /**< \deprecated (AES_IVR) Register MASK  (Use AES_IVR_Msk instead)  */
#define AES_IVR_Msk                         (0xFFFFFFFFU)                                  /**< (AES_IVR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- AES_AADLENR : (AES Offset: 0x70) (R/W 32) Additional Authenticated Data Length Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t AADLEN:32;                 /**< bit:  0..31  Additional Authenticated Data Length     */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} AES_AADLENR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define AES_AADLENR_OFFSET                  (0x70)                                        /**<  (AES_AADLENR) Additional Authenticated Data Length Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define AES_AADLENR_AADLEN_Pos              0                                              /**< (AES_AADLENR) Additional Authenticated Data Length Position */
#define AES_AADLENR_AADLEN_Msk              (0xFFFFFFFFU << AES_AADLENR_AADLEN_Pos)        /**< (AES_AADLENR) Additional Authenticated Data Length Mask */
#define AES_AADLENR_AADLEN(value)           (AES_AADLENR_AADLEN_Msk & ((value) << AES_AADLENR_AADLEN_Pos))
#define AES_AADLENR_MASK                    (0xFFFFFFFFU)                                  /**< \deprecated (AES_AADLENR) Register MASK  (Use AES_AADLENR_Msk instead)  */
#define AES_AADLENR_Msk                     (0xFFFFFFFFU)                                  /**< (AES_AADLENR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- AES_CLENR : (AES Offset: 0x74) (R/W 32) Plaintext/Ciphertext Length Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t CLEN:32;                   /**< bit:  0..31  Plaintext/Ciphertext Length              */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} AES_CLENR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define AES_CLENR_OFFSET                    (0x74)                                        /**<  (AES_CLENR) Plaintext/Ciphertext Length Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define AES_CLENR_CLEN_Pos                  0                                              /**< (AES_CLENR) Plaintext/Ciphertext Length Position */
#define AES_CLENR_CLEN_Msk                  (0xFFFFFFFFU << AES_CLENR_CLEN_Pos)            /**< (AES_CLENR) Plaintext/Ciphertext Length Mask */
#define AES_CLENR_CLEN(value)               (AES_CLENR_CLEN_Msk & ((value) << AES_CLENR_CLEN_Pos))
#define AES_CLENR_MASK                      (0xFFFFFFFFU)                                  /**< \deprecated (AES_CLENR) Register MASK  (Use AES_CLENR_Msk instead)  */
#define AES_CLENR_Msk                       (0xFFFFFFFFU)                                  /**< (AES_CLENR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- AES_GHASHR : (AES Offset: 0x78) (R/W 32) GCM Intermediate Hash Word Register 0 -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t GHASH:32;                  /**< bit:  0..31  Intermediate GCM Hash Word x             */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} AES_GHASHR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define AES_GHASHR_OFFSET                   (0x78)                                        /**<  (AES_GHASHR) GCM Intermediate Hash Word Register 0  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define AES_GHASHR_GHASH_Pos                0                                              /**< (AES_GHASHR) Intermediate GCM Hash Word x Position */
#define AES_GHASHR_GHASH_Msk                (0xFFFFFFFFU << AES_GHASHR_GHASH_Pos)          /**< (AES_GHASHR) Intermediate GCM Hash Word x Mask */
#define AES_GHASHR_GHASH(value)             (AES_GHASHR_GHASH_Msk & ((value) << AES_GHASHR_GHASH_Pos))
#define AES_GHASHR_MASK                     (0xFFFFFFFFU)                                  /**< \deprecated (AES_GHASHR) Register MASK  (Use AES_GHASHR_Msk instead)  */
#define AES_GHASHR_Msk                      (0xFFFFFFFFU)                                  /**< (AES_GHASHR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- AES_TAGR : (AES Offset: 0x88) (R/ 32) GCM Authentication Tag Word Register 0 -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t TAG:32;                    /**< bit:  0..31  GCM Authentication Tag x                 */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} AES_TAGR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define AES_TAGR_OFFSET                     (0x88)                                        /**<  (AES_TAGR) GCM Authentication Tag Word Register 0  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define AES_TAGR_TAG_Pos                    0                                              /**< (AES_TAGR) GCM Authentication Tag x Position */
#define AES_TAGR_TAG_Msk                    (0xFFFFFFFFU << AES_TAGR_TAG_Pos)              /**< (AES_TAGR) GCM Authentication Tag x Mask */
#define AES_TAGR_TAG(value)                 (AES_TAGR_TAG_Msk & ((value) << AES_TAGR_TAG_Pos))
#define AES_TAGR_MASK                       (0xFFFFFFFFU)                                  /**< \deprecated (AES_TAGR) Register MASK  (Use AES_TAGR_Msk instead)  */
#define AES_TAGR_Msk                        (0xFFFFFFFFU)                                  /**< (AES_TAGR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- AES_CTRR : (AES Offset: 0x98) (R/ 32) GCM Encryption Counter Value Register -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t CTR:32;                    /**< bit:  0..31  GCM Encryption Counter                   */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} AES_CTRR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define AES_CTRR_OFFSET                     (0x98)                                        /**<  (AES_CTRR) GCM Encryption Counter Value Register  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define AES_CTRR_CTR_Pos                    0                                              /**< (AES_CTRR) GCM Encryption Counter Position */
#define AES_CTRR_CTR_Msk                    (0xFFFFFFFFU << AES_CTRR_CTR_Pos)              /**< (AES_CTRR) GCM Encryption Counter Mask */
#define AES_CTRR_CTR(value)                 (AES_CTRR_CTR_Msk & ((value) << AES_CTRR_CTR_Pos))
#define AES_CTRR_MASK                       (0xFFFFFFFFU)                                  /**< \deprecated (AES_CTRR) Register MASK  (Use AES_CTRR_Msk instead)  */
#define AES_CTRR_Msk                        (0xFFFFFFFFU)                                  /**< (AES_CTRR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/* -------- AES_GCMHR : (AES Offset: 0x9c) (R/W 32) GCM H Word Register 0 -------- */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { 
  struct {
    uint32_t H:32;                      /**< bit:  0..31  GCM H Word x                             */
  } bit;                                /**< Structure used for bit  access */
  uint32_t reg;                         /**< Type used for register access */
} AES_GCMHR_Type;
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#define AES_GCMHR_OFFSET                    (0x9C)                                        /**<  (AES_GCMHR) GCM H Word Register 0  Offset */
#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define AES_GCMHR_H_Pos                     0                                              /**< (AES_GCMHR) GCM H Word x Position */
#define AES_GCMHR_H_Msk                     (0xFFFFFFFFU << AES_GCMHR_H_Pos)               /**< (AES_GCMHR) GCM H Word x Mask */
#define AES_GCMHR_H(value)                  (AES_GCMHR_H_Msk & ((value) << AES_GCMHR_H_Pos))
#define AES_GCMHR_MASK                      (0xFFFFFFFFU)                                  /**< \deprecated (AES_GCMHR) Register MASK  (Use AES_GCMHR_Msk instead)  */
#define AES_GCMHR_Msk                       (0xFFFFFFFFU)                                  /**< (AES_GCMHR) Register Mask  */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
#if COMPONENT_TYPEDEF_STYLE == 'R'
/** \brief AES hardware registers */
typedef struct {  
  __O  uint32_t AES_CR;         /**< (AES Offset: 0x00) Control Register */
  __IO uint32_t AES_MR;         /**< (AES Offset: 0x04) Mode Register */
  __I  uint32_t Reserved1[2];
  __O  uint32_t AES_IER;        /**< (AES Offset: 0x10) Interrupt Enable Register */
  __O  uint32_t AES_IDR;        /**< (AES Offset: 0x14) Interrupt Disable Register */
  __I  uint32_t AES_IMR;        /**< (AES Offset: 0x18) Interrupt Mask Register */
  __I  uint32_t AES_ISR;        /**< (AES Offset: 0x1C) Interrupt Status Register */
  __O  uint32_t AES_KEYWR[8];   /**< (AES Offset: 0x20) Key Word Register 0 */
  __O  uint32_t AES_IDATAR[4];  /**< (AES Offset: 0x40) Input Data Register 0 */
  __I  uint32_t AES_ODATAR[4];  /**< (AES Offset: 0x50) Output Data Register 0 */
  __O  uint32_t AES_IVR[4];     /**< (AES Offset: 0x60) Initialization Vector Register 0 */
  __IO uint32_t AES_AADLENR;    /**< (AES Offset: 0x70) Additional Authenticated Data Length Register */
  __IO uint32_t AES_CLENR;      /**< (AES Offset: 0x74) Plaintext/Ciphertext Length Register */
  __IO uint32_t AES_GHASHR[4];  /**< (AES Offset: 0x78) GCM Intermediate Hash Word Register 0 */
  __I  uint32_t AES_TAGR[4];    /**< (AES Offset: 0x88) GCM Authentication Tag Word Register 0 */
  __I  uint32_t AES_CTRR;       /**< (AES Offset: 0x98) GCM Encryption Counter Value Register */
  __IO uint32_t AES_GCMHR[4];   /**< (AES Offset: 0x9C) GCM H Word Register 0 */
} Aes;

#elif COMPONENT_TYPEDEF_STYLE == 'N'
/** \brief AES hardware registers */
typedef struct {  
  __O  AES_CR_Type                    AES_CR;         /**< Offset: 0x00 ( /W  32) Control Register */
  __IO AES_MR_Type                    AES_MR;         /**< Offset: 0x04 (R/W  32) Mode Register */
       RoReg8                         Reserved1[0x8];
  __O  AES_IER_Type                   AES_IER;        /**< Offset: 0x10 ( /W  32) Interrupt Enable Register */
  __O  AES_IDR_Type                   AES_IDR;        /**< Offset: 0x14 ( /W  32) Interrupt Disable Register */
  __I  AES_IMR_Type                   AES_IMR;        /**< Offset: 0x18 (R/   32) Interrupt Mask Register */
  __I  AES_ISR_Type                   AES_ISR;        /**< Offset: 0x1C (R/   32) Interrupt Status Register */
  __O  AES_KEYWR_Type                 AES_KEYWR[8];   /**< Offset: 0x20 ( /W  32) Key Word Register 0 */
  __O  AES_IDATAR_Type                AES_IDATAR[4];  /**< Offset: 0x40 ( /W  32) Input Data Register 0 */
  __I  AES_ODATAR_Type                AES_ODATAR[4];  /**< Offset: 0x50 (R/   32) Output Data Register 0 */
  __O  AES_IVR_Type                   AES_IVR[4];     /**< Offset: 0x60 ( /W  32) Initialization Vector Register 0 */
  __IO AES_AADLENR_Type               AES_AADLENR;    /**< Offset: 0x70 (R/W  32) Additional Authenticated Data Length Register */
  __IO AES_CLENR_Type                 AES_CLENR;      /**< Offset: 0x74 (R/W  32) Plaintext/Ciphertext Length Register */
  __IO AES_GHASHR_Type                AES_GHASHR[4];  /**< Offset: 0x78 (R/W  32) GCM Intermediate Hash Word Register 0 */
  __I  AES_TAGR_Type                  AES_TAGR[4];    /**< Offset: 0x88 (R/   32) GCM Authentication Tag Word Register 0 */
  __I  AES_CTRR_Type                  AES_CTRR;       /**< Offset: 0x98 (R/   32) GCM Encryption Counter Value Register */
  __IO AES_GCMHR_Type                 AES_GCMHR[4];   /**< Offset: 0x9C (R/W  32) GCM H Word Register 0 */
} Aes;

#else /* COMPONENT_TYPEDEF_STYLE */
#error Unknown component typedef style
#endif /* COMPONENT_TYPEDEF_STYLE */

#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */


/** @}  end of Advanced Encryption Standard */

#endif /* _SAME70_AES_COMPONENT_H_ */
