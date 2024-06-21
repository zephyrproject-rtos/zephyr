/*
 * Copyright (c) 2024 Microchip
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MICROCHIP_PIC32CXSG_AES_COMPONENT_FIXUP_H_
#define _MICROCHIP_PIC32CXSG_AES_COMPONENT_FIXUP_H_

/* -------- AES_CTRLA : (AES Offset: 0x00) (R/W 32) Control A -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t SWRST:1;          /*!< bit:      0  Software Reset                     */
    uint32_t ENABLE:1;         /*!< bit:      1  Enable                             */
    uint32_t AESMODE:3;        /*!< bit:  2.. 4  AES Modes of operation             */
    uint32_t CFBS:3;           /*!< bit:  5.. 7  Cipher Feedback Block Size         */
    uint32_t KEYSIZE:2;        /*!< bit:  8.. 9  Encryption Key Size                */
    uint32_t CIPHER:1;         /*!< bit:     10  Cipher Mode                        */
    uint32_t STARTMODE:1;      /*!< bit:     11  Start Mode Select                  */
    uint32_t LOD:1;            /*!< bit:     12  Last Output Data Mode              */
    uint32_t KEYGEN:1;         /*!< bit:     13  Last Key Generation                */
    uint32_t XORKEY:1;         /*!< bit:     14  XOR Key Operation                  */
    uint32_t :1;               /*!< bit:     15  Reserved                           */
    uint32_t CTYPE:4;          /*!< bit: 16..19  Counter Measure Type               */
    uint32_t :12;              /*!< bit: 20..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} AES_CTRLA_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- AES_CTRLB : (AES Offset: 0x04) (R/W 8) Control B -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  START:1;          /*!< bit:      0  Start Encryption/Decryption        */
    uint8_t  NEWMSG:1;         /*!< bit:      1  New message                        */
    uint8_t  EOM:1;            /*!< bit:      2  End of message                     */
    uint8_t  GFMUL:1;          /*!< bit:      3  GF Multiplication                  */
    uint8_t  :4;               /*!< bit:  4.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} AES_CTRLB_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- AES_INTENCLR : (AES Offset: 0x05) (R/W 8) Interrupt Enable Clear -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  ENCCMP:1;         /*!< bit:      0  Encryption Complete Interrupt Enable */
    uint8_t  GFMCMP:1;         /*!< bit:      1  GF Multiplication Complete Interrupt Enable */
    uint8_t  :6;               /*!< bit:  2.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} AES_INTENCLR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- AES_INTENSET : (AES Offset: 0x06) (R/W 8) Interrupt Enable Set -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  ENCCMP:1;         /*!< bit:      0  Encryption Complete Interrupt Enable */
    uint8_t  GFMCMP:1;         /*!< bit:      1  GF Multiplication Complete Interrupt Enable */
    uint8_t  :6;               /*!< bit:  2.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} AES_INTENSET_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- AES_INTFLAG : (AES Offset: 0x07) (R/W 8) Interrupt Flag Status -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { // __I to avoid read-modify-write on write-to-clear register
  struct {
    __I uint8_t  ENCCMP:1;         /*!< bit:      0  Encryption Complete                */
    __I uint8_t  GFMCMP:1;         /*!< bit:      1  GF Multiplication Complete         */
    __I uint8_t  :6;               /*!< bit:  2.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} AES_INTFLAG_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- AES_DATABUFPTR : (AES Offset: 0x08) (R/W 8) Data buffer pointer -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  INDATAPTR:2;      /*!< bit:  0.. 1  Input Data Pointer                 */
    uint8_t  :6;               /*!< bit:  2.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} AES_DATABUFPTR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- AES_DBGCTRL : (AES Offset: 0x09) (R/W 8) Debug control -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  DBGRUN:1;         /*!< bit:      0  Debug Run                          */
    uint8_t  :7;               /*!< bit:  1.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} AES_DBGCTRL_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- AES_KEYWORD : (AES Offset: 0x0C) ( /W 32) Keyword n -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  uint32_t reg;                /*!< Type      used for register access              */
} AES_KEYWORD_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#endif /* _MICROCHIP_PIC32CXSG_AES_COMPONENT_FIXUP_H_ */

/* -------- AES_INDATA : (AES Offset: 0x38) (R/W 32) Indata -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  uint32_t reg;                /*!< Type      used for register access              */
} AES_INDATA_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- AES_INTVECTV : (AES Offset: 0x3C) ( /W 32) Initialisation Vector n -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  uint32_t reg;                /*!< Type      used for register access              */
} AES_INTVECTV_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- AES_HASHKEY : (AES Offset: 0x5C) (R/W 32) Hash key n -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  uint32_t reg;                /*!< Type      used for register access              */
} AES_HASHKEY_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- AES_GHASH : (AES Offset: 0x6C) (R/W 32) Galois Hash n -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  uint32_t reg;                /*!< Type      used for register access              */
} AES_GHASH_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- AES_CIPLEN : (AES Offset: 0x80) (R/W 32) Cipher Length -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  uint32_t reg;                /*!< Type      used for register access              */
} AES_CIPLEN_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- AES_RANDSEED : (AES Offset: 0x84) (R/W 32) Random Seed -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  uint32_t reg;                /*!< Type      used for register access              */
} AES_RANDSEED_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/** \brief AES hardware registers */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef struct {
  __IO AES_CTRLA_Type            CTRLA;       /**< \brief Offset: 0x00 (R/W 32) Control A */
  __IO AES_CTRLB_Type            CTRLB;       /**< \brief Offset: 0x04 (R/W  8) Control B */
  __IO AES_INTENCLR_Type         INTENCLR;    /**< \brief Offset: 0x05 (R/W  8) Interrupt Enable Clear */
  __IO AES_INTENSET_Type         INTENSET;    /**< \brief Offset: 0x06 (R/W  8) Interrupt Enable Set */
  __IO AES_INTFLAG_Type          INTFLAG;     /**< \brief Offset: 0x07 (R/W  8) Interrupt Flag Status */
  __IO AES_DATABUFPTR_Type       DATABUFPTR;  /**< \brief Offset: 0x08 (R/W  8) Data buffer pointer */
  __IO AES_DBGCTRL_Type          DBGCTRL;     /**< \brief Offset: 0x09 (R/W  8) Debug control */
       RoReg8                    Reserved1[0x2];
  __O  AES_KEYWORD_Type          KEYWORD[8];  /**< \brief Offset: 0x0C ( /W 32) Keyword n */
       RoReg8                    Reserved2[0xC];
  __IO AES_INDATA_Type           INDATA;      /**< \brief Offset: 0x38 (R/W 32) Indata */
  __O  AES_INTVECTV_Type         INTVECTV[4]; /**< \brief Offset: 0x3C ( /W 32) Initialisation Vector n */
       RoReg8                    Reserved3[0x10];
  __IO AES_HASHKEY_Type          HASHKEY[4];  /**< \brief Offset: 0x5C (R/W 32) Hash key n */
  __IO AES_GHASH_Type            GHASH[4];    /**< \brief Offset: 0x6C (R/W 32) Galois Hash n */
       RoReg8                    Reserved4[0x4];
  __IO AES_CIPLEN_Type           CIPLEN;      /**< \brief Offset: 0x80 (R/W 32) Cipher Length */
  __IO AES_RANDSEED_Type         RANDSEED;    /**< \brief Offset: 0x84 (R/W 32) Random Seed */
} Aes;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */
