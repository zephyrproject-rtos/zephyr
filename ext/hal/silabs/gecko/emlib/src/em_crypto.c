/***************************************************************************//**
 * @file em_crypto.c
 * @brief Cryptography accelerator peripheral API
 * @version 5.6.0
 *******************************************************************************
 * # License
 * <b>Copyright 2016 Silicon Laboratories, Inc. www.silabs.com</b>
 *******************************************************************************
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
 * DISCLAIMER OF WARRANTY/LIMITATION OF REMEDIES: Silicon Labs has no
 * obligation to support this Software. Silicon Labs is providing the
 * Software "AS IS", with no express or implied warranties of any kind,
 * including, but not limited to, any implied warranties of merchantability
 * or fitness for any particular purpose or warranties against infringement
 * of any proprietary rights of a third party.
 *
 * Silicon Labs will not be liable for any consequential, incidental, or
 * special damages, or any other relief, or for any claim by any third party,
 * arising from your use of this Software.
 *
 ******************************************************************************/
#include "em_device.h"

#if defined(CRYPTO_COUNT) && (CRYPTO_COUNT > 0)

#include "em_crypto.h"
#include "em_assert.h"
#include <stddef.h>
#include <string.h>

/***************************************************************************//**
 * @addtogroup emlib
 * @{
 ******************************************************************************/

/***************************************************************************//**
 * @addtogroup CRYPTO
 * @{
 ******************************************************************************/

/*******************************************************************************
 *******************************   DEFINES   ***********************************
 ******************************************************************************/

/** @cond DO_NOT_INCLUDE_WITH_DOXYGEN */

#define CRYPTO_INSTRUCTIONS_PER_REG              (4UL)
#define CRYPTO_INSTRUCTIONS_MAX                  (12UL)
#define CRYPTO_INSTRUCTION_REGS                  (CRYPTO_INSTRUCTIONS_MAX / CRYPTO_INSTRUCTIONS_PER_REG)

#define CRYPTO_SHA1_BLOCK_SIZE_IN_BITS           (512UL)
#define CRYPTO_SHA1_BLOCK_SIZE_IN_BYTES          (CRYPTO_SHA1_BLOCK_SIZE_IN_BITS / 8UL)
#define CRYPTO_SHA1_BLOCK_SIZE_IN_32BIT_WORDS    (CRYPTO_SHA1_BLOCK_SIZE_IN_BYTES / sizeof(uint32_t))
#define CRYPTO_SHA1_DIGEST_SIZE_IN_32BIT_WORDS   (CRYPTO_SHA1_DIGEST_SIZE_IN_BYTES / sizeof(uint32_t))

#define CRYPTO_SHA256_BLOCK_SIZE_IN_BITS         (512UL)
#define CRYPTO_SHA256_BLOCK_SIZE_IN_BYTES        (CRYPTO_SHA256_BLOCK_SIZE_IN_BITS / 8UL)
#define CRYPTO_SHA256_BLOCK_SIZE_IN_32BIT_WORDS  (CRYPTO_SHA256_BLOCK_SIZE_IN_BYTES / sizeof(uint32_t))

#define CRYPTO_SHA256_DIGEST_SIZE_IN_32BIT_WORDS (CRYPTO_SHA256_DIGEST_SIZE_IN_BYTES / sizeof(uint32_t))

#define PARTIAL_OPERAND_WIDTH_LOG2               (7UL)  /* 2^7 = 128 */
#define PARTIAL_OPERAND_WIDTH                    (1UL << PARTIAL_OPERAND_WIDTH_LOG2)
#define PARTIAL_OPERAND_WIDTH_MASK               (PARTIAL_OPERAND_WIDTH - 1UL)
#define PARTIAL_OPERAND_WIDTH_IN_BYTES           (PARTIAL_OPERAND_WIDTH / 8UL)
#define PARTIAL_OPERAND_WIDTH_IN_32BIT_WORDS     (PARTIAL_OPERAND_WIDTH_IN_BYTES / sizeof(uint32_t))

#define SWAP32(x)                                (__REV(x))

#define CRYPTO_AES_BLOCKSIZE                     (16UL)

/*******************************************************************************
 ***********************   STATIC FUNCTIONS   **********************************
 ******************************************************************************/

__STATIC_INLINE void CRYPTO_AES_ProcessLoop(CRYPTO_TypeDef *crypto,
                                            uint32_t len,
                                            CRYPTO_DataReg_TypeDef inReg,
                                            const uint8_t * in,
                                            CRYPTO_DataReg_TypeDef outReg,
                                            uint8_t * out);

static void CRYPTO_AES_CBCx(CRYPTO_TypeDef *crypto,
                            uint8_t * out,
                            const uint8_t * in,
                            unsigned int len,
                            const uint8_t * key,
                            const uint8_t * iv,
                            bool encrypt,
                            CRYPTO_KeyWidth_TypeDef keyWidth);

static void CRYPTO_AES_CFBx(CRYPTO_TypeDef *crypto,
                            uint8_t * out,
                            const uint8_t * in,
                            unsigned int len,
                            const uint8_t * key,
                            const uint8_t * iv,
                            bool encrypt,
                            CRYPTO_KeyWidth_TypeDef keyWidth);

static void CRYPTO_AES_CTRx(CRYPTO_TypeDef *crypto,
                            uint8_t * out,
                            const uint8_t * in,
                            unsigned int len,
                            const uint8_t * key,
                            uint8_t * ctr,
                            CRYPTO_AES_CtrFuncPtr_TypeDef ctrFunc,
                            CRYPTO_KeyWidth_TypeDef keyWidth);

static void CRYPTO_AES_ECBx(CRYPTO_TypeDef *crypto,
                            uint8_t * out,
                            const uint8_t * in,
                            unsigned int len,
                            const uint8_t * key,
                            bool encrypt,
                            CRYPTO_KeyWidth_TypeDef keyWidth);

static void CRYPTO_AES_OFBx(CRYPTO_TypeDef *crypto,
                            uint8_t * out,
                            const uint8_t * in,
                            unsigned int len,
                            const uint8_t * key,
                            const uint8_t * iv,
                            CRYPTO_KeyWidth_TypeDef keyWidth);

__STATIC_INLINE void CRYPTO_DataReadUnaligned(volatile uint32_t * reg,
                                              uint8_t * val);
__STATIC_INLINE void CRYPTO_DataWriteUnaligned(volatile uint32_t * reg,
                                               const uint8_t * val);
__STATIC_INLINE
void CRYPTO_KeyBufWriteUnaligned(CRYPTO_TypeDef          *crypto,
                                 const uint8_t *          val,
                                 CRYPTO_KeyWidth_TypeDef  keyWidth);

#ifdef USE_VARIABLE_SIZED_DATA_LOADS
/***************************************************************************//**
 * @brief
 *   Write variable sized 32 bit data array (max 128 bits) to a DATAX register.
 *
 * @details
 *   Write variable sized 32 bit array (max 128 bits / 4 words) to a DATAX
 *   register in the CRYPTO module.
 *
 * @param[in]  dataReg    The 128 bits DATA register.
 * @param[in]  val        Value of the data to write to the DATA register.
 * @param[in]  valSize    Size of @ref val in number of 32 bit words.
 ******************************************************************************/
__STATIC_INLINE
void CRYPTO_DataWriteVariableSize(CRYPTO_DataReg_TypeDef    dataReg,
                                  const CRYPTO_Data_TypeDef val,
                                  int                       valSize)
{
  int i;
  volatile uint32_t * reg = (volatile uint32_t *) dataReg;

  if (valSize < 4) {
    /* Non optimal write of data. */
    for (i = 0; i < valSize; i++) {
      *reg = *val++;
    }
    for (; i < 4; i++) {
      *reg = 0;
    }
  } else {
    CRYPTO_BurstToCrypto(reg, &val[0]);
  }
}
#endif

/** @endcond */

/*******************************************************************************
 **************************   GLOBAL FUNCTIONS   *******************************
 ******************************************************************************/

/***************************************************************************//**
 * @brief
 *   Set the modulus used for wide modular operations.
 *
 * @details
 *   This function sets the modulus to be used by the modular instructions
 *   of the CRYPTO module.
 *
 * @param[in]  crypto
 *   A pointer to the CRYPTO peripheral register block.
 *
 * @param[in]  modulusId
 *   A modulus identifier.
 ******************************************************************************/
void CRYPTO_ModulusSet(CRYPTO_TypeDef *          crypto,
                       CRYPTO_ModulusId_TypeDef  modulusId)
{
  uint32_t temp = crypto->WAC & (~(_CRYPTO_WAC_MODULUS_MASK | _CRYPTO_WAC_MODOP_MASK));

  switch (modulusId) {
    case cryptoModulusBin256:
    case cryptoModulusBin128:
    case cryptoModulusGcmBin128:
    case cryptoModulusEccB233:
    case cryptoModulusEccB163:
#ifdef _CRYPTO_WAC_MODULUS_ECCBIN233N
    case cryptoModulusEccB233Order:
    case cryptoModulusEccB233KOrder:
    case cryptoModulusEccB163Order:
    case cryptoModulusEccB163KOrder:
#endif
      crypto->WAC = temp | (uint32_t)modulusId | CRYPTO_WAC_MODOP_BINARY;
      break;

    case cryptoModulusEccP256:
    case cryptoModulusEccP224:
    case cryptoModulusEccP192:
#ifdef _CRYPTO_WAC_MODULUS_ECCPRIME256P
    case cryptoModulusEccP256Order:
    case cryptoModulusEccP224Order:
    case cryptoModulusEccP192Order:
#endif
      crypto->WAC = temp | (uint32_t)modulusId | CRYPTO_WAC_MODOP_REGULAR;
      break;

    default:
      /* Unknown modulus identifier. */
      EFM_ASSERT(false);
      break;
  }
}

/***************************************************************************//**
 * @brief
 *   Read the key value currently used by the CRYPTO module.
 *
 * @details
 *   Read 128 bits or 256 bits from the KEY register in the CRYPTO module.
 *
 * @param[in]  crypto
 *   A pointer to the CRYPTO peripheral register block.
 *
 * @param[in]  val
 *   A value of the data to write to the KEYBUF register.
 *
 * @param[in]  keyWidth
 *   Key width - 128 or 256 bits
 ******************************************************************************/
void CRYPTO_KeyRead(CRYPTO_TypeDef *         crypto,
                    CRYPTO_KeyBuf_TypeDef    val,
                    CRYPTO_KeyWidth_TypeDef  keyWidth)
{
  EFM_ASSERT(&val[0] != NULL);

  CRYPTO_BurstFromCrypto(&crypto->KEY, &val[0]);
  if (keyWidth == cryptoKey256Bits) {
    CRYPTO_BurstFromCrypto(&crypto->KEY, &val[4]);
  }
}

/***************************************************************************//**
 * @brief
 *   Perform a SHA-1 hash operation on a message.
 *
 * @details
 *   This function performs a SHA-1 hash operation on the message specified by
 *   msg with length msgLen and returns the message digest in msgDigest.
 *
 * @param[in]  crypto
 *   A pointer to the CRYPTO peripheral register block.
 *
 * @param[in]  msg
 *   Message to hash.
 *
 * @param[in]  msgLen
 *   Length of message in bytes.
 *
 * @param[out] msgDigest
 *   A message digest.
 ******************************************************************************/
void CRYPTO_SHA_1(CRYPTO_TypeDef *             crypto,
                  const uint8_t *              msg,
                  uint64_t                     msgLen,
                  CRYPTO_SHA1_Digest_TypeDef   msgDigest)
{
  uint32_t  temp;
  uint32_t  len;
  int       blockLen;
  uint32_t  shaBlock[CRYPTO_SHA1_BLOCK_SIZE_IN_32BIT_WORDS];
  uint8_t * p8ShaBlock = (uint8_t *) shaBlock;

  /* Initialize the CRYPTO module to do SHA-1. */
  crypto->CTRL     = CRYPTO_CTRL_SHA_SHA1;
  crypto->SEQCTRL  = 0;
  crypto->SEQCTRLB = 0;

  /* Set the result width of the MADD32 operation. */
  CRYPTO_ResultWidthSet(crypto, cryptoResult256Bits);

  /* Write the initialization value to DDATA1.  */
  crypto->DDATA1 = 0x67452301UL;
  crypto->DDATA1 = 0xefcdab89UL;
  crypto->DDATA1 = 0x98badcfeUL;
  crypto->DDATA1 = 0x10325476UL;
  crypto->DDATA1 = 0xc3d2e1f0UL;
  crypto->DDATA1 = 0x00000000UL;
  crypto->DDATA1 = 0x00000000UL;
  crypto->DDATA1 = 0x00000000UL;

  /* Copy data to DDATA0 and select DDATA0 and DDATA1 for SHA operation. */
  CRYPTO_EXECUTE_2(crypto,
                   CRYPTO_CMD_INSTR_DDATA1TODDATA0,
                   CRYPTO_CMD_INSTR_SELDDATA0DDATA1);

  len = (uint32_t)msgLen;

  while (len >= CRYPTO_SHA1_BLOCK_SIZE_IN_BYTES) {
    /* Write block to QDATA1.  */
    CRYPTO_QDataWrite(&crypto->QDATA1BIG, (uint32_t *) msg);

    /* Execute SHA. */
    CRYPTO_EXECUTE_3(crypto,
                     CRYPTO_CMD_INSTR_SHA,
                     CRYPTO_CMD_INSTR_MADD32,
                     CRYPTO_CMD_INSTR_DDATA0TODDATA1);

    len -= CRYPTO_SHA1_BLOCK_SIZE_IN_BYTES;
    msg += CRYPTO_SHA1_BLOCK_SIZE_IN_BYTES;
  }

  blockLen = 0;

  /* Build the last (or second to last) block. */
  for (; len > 0U; len--) {
    p8ShaBlock[blockLen++] = *msg++;
  }

  /* Append the '1' bit. */
  p8ShaBlock[blockLen++] = 0x80;

  /* If the length is currently above 56 bytes, zeros are appended
   * then compressed.  Then, zeros are padded and length
   * encoded like normal.
   */
  if (blockLen > 56) {
    while (blockLen < 64) {
      p8ShaBlock[blockLen++] = 0;
    }

    /* Write block to QDATA1BIG. */
    CRYPTO_QDataWrite(&crypto->QDATA1BIG, shaBlock);

    /* Execute SH. */
    CRYPTO_EXECUTE_3(crypto,
                     CRYPTO_CMD_INSTR_SHA,
                     CRYPTO_CMD_INSTR_MADD32,
                     CRYPTO_CMD_INSTR_DDATA0TODDATA1);
    blockLen = 0;
  }

  /* Pad up to 56 bytes of zeros. */
  while (blockLen < 56) {
    p8ShaBlock[blockLen++] = 0;
  }

  /* Finally, encode the message length. */
  {
    uint64_t msgLenInBits = msgLen << 3U;
    temp = (uint32_t)(msgLenInBits >> 32U);
    *(uint32_t*)&p8ShaBlock[56] = SWAP32(temp);
    temp = (uint32_t)msgLenInBits & 0xFFFFFFFFUL;
    *(uint32_t*)&p8ShaBlock[60] = SWAP32(temp);
  }

  /* Write block to QDATA1BIG. */
  CRYPTO_QDataWrite(&crypto->QDATA1BIG, shaBlock);

  /* Execute SHA. */
  CRYPTO_EXECUTE_3(crypto,
                   CRYPTO_CMD_INSTR_SHA,
                   CRYPTO_CMD_INSTR_MADD32,
                   CRYPTO_CMD_INSTR_DDATA0TODDATA1);

  /* Read the resulting message digest from DDATA0BIG.  */
  ((uint32_t*)msgDigest)[0] = crypto->DDATA0BIG;
  ((uint32_t*)msgDigest)[1] = crypto->DDATA0BIG;
  ((uint32_t*)msgDigest)[2] = crypto->DDATA0BIG;
  ((uint32_t*)msgDigest)[3] = crypto->DDATA0BIG;
  ((uint32_t*)msgDigest)[4] = crypto->DDATA0BIG;
  crypto->DDATA0BIG;
  crypto->DDATA0BIG;
  crypto->DDATA0BIG;
}

/***************************************************************************//**
 * @brief
 *   Perform a SHA-256 hash operation on a message.
 *
 * @details
 *   This function performs a SHA-256 hash operation on the message specified
 *   by msg with length msgLen and returns the message digest in msgDigest.
 *
 * @param[in]  crypto
 *   A pointer to the CRYPTO peripheral register block.
 *
 * @param[in]  msg
 *   A message to hash.
 *
 * @param[in]  msgLen
 *   The length of message in bytes.
 *
 * @param[out] msgDigest
 *   A message digest.
 ******************************************************************************/
void CRYPTO_SHA_256(CRYPTO_TypeDef *             crypto,
                    const uint8_t *              msg,
                    uint64_t                     msgLen,
                    CRYPTO_SHA256_Digest_TypeDef msgDigest)
{
  uint32_t  temp;
  uint32_t  len;
  int       blockLen;
  uint32_t  shaBlock[CRYPTO_SHA256_BLOCK_SIZE_IN_32BIT_WORDS];
  uint8_t * p8ShaBlock = (uint8_t *) shaBlock;

  /* Initial values */
  shaBlock[0] = 0x6a09e667UL;
  shaBlock[1] = 0xbb67ae85UL;
  shaBlock[2] = 0x3c6ef372UL;
  shaBlock[3] = 0xa54ff53aUL;
  shaBlock[4] = 0x510e527fUL;
  shaBlock[5] = 0x9b05688cUL;
  shaBlock[6] = 0x1f83d9abUL;
  shaBlock[7] = 0x5be0cd19UL;

  /* Initialize the CRYPTO module to do SHA-256 (SHA-2). */
  crypto->CTRL     = CRYPTO_CTRL_SHA_SHA2;
  crypto->SEQCTRL  = 0;
  crypto->SEQCTRLB = 0;

  /* Set the result width of the MADD32 operation. */
  CRYPTO_ResultWidthSet(crypto, cryptoResult256Bits);

  /* Write the initialization value to DDATA1.  */
  CRYPTO_DDataWrite(&crypto->DDATA1, shaBlock);

  /* Copy data ot DDATA0 and select DDATA0 and DDATA1 for SHA operation. */
  CRYPTO_EXECUTE_2(crypto,
                   CRYPTO_CMD_INSTR_DDATA1TODDATA0,
                   CRYPTO_CMD_INSTR_SELDDATA0DDATA1);
  len = (uint32_t)msgLen;

  while (len >= CRYPTO_SHA256_BLOCK_SIZE_IN_BYTES) {
    /* Write block to QDATA1BIG.  */
    CRYPTO_QDataWrite(&crypto->QDATA1BIG, (uint32_t *) msg);

    /* Execute SHA. */
    CRYPTO_EXECUTE_3(crypto,
                     CRYPTO_CMD_INSTR_SHA,
                     CRYPTO_CMD_INSTR_MADD32,
                     CRYPTO_CMD_INSTR_DDATA0TODDATA1);

    len -= CRYPTO_SHA256_BLOCK_SIZE_IN_BYTES;
    msg += CRYPTO_SHA256_BLOCK_SIZE_IN_BYTES;
  }

  blockLen = 0;

  /* Build the last (or second to last) block. */
  for (; len > 0U; len--) {
    p8ShaBlock[blockLen++] = *msg++;
  }

  /* Append the '1' bit. */
  p8ShaBlock[blockLen++] = 0x80;

  /* If the length is currently above 56 bytes, zeros are appended
   * then compressed.  Then, zeros are padded and length
   * encoded like normal.
   */
  if (blockLen > 56) {
    while (blockLen < 64) {
      p8ShaBlock[blockLen++] = 0;
    }

    /* Write block to QDATA1BIG. */
    CRYPTO_QDataWrite(&crypto->QDATA1BIG, shaBlock);

    /* Execute SHA. */
    CRYPTO_EXECUTE_3(crypto,
                     CRYPTO_CMD_INSTR_SHA,
                     CRYPTO_CMD_INSTR_MADD32,
                     CRYPTO_CMD_INSTR_DDATA0TODDATA1);
    blockLen = 0;
  }

  /* Pad up to 56 bytes of zeros. */
  while (blockLen < 56) {
    p8ShaBlock[blockLen++] = 0;
  }

  /* Finally, encode the message length. */
  {
    uint64_t msgLenInBits = msgLen << 3;
    temp = (uint32_t)(msgLenInBits >> 32);
    *(uint32_t *)&p8ShaBlock[56] = SWAP32(temp);
    temp = (uint32_t)msgLenInBits & 0xFFFFFFFFUL;
    *(uint32_t *)&p8ShaBlock[60] = SWAP32(temp);
  }

  /* Write the final block to QDATA1BIG. */
  CRYPTO_QDataWrite(&crypto->QDATA1BIG, shaBlock);

  /* Execute SHA. */
  CRYPTO_EXECUTE_3(crypto,
                   CRYPTO_CMD_INSTR_SHA,
                   CRYPTO_CMD_INSTR_MADD32,
                   CRYPTO_CMD_INSTR_DDATA0TODDATA1);

  /* Read the resulting message digest from DDATA0BIG.  */
  CRYPTO_DDataRead(&crypto->DDATA0BIG, (uint32_t *)msgDigest);
}

/***************************************************************************//**
 * @brief
 *   Set the 32 bit word array to zero.
 *
 * @param[in]  words32bits    A pointer to the 32 bit word array.
 * @param[in]  num32bitWords  A number of 32 bit words in array.
 ******************************************************************************/
__STATIC_INLINE void cryptoBigintZeroize(uint32_t * words32bits,
                                         unsigned   num32bitWords)
{
  while (num32bitWords > 0UL) {
    num32bitWords--;
    *words32bits++ = 0;
  }
}

/***************************************************************************//**
 * @brief
 *   Increment value of 32bit word array by one.
 *
 * @param[in] words32bits    Pointer to 32bit word array
 * @param[in] num32bitWords  Number of 32bit words in array
 ******************************************************************************/
__STATIC_INLINE void cryptoBigintIncrement(uint32_t * words32bits,
                                           unsigned   num32bitWords)
{
  unsigned i;
  for (i = 0; i < num32bitWords; i++) {
    if (++words32bits[i] != 0UL) {
      break;
    }
  }
  return;
}

/***************************************************************************//**
 * @brief
 *   Multiply two big integers.
 *
 * @details
 *   This function uses the CRYPTO unit to multiply two big integer operands.
 *   If USE_VARIABLE_SIZED_DATA_LOADS is defined, the sizes of the operands
 *   may be any multiple of 32 bits. If USE_VARIABLE_SIZED_DATA_LOADS is _not_
 *   defined, the sizes of the operands must be a multiple of 128 bits.
 *
 * @param[in]  A        An operand A
 * @param[in]  aSize    The size of the operand A in bits
 * @param[in]  B        An operand B
 * @param[in]  bSize    The size of the operand B in bits
 * @param[out] R        The result of multiplication
 * @param[in]  rSize    The size of the result buffer R in bits
 ******************************************************************************/
void CRYPTO_Mul(CRYPTO_TypeDef * crypto,
                uint32_t * A, int aSize,
                uint32_t * B, int bSize,
                uint32_t * R, int rSize)
{
  unsigned i, j;

  /****************   Initializations   ******************/

#ifdef USE_VARIABLE_SIZED_DATA_LOADS
  unsigned numWordsLastOperandA = ((unsigned)aSize & PARTIAL_OPERAND_WIDTH_MASK) >> 5;
  unsigned numPartialOperandsA = numWordsLastOperandA
                                 ? ((unsigned)aSize >> PARTIAL_OPERAND_WIDTH_LOG2) + 1
                                 : (unsigned)aSize >> PARTIAL_OPERAND_WIDTH_LOG2;
  unsigned numWordsLastOperandB = ((unsigned)bSize & PARTIAL_OPERAND_WIDTH_MASK) >> 5;
  unsigned numPartialOperandsB = numWordsLastOperandB
                                 ? ((unsigned)bSize >> PARTIAL_OPERAND_WIDTH_LOG2) + 1
                                 : (unsigned)bSize >> PARTIAL_OPERAND_WIDTH_LOG2;
  unsigned numWordsLastOperandR = ((unsigned)rSize & PARTIAL_OPERAND_WIDTH_MASK) >> 5;
  unsigned numPartialOperandsR = numWordsLastOperandR
                                 ? ((unsigned)rSize >> PARTIAL_OPERAND_WIDTH_LOG2) + 1
                                 : (unsigned)rSize >> PARTIAL_OPERAND_WIDTH_LOG2;
  EFM_ASSERT(numPartialOperandsA + numPartialOperandsB <= numPartialOperandsR);
#else
  unsigned numPartialOperandsA = (unsigned)aSize >> PARTIAL_OPERAND_WIDTH_LOG2;
  unsigned numPartialOperandsB = (unsigned)bSize >> PARTIAL_OPERAND_WIDTH_LOG2;
  EFM_ASSERT(((unsigned)aSize & PARTIAL_OPERAND_WIDTH_MASK) == 0UL);
  EFM_ASSERT(((unsigned)bSize & PARTIAL_OPERAND_WIDTH_MASK) == 0UL);
#endif
  EFM_ASSERT(aSize + bSize <= rSize);

  /* Set R to zero. */
  cryptoBigintZeroize(R, (unsigned)rSize >> 5);

  /* Set the multiplication width. */
  crypto->WAC = CRYPTO_WAC_MULWIDTH_MUL128 | CRYPTO_WAC_RESULTWIDTH_256BIT;

  /* Set up DMA request signaling for MCU to run in parallel with
     the CRYPTO instruction sequence execution and prepare data loading which
     can take place immediately when CRYPTO is ready inside the instruction
     sequence. */
  crypto->CTRL =
    CRYPTO_CTRL_DMA0RSEL_DATA0 | CRYPTO_CTRL_DMA0MODE_FULL
    | CRYPTO_CTRL_DMA1RSEL_DATA1 | CRYPTO_CTRL_DMA1MODE_FULL;

  CRYPTO_EXECUTE_4(crypto,
                   CRYPTO_CMD_INSTR_CCLR,    /* Carry = 0 */
                   CRYPTO_CMD_INSTR_CLR,     /* DDATA0 = 0 */
                   /* clear result accumulation register */
                   CRYPTO_CMD_INSTR_DDATA0TODDATA2,
                   CRYPTO_CMD_INSTR_SELDDATA1DDATA3);
  /*
     register map:
     DDATA0: working register
     DDATA1: B(j)
     DDATA2: R(i+j+1) and R(i+j), combined with DMA entry for B(j)
     DDATA3: A(i)
   */

  CRYPTO_SEQ_LOAD_10(crypto,
                     /* Temporarily load the partial operand B(j) to DATA0. */
                     /* R(i+j+1) is still in DATA1 */
                     CRYPTO_CMD_INSTR_DMA0TODATA,
                     /* Move B(j) to DDATA1. */
                     CRYPTO_CMD_INSTR_DDATA2TODDATA1,

                     /* Restore the previous partial result (now R(i+j)). */
                     CRYPTO_CMD_INSTR_DATA1TODATA0,

                     /* Load the next partial result R(i+j+1). */
                     CRYPTO_CMD_INSTR_DMA1TODATA,

                     /* Execute the partial multiplication A(i)inDDATA1 * B(j)inDDATA3.*/
                     CRYPTO_CMD_INSTR_MULO,

                     /* Add the result to the previous partial result. */
                     /* AND take the previous carry value into account */
                     /* at the right place (bit 128, ADDIC instruction. */
                     CRYPTO_CMD_INSTR_SELDDATA0DDATA2,
                     CRYPTO_CMD_INSTR_ADDIC,

                     /* Save the new partial result (lower half). */
                     CRYPTO_CMD_INSTR_DDATA0TODDATA2,
                     CRYPTO_CMD_INSTR_DATATODMA0,
                     /* Reset the operand selector for next.*/
                     CRYPTO_CMD_INSTR_SELDDATA2DDATA3
                     );

  /****************   End Initializations   ******************/

  for (i = 0; i < numPartialOperandsA; i++) {
    /* Load the partial operand #1 A>>(i*PARTIAL_OPERAND_WIDTH) to DDATA1. */
#ifdef USE_VARIABLE_SIZED_DATA_LOADS
    if ( (numWordsLastOperandA != 0) && (i == numPartialOperandsA - 1) ) {
      CRYPTO_DataWriteVariableSize(&crypto->DATA2,
                                   &A[i * PARTIAL_OPERAND_WIDTH_IN_32BIT_WORDS],
                                   numWordsLastOperandA);
    } else {
      CRYPTO_DataWrite(&crypto->DATA2, &A[i * PARTIAL_OPERAND_WIDTH_IN_32BIT_WORDS]);
    }
#else
    CRYPTO_DataWrite(&crypto->DATA2, &A[i * PARTIAL_OPERAND_WIDTH_IN_32BIT_WORDS]);
#endif

    /* Load the partial result in R>>(i*PARTIAL_OPERAND_WIDTH) to DATA1. */
#ifdef USE_VARIABLE_SIZED_DATA_LOADS
    if ( (numWordsLastOperandR != 0) && (i == numPartialOperandsR - 1) ) {
      CRYPTO_DataWriteVariableSize(&crypto->DATA1,
                                   &R[i * PARTIAL_OPERAND_WIDTH_IN_32BIT_WORDS],
                                   numWordsLastOperandR);
    } else {
      CRYPTO_DataWrite(&crypto->DATA1, &R[i * PARTIAL_OPERAND_WIDTH_IN_32BIT_WORDS]);
    }
#else
    CRYPTO_DataWrite(&crypto->DATA1, &R[i * PARTIAL_OPERAND_WIDTH_IN_32BIT_WORDS]);
#endif

    /* Clear carry. */
    crypto->CMD = CRYPTO_CMD_INSTR_CCLR;

    /* Set up the number of sequence iterations and block size. */
    crypto->SEQCTRL = CRYPTO_SEQCTRL_BLOCKSIZE_16BYTES
                      | (PARTIAL_OPERAND_WIDTH_IN_BYTES * numPartialOperandsB);

    /* Execute the MULtiply instruction sequence. */
    CRYPTO_InstructionSequenceExecute(crypto);

    for (j = 0; j < numPartialOperandsB; j++) {
      /* Load the partial operand 2 B>>(j*`PARTIAL_OPERAND_WIDTH) to DDATA2
         (via DATA0). */
#ifdef USE_VARIABLE_SIZED_DATA_LOADS
      if ( (numWordsLastOperandB != 0) && (j == numPartialOperandsB - 1) ) {
        CRYPTO_DataWriteVariableSize(&crypto->DATA0,
                                     &B[j * PARTIAL_OPERAND_WIDTH_IN_32BIT_WORDS],
                                     numWordsLastOperandB);
      } else {
        CRYPTO_DataWrite(&crypto->DATA0,
                         &B[j * PARTIAL_OPERAND_WIDTH_IN_32BIT_WORDS]);
      }
#else
      CRYPTO_DataWrite(&crypto->DATA0,
                       &B[j * PARTIAL_OPERAND_WIDTH_IN_32BIT_WORDS]);
#endif

      /* Load the most significant partial result
         R>>((i+j+1)*`PARTIAL_OPERAND_WIDTH) into DATA1. */
#ifdef USE_VARIABLE_SIZED_DATA_LOADS
      if ( (numWordsLastOperandR != 0) && ( (i + j + 1U) == numPartialOperandsR - 1U) ) {
        CRYPTO_DataWriteVariableSize(&crypto->DATA1,
                                     &R[(i + j + 1U) * PARTIAL_OPERAND_WIDTH_IN_32BIT_WORDS],
                                     numWordsLastOperandR);
      } else {
        CRYPTO_DataWrite(&crypto->DATA1,
                         &R[(i + j + 1U) * PARTIAL_OPERAND_WIDTH_IN_32BIT_WORDS]);
      }
#else
      CRYPTO_DataWrite(&crypto->DATA1,
                       &R[(i + j + 1U) * PARTIAL_OPERAND_WIDTH_IN_32BIT_WORDS]);
#endif
      /* Store the least significant partial result. */
      CRYPTO_DataRead(&crypto->DATA0,
                      &R[(i + j) * PARTIAL_OPERAND_WIDTH_IN_32BIT_WORDS]);
    } /* for (j=0; j<numPartialOperandsB; j++) */

    /* Handle carry at the end of the inner loop. */
    if (CRYPTO_CarryIsSet(crypto)) {
      cryptoBigintIncrement(&R[(i + numPartialOperandsB + 1U)
                               * PARTIAL_OPERAND_WIDTH_IN_32BIT_WORDS],
                            (numPartialOperandsA - i - 1U)
                            * PARTIAL_OPERAND_WIDTH_IN_32BIT_WORDS);
    }

    CRYPTO_DataRead(&crypto->DATA1,
                    &R[(i + numPartialOperandsB)
                       * PARTIAL_OPERAND_WIDTH_IN_32BIT_WORDS]);
  } /* for (i=0; i<numPartialOperandsA; i++) */
}

/***************************************************************************//**
 * @brief
 *   AES Cipher-block chaining (CBC) cipher mode encryption/decryption, 128 bit key.
 *
 * @details
 *   Encryption:
 * @verbatim
 *           Plaintext                  Plaintext
 *               |                          |
 *               V                          V
 * InitVector ->XOR        +-------------->XOR
 *               |         |                |
 *               V         |                V
 *       +--------------+  |        +--------------+
 * Key ->| Block cipher |  |  Key ->| Block cipher |
 *       |  encryption  |  |        |  encryption  |
 *       +--------------+  |        +--------------+
 *               |---------+                |
 *               V                          V
 *           Ciphertext                 Ciphertext
 * @endverbatim
 *   Decryption:
 * @verbatim
 *         Ciphertext                 Ciphertext
 *              |----------+                |
 *              V          |                V
 *       +--------------+  |        +--------------+
 * Key ->| Block cipher |  |  Key ->| Block cipher |
 *       |  decryption  |  |        |  decryption  |
 *       +--------------+  |        +--------------+
 *               |         |                |
 *               V         |                V
 * InitVector ->XOR        +-------------->XOR
 *               |                          |
 *               V                          V
 *           Plaintext                  Plaintext
 * @endverbatim
 *   See general comments on layout and byte ordering of parameters.
 *
 * @param[in]  crypto
 *   A pointer to the CRYPTO peripheral register block.
 *
 * @param[out] out
 *   A buffer to place encrypted/decrypted data. Must be at least @p len long. It
 *   may be set equal to @p in, in which case the input buffer is overwritten.
 *
 * @param[in] in
 *   A buffer holding data to encrypt/decrypt. Must be at least @p len long.
 *
 * @param[in] len
 *   A number of bytes to encrypt/decrypt. Must be a multiple of 16.
 *
 * @param[in] key
 *   When encrypting, this is the 128 bit encryption key. When
 *   decrypting, this is the 128 bit decryption key. The decryption key may
 *   be generated from the encryption key with CRYPTO_AES_DecryptKey128().
 *   If this argument is null, the key will not be loaded, as it is assumed
 *   the key has been loaded into KEYHA previously.
 *
 * @param[in] iv
 *   128 bit initialization vector to use.
 *
 * @param[in] encrypt
 *   Set to true to encrypt, false to decrypt.
 ******************************************************************************/
void CRYPTO_AES_CBC128(CRYPTO_TypeDef *  crypto,
                       uint8_t *         out,
                       const uint8_t *   in,
                       unsigned int      len,
                       const uint8_t *   key,
                       const uint8_t *   iv,
                       bool              encrypt)
{
  crypto->CTRL = CRYPTO_CTRL_AES_AES128;
  CRYPTO_AES_CBCx(crypto, out, in, len, key, iv, encrypt, cryptoKey128Bits);
}

/***************************************************************************//**
 * @brief
 *   AES Cipher-block chaining (CBC) cipher mode encryption/decryption, 256 bit
 *   key.
 *
 * @details
 *   See CRYPTO_AES_CBC128() for the CBC figure.
 *
 *   See general comments on layout and byte ordering of parameters.
 *
 * @param[in]  crypto
 *   A pointer to the CRYPTO peripheral register block.
 *
 * @param[out] out
 *   A buffer to place encrypted/decrypted data. Must be at least @p len long. It
 *   may be set equal to @p in, in which case the input buffer is overwritten.
 *
 * @param[in] in
 *   A buffer holding data to encrypt/decrypt. Must be at least @p len long.
 *
 * @param[in] len
 *   A number of bytes to encrypt/decrypt. Must be a multiple of 16.
 *
 * @param[in] key
 *   When encrypting, this is the 256 bit encryption key. When
 *   decrypting, this is the 256 bit decryption key. The decryption key may
 *   be generated from the encryption key with CRYPTO_AES_DecryptKey256().
 *
 * @param[in] iv
 *   128 bit initialization vector to use.
 *
 * @param[in] encrypt
 *   Set to true to encrypt, false to decrypt.
 ******************************************************************************/
void CRYPTO_AES_CBC256(CRYPTO_TypeDef *  crypto,
                       uint8_t *         out,
                       const uint8_t *   in,
                       unsigned int      len,
                       const uint8_t *   key,
                       const uint8_t *   iv,
                       bool              encrypt)
{
  crypto->CTRL = CRYPTO_CTRL_AES_AES256;
  CRYPTO_AES_CBCx(crypto, out, in, len, key, iv, encrypt, cryptoKey256Bits);
}

/***************************************************************************//**
 * @brief
 *   AES Cipher feedback (CFB) cipher mode encryption/decryption, 128 bit key.
 *
 * @details
 *   Encryption:
 * @verbatim
 *           InitVector    +----------------+
 *               |         |                |
 *               V         |                V
 *       +--------------+  |        +--------------+
 * Key ->| Block cipher |  |  Key ->| Block cipher |
 *       |  encryption  |  |        |  encryption  |
 *       +--------------+  |        +--------------+
 *               |         |                |
 *               V         |                V
 *  Plaintext ->XOR        |   Plaintext ->XOR
 *               |---------+                |
 *               V                          V
 *           Ciphertext                 Ciphertext
 * @endverbatim
 *   Decryption:
 * @verbatim
 *          InitVector     +----------------+
 *               |         |                |
 *               V         |                V
 *       +--------------+  |        +--------------+
 * Key ->| Block cipher |  |  Key ->| Block cipher |
 *       |  encryption  |  |        |  encryption  |
 *       +--------------+  |        +--------------+
 *               |         |                |
 *               V         |                V
 *              XOR<- Ciphertext           XOR<- Ciphertext
 *               |                          |
 *               V                          V
 *           Plaintext                  Plaintext
 * @endverbatim
 *   See general comments on layout and byte ordering of parameters.
 *
 * @param[in]  crypto
 *   A pointer to the CRYPTO peripheral register block.
 *
 * @param[out] out
 *   A buffer to place encrypted/decrypted data. Must be at least @p len long. It
 *   may be set equal to @p in, in which case the input buffer is overwritten.
 *
 * @param[in] in
 *   A buffer holding data to encrypt/decrypt. Must be at least @p len long.
 *
 * @param[in] len
 *   A number of bytes to encrypt/decrypt. Must be a multiple of 16.
 *
 * @param[in] key
 *   128 bit encryption key is used for both encryption and decryption modes.
 *
 * @param[in] iv
 *   128 bit initialization vector to use.
 *
 * @param[in] encrypt
 *   Set to true to encrypt, false to decrypt.
 ******************************************************************************/
void CRYPTO_AES_CFB128(CRYPTO_TypeDef *  crypto,
                       uint8_t *         out,
                       const uint8_t *   in,
                       unsigned int      len,
                       const uint8_t *   key,
                       const uint8_t *   iv,
                       bool              encrypt)
{
  crypto->CTRL = CRYPTO_CTRL_AES_AES128;
  CRYPTO_AES_CFBx(crypto, out, in, len, key, iv, encrypt, cryptoKey128Bits);
}

/***************************************************************************//**
 * @brief
 *   AES Cipher feedback (CFB) cipher mode encryption/decryption, 256 bit key.
 *
 * @details
 *   See CRYPTO_AES_CFB128() for the CFB figure.
 *
 *   See general comments on layout and byte ordering of parameters.
 *
 * @param[in]  crypto
 *   A pointer to the CRYPTO peripheral register block.
 *
 * @param[out] out
 *   A buffer to place encrypted/decrypted data. Must be at least @p len long. It
 *   may be set equal to @p in, in which case the input buffer is overwritten.
 *
 * @param[in] in
 *   A buffer holding data to encrypt/decrypt. Must be at least @p len long.
 *
 * @param[in] len
 *   A number of bytes to encrypt/decrypt. Must be a multiple of 16.
 *
 * @param[in] key
 *   256 bit encryption key is used for both encryption and decryption modes.
 *
 * @param[in] iv
 *   128 bit initialization vector to use.
 *
 * @param[in] encrypt
 *   Set to true to encrypt, false to decrypt.
 ******************************************************************************/
void CRYPTO_AES_CFB256(CRYPTO_TypeDef *  crypto,
                       uint8_t *         out,
                       const uint8_t *   in,
                       unsigned int      len,
                       const uint8_t *   key,
                       const uint8_t *   iv,
                       bool              encrypt)
{
  crypto->CTRL = CRYPTO_CTRL_AES_AES256;
  CRYPTO_AES_CFBx(crypto, out, in, len, key, iv, encrypt, cryptoKey256Bits);
}

/***************************************************************************//**
 * @brief
 *   AES Counter (CTR) cipher mode encryption/decryption, 128 bit key.
 *
 * @details
 *   Encryption:
 * @verbatim
 *           Counter                    Counter
 *              |                          |
 *              V                          V
 *       +--------------+           +--------------+
 * Key ->| Block cipher |     Key ->| Block cipher |
 *       |  encryption  |           |  encryption  |
 *       +--------------+           +--------------+
 *              |                          |
 * Plaintext ->XOR            Plaintext ->XOR
 *              |                          |
 *              V                          V
 *         Ciphertext                 Ciphertext
 * @endverbatim
 *   Decryption:
 * @verbatim
 *           Counter                    Counter
 *              |                          |
 *              V                          V
 *       +--------------+           +--------------+
 * Key ->| Block cipher |     Key ->| Block cipher |
 *       |  encryption  |           |  encryption  |
 *       +--------------+           +--------------+
 *               |                          |
 * Ciphertext ->XOR           Ciphertext ->XOR
 *               |                          |
 *               V                          V
 *           Plaintext                  Plaintext
 * @endverbatim
 *   See general comments on layout and byte ordering of parameters.
 *
 * @param[in]  crypto
 *   A pointer to CRYPTO peripheral register block.
 *
 * @param[out] out
 *   A buffer to place encrypted/decrypted data. Must be at least @p len long. It
 *   may be set equal to @p in, in which case the input buffer is overwritten.
 *
 * @param[in] in
 *   A buffer holding data to encrypt/decrypt. Must be at least @p len long.
 *
 * @param[in] len
 *   A number of bytes to encrypt/decrypt. Must be a multiple of 16.
 *
 * @param[in] key
 *   128 bit encryption key.
 *   If this argument is null, the key will not be loaded, as it is assumed
 *   the key has been loaded into KEYHA previously.
 *
 * @param[in,out] ctr
 *   128 bit initial counter value. The counter is updated after each AES
 *   block encoding through use of @p ctrFunc.
 *
 * @param[in] ctrFunc
 *   A function used to update the counter value. Not supported by CRYPTO.
 *   This parameter is included for backwards compatibility with
 *   the EFM32 em_aes.h API.
 ******************************************************************************/
void CRYPTO_AES_CTR128(CRYPTO_TypeDef *  crypto,
                       uint8_t *         out,
                       const uint8_t *   in,
                       unsigned int      len,
                       const uint8_t *   key,
                       uint8_t *         ctr,
                       CRYPTO_AES_CtrFuncPtr_TypeDef ctrFunc)
{
  crypto->CTRL = CRYPTO_CTRL_AES_AES128;
  CRYPTO_AES_CTRx(crypto, out, in, len, key, ctr, ctrFunc, cryptoKey128Bits);
}

/***************************************************************************//**
 * @brief
 *   AES Counter (CTR) cipher mode encryption/decryption, 256 bit key.
 *
 * @details
 *   See CRYPTO_AES_CTR128() for CTR figure.
 *
 *   See general comments on layout and byte ordering of parameters.
 *
 * @param[in]  crypto
 *   A pointer to the CRYPTO peripheral register block.
 *
 * @param[out] out
 *   A buffer to place encrypted/decrypted data. Must be at least @p len long. It
 *   may be set equal to @p in, in which case the input buffer is overwritten.
 *
 * @param[in] in
 *   A buffer holding data to encrypt/decrypt. Must be at least @p len long.
 *
 * @param[in] len
 *   A number of bytes to encrypt/decrypt. Must be a multiple of 16.
 *
 * @param[in] key
 *   256 bit encryption key.
 *
 * @param[in,out] ctr
 *   128 bit initial counter value. The counter is updated after each AES
 *   block encoding through use of @p ctrFunc.
 *
 * @param[in] ctrFunc
 *   A function used to update counter value. Not supported by CRYPTO.
 *   This parameter is included in order for backwards compatibility with
 *   the EFM32 em_aes.h API.
 ******************************************************************************/
void CRYPTO_AES_CTR256(CRYPTO_TypeDef *  crypto,
                       uint8_t *         out,
                       const uint8_t *   in,
                       unsigned int      len,
                       const uint8_t *   key,
                       uint8_t *         ctr,
                       CRYPTO_AES_CtrFuncPtr_TypeDef ctrFunc)
{
  crypto->CTRL = CRYPTO_CTRL_AES_AES256;
  CRYPTO_AES_CTRx(crypto, out, in, len, key, ctr, ctrFunc, cryptoKey256Bits);
}

/***************************************************************************//**
 * @brief
 *   Update the last 32 bits of 128 bit counter by incrementing with 1.
 *
 * @details
 *   Notice that no special consideration is given to the possible wrap around. If
 *   32 least significant bits are 0xFFFFFFFF, they will be updated to 0x00000000,
 *   ignoring overflow.
 *
 *   See general comments on layout and byte ordering of parameters.
 *
 * @param[in,out] ctr
 *   A buffer holding 128 bit counter to be updated.
 ******************************************************************************/
void CRYPTO_AES_CTRUpdate32Bit(uint8_t * ctr)
{
  uint32_t * _ctr = (uint32_t *) ctr;

  _ctr[3] = __REV(__REV(_ctr[3]) + 1U);
}

/***************************************************************************//**
 * @brief
 *   Generate 128 bit AES decryption key from 128 bit encryption key. The
 *   decryption key is used for some cipher modes when decrypting.
 *
 * @details
 *   See general comments on layout and byte ordering of parameters.
 *
 * @param[in]  crypto
 *   A pointer to the CRYPTO peripheral register block.
 *
 * @param[out] out
 *   A buffer to place 128 bit decryption key. Must be at least 16 bytes long. It
 *   may be set equal to @p in, in which case the input buffer is overwritten.
 *
 * @param[in] in
 *   A buffer holding 128 bit encryption key. Must be at least 16 bytes long.
 ******************************************************************************/
void CRYPTO_AES_DecryptKey128(CRYPTO_TypeDef *  crypto,
                              uint8_t *         out,
                              const uint8_t *   in)
{
  /* Setup CRYPTO in AES-128 mode. */
  crypto->CTRL = CRYPTO_CTRL_AES_AES128;

  /* Load key */
  CRYPTO_DataWriteUnaligned(&crypto->KEYBUF, in);

  /* Do dummy encryption to generate decrypt key */
  crypto->CMD  = CRYPTO_CMD_INSTR_AESENC;

  /* Save decryption key */
  CRYPTO_DataReadUnaligned(&crypto->KEY, out);
}

/***************************************************************************//**
 * @brief
 *   Generate 256 bit AES decryption key from 256 bit encryption key. The
 *   decryption key is used for some cipher modes when decrypting.
 *
 * @details
 *   See general comments on layout and byte ordering of parameters.
 *
 * @param[in]  crypto
 *   A pointer to the CRYPTO peripheral register block.
 *
 * @param[out] out
 *   A buffer to place 256 bit decryption key. Must be at least 32 bytes long. It
 *   may be set equal to @p in, in which case the input buffer is overwritten.
 *
 * @param[in] in
 *   A buffer holding 256 bit encryption key. Must be at least 32 bytes long.
 ******************************************************************************/
void CRYPTO_AES_DecryptKey256(CRYPTO_TypeDef *  crypto,
                              uint8_t *         out,
                              const uint8_t *   in)
{
  /* Set up CRYPTO in AES-256 mode. */
  crypto->CTRL = CRYPTO_CTRL_AES_AES256;

  /* Load the key. */
  CRYPTO_DataWriteUnaligned(&crypto->KEYBUF, in);
  CRYPTO_DataWriteUnaligned(&crypto->KEYBUF, &in[16]);

  /* Do dummy encryption to generate a decrypt key. */
  crypto->CMD  = CRYPTO_CMD_INSTR_AESENC;

  /* Save the decryption key. */
  CRYPTO_DataReadUnaligned(&crypto->KEY, out);
  CRYPTO_DataReadUnaligned(&crypto->KEY, &out[16]);
}

/***************************************************************************//**
 * @brief
 *   AES Electronic Codebook (ECB) cipher mode encryption/decryption,
 *   128 bit key.
 *
 * @details
 *   Encryption:
 * @verbatim
 *          Plaintext                  Plaintext
 *              |                          |
 *              V                          V
 *       +--------------+           +--------------+
 * Key ->| Block cipher |     Key ->| Block cipher |
 *       |  encryption  |           |  encryption  |
 *       +--------------+           +--------------+
 *              |                          |
 *              V                          V
 *         Ciphertext                 Ciphertext
 * @endverbatim
 *   Decryption:
 * @verbatim
 *         Ciphertext                 Ciphertext
 *              |                          |
 *              V                          V
 *       +--------------+           +--------------+
 * Key ->| Block cipher |     Key ->| Block cipher |
 *       |  decryption  |           |  decryption  |
 *       +--------------+           +--------------+
 *              |                          |
 *              V                          V
 *          Plaintext                  Plaintext
 * @endverbatim
 *   See general comments on layout and byte ordering of parameters.
 *
 * @param[in]  crypto
 *   A pointer to the CRYPTO peripheral register block.
 *
 * @param[out] out
 *   A buffer to place encrypted/decrypted data. Must be at least @p len long. It
 *   may be set equal to @p in, in which case the input buffer is overwritten.
 *
 * @param[in] in
 *   A buffer holding data to encrypt/decrypt. Must be at least @p len long.
 *
 * @param[in] len
 *   A number of bytes to encrypt/decrypt. Must be a multiple of 16.
 *
 * @param[in] key
 *   When encrypting, this is the 128 bit encryption key. When
 *   decrypting, this is the 128 bit decryption key. The decryption key may
 *   be generated from the encryption key with CRYPTO_AES_DecryptKey128().
 *
 * @param[in] encrypt
 *   Set to true to encrypt, false to decrypt.
 ******************************************************************************/
void CRYPTO_AES_ECB128(CRYPTO_TypeDef *  crypto,
                       uint8_t *         out,
                       const uint8_t *   in,
                       unsigned int      len,
                       const uint8_t *   key,
                       bool              encrypt)
{
  crypto->CTRL = CRYPTO_CTRL_AES_AES128;
  CRYPTO_AES_ECBx(crypto, out, in, len, key, encrypt, cryptoKey128Bits);
}

/***************************************************************************//**
 * @brief
 *   AES Electronic Codebook (ECB) cipher mode encryption/decryption,
 *   256 bit key.
 *
 * @details
 *   See CRYPTO_AES_ECB128() for ECB figure.
 *
 *   See general comments on layout and byte ordering of parameters.
 *
 * @param[in]  crypto
 *   A pointer to the CRYPTO peripheral register block.
 *
 * @param[out] out
 *   A buffer to place encrypted/decrypted data. Must be at least @p len long. It
 *   may be set equal to @p in, in which case the input buffer is overwritten.
 *
 * @param[in] in
 *   A buffer holding data to encrypt/decrypt. Must be at least @p len long.
 *
 * @param[in] len
 *   A number of bytes to encrypt/decrypt. Must be a multiple of 16.
 *
 * @param[in] key
 *   When encrypting, this is the 256 bit encryption key. When
 *   decrypting, this is the 256 bit decryption key. The decryption key may
 *   be generated from the encryption key with CRYPTO_AES_DecryptKey256().
 *
 * @param[in] encrypt
 *   Set to true to encrypt, false to decrypt.
 ******************************************************************************/
void CRYPTO_AES_ECB256(CRYPTO_TypeDef *  crypto,
                       uint8_t *         out,
                       const uint8_t *   in,
                       unsigned int      len,
                       const uint8_t *   key,
                       bool              encrypt)
{
  crypto->CTRL = CRYPTO_CTRL_AES_AES256;
  CRYPTO_AES_ECBx(crypto, out, in, len, key, encrypt, cryptoKey256Bits);
}

/***************************************************************************//**
 * @brief
 *   AES Output feedback (OFB) cipher mode encryption/decryption, 128 bit key.
 *
 * @details
 *   Encryption:
 * @verbatim
 *          InitVector    +----------------+
 *              |         |                |
 *              V         |                V
 *       +--------------+ |        +--------------+
 * Key ->| Block cipher | |  Key ->| Block cipher |
 *       |  encryption  | |        |  encryption  |
 *       +--------------+ |        +--------------+
 *              |         |                |
 *              |---------+                |
 *              V                          V
 * Plaintext ->XOR            Plaintext ->XOR
 *              |                          |
 *              V                          V
 *         Ciphertext                 Ciphertext
 * @endverbatim
 *   Decryption:
 * @verbatim
 *          InitVector    +----------------+
 *              |         |                |
 *              V         |                V
 *       +--------------+ |        +--------------+
 * Key ->| Block cipher | |  Key ->| Block cipher |
 *       |  encryption  | |        |  encryption  |
 *       +--------------+ |        +--------------+
 *              |         |                |
 *              |---------+                |
 *              V                          V
 * Ciphertext ->XOR           Ciphertext ->XOR
 *              |                          |
 *              V                          V
 *          Plaintext                  Plaintext
 * @endverbatim
 *   See general comments on layout and byte ordering of parameters.
 *
 * @param[in]  crypto
 *   A pointer to the CRYPTO peripheral register block.
 *
 * @param[out] out
 *   A buffer to place encrypted/decrypted data. Must be at least @p len long. It
 *   may be set equal to @p in, in which case the input buffer is overwritten.
 *
 * @param[in] in
 *   A buffer holding data to encrypt/decrypt. Must be at least @p len long.
 *
 * @param[in] len
 *   A number of bytes to encrypt/decrypt. Must be a multiple of 16.
 *
 * @param[in] key
 *   128 bit encryption key.
 *
 * @param[in] iv
 *   128 bit initialization vector to use.
 ******************************************************************************/
void CRYPTO_AES_OFB128(CRYPTO_TypeDef *  crypto,
                       uint8_t *         out,
                       const uint8_t *   in,
                       unsigned int      len,
                       const uint8_t *   key,
                       const uint8_t *   iv)
{
  crypto->CTRL = CRYPTO_CTRL_AES_AES128;
  CRYPTO_AES_OFBx(crypto, out, in, len, key, iv, cryptoKey128Bits);
}

/***************************************************************************//**
 * @brief
 *   AES Output feedback (OFB) cipher mode encryption/decryption, 256 bit key.
 *
 * @details
 *   See CRYPTO_AES_OFB128() for OFB figure.
 *
 *   See general comments on layout and byte ordering of parameters.
 *
 * @param[in]  crypto
 *   A pointer to CRYPTO peripheral register block.
 *
 * @param[out] out
 *   A buffer to place encrypted/decrypted data. Must be at least @p len long. It
 *   may be set equal to @p in, in which case the input buffer is overwritten.
 *
 * @param[in] in
 *   A buffer holding data to encrypt/decrypt. Must be at least @p len long.
 *
 * @param[in] len
 *   A number of bytes to encrypt/decrypt. Must be a multiple of 16.
 *
 * @param[in] key
 *   256 bit encryption key.
 *
 * @param[in] iv
 *   128 bit initialization vector to use.
 ******************************************************************************/
void CRYPTO_AES_OFB256(CRYPTO_TypeDef *  crypto,
                       uint8_t *         out,
                       const uint8_t *   in,
                       unsigned int      len,
                       const uint8_t *   key,
                       const uint8_t *   iv)
{
  crypto->CTRL = CRYPTO_CTRL_AES_AES256;
  CRYPTO_AES_OFBx(crypto, out, in, len, key, iv, cryptoKey256Bits);
}

/*******************************************************************************
 **************************   LOCAL FUNCTIONS   *******************************
 ******************************************************************************/

/***************************************************************************//**
 * @brief
 *   Read 128 bits of data from a DATAX register in the CRYPTO module.
 *
 * @param[in]  dataReg   The 128 bit DATA register.
 * @param[out] val       Location where to store the value in memory.
 ******************************************************************************/
__STATIC_INLINE void CRYPTO_DataReadUnaligned(volatile uint32_t * reg,
                                              uint8_t * val)
{
  /* Check data is 32bit aligned, if not, read into temporary buffer and
     then move to user buffer. */
  if ((uint32_t)val & 0x3) {
    uint32_t temp[4];
    CRYPTO_DataRead(reg, temp);
    memcpy(val, temp, 16);
  } else {
    CRYPTO_DataRead(reg, (uint32_t *)val);
  }
}

/***************************************************************************//**
 * @brief
 *   Write 128 bits of data to a DATAX register in the CRYPTO module.
 *
 * @param[in]  dataReg    The 128 bit DATA register.
 * @param[in]  val        Pointer to value to write to the DATA register.
 ******************************************************************************/
__STATIC_INLINE void CRYPTO_DataWriteUnaligned(volatile uint32_t * reg,
                                               const uint8_t * val)
{
  /* Check data is 32bit aligned, if not move to temporary buffer before
     writing.*/
  if ((uint32_t)val & 0x3) {
    uint32_t temp[4];
    memcpy(temp, val, 16);
    CRYPTO_DataWrite(reg, temp);
  } else {
    CRYPTO_DataWrite(reg, (const uint32_t *)val);
  }
}

/***************************************************************************//**
 * @brief
 *   Set the key value to be used by the CRYPTO module.
 *
 * @details
 *   Write 128 or 256 bit key to the KEYBUF register in the crypto module.
 *
 * @param[in]  crypto
 *   A pointer to the CRYPTO peripheral register block.
 *
 * @param[in]  val
 *   Pointer to value to write to the KEYBUF register.
 *
 * @param[in]  keyWidth
 *   Key width - 128 or 256 bits.
 ******************************************************************************/
__STATIC_INLINE
void CRYPTO_KeyBufWriteUnaligned(CRYPTO_TypeDef          *crypto,
                                 const uint8_t *          val,
                                 CRYPTO_KeyWidth_TypeDef  keyWidth)
{
  /* Check if key val buffer is 32bit aligned, if not move to temporary
     aligned buffer before writing.*/
  if ((uint32_t)val & 0x3) {
    CRYPTO_KeyBuf_TypeDef temp;
    if (keyWidth == cryptoKey128Bits) {
      memcpy(temp, val, 16);
    } else {
      memcpy(temp, val, 32);
    }
    CRYPTO_KeyBufWrite(crypto, temp, keyWidth);
  } else {
    CRYPTO_KeyBufWrite(crypto, (uint32_t*)val, keyWidth);
  }
}

/***************************************************************************//**
 * @brief
 *   Cipher-block chaining (CBC) cipher mode encryption/decryption, 128/256 bit key.
 *
 * @details
 *   See CRYPTO_AES_CBC128() for CBC figure.
 *
 *   See general comments on layout and byte ordering of parameters.
 *
 * @param[in]  crypto
 *   A pointer to the CRYPTO peripheral register block.
 *
 * @param[out] out
 *   A buffer to place encrypted/decrypted data. Must be at least @p len long. It
 *   may be set equal to @p in, in which case the input buffer is overwritten.
 *
 * @param[in] in
 *   A buffer holding data to encrypt/decrypt. Must be at least @p len long.
 *
 * @param[in] len
 *   A number of bytes to encrypt/decrypt. Must be a multiple of 16.
 *
 * @param[in] key
 *   When encrypting, this is the 256 bit encryption key. When
 *   decrypting, this is the 256 bit decryption key. The decryption key may
 *   be generated from the encryption key with CRYPTO_AES_DecryptKey256().
 *
 * @param[in] iv
 *   128 bit initialization vector to use.
 *
 * @param[in] encrypt
 *   Set to true to encrypt, false to decrypt.
 *
 * @param[in] keyWidth
 *   Set to cryptoKey128Bits or cryptoKey256Bits.
 ******************************************************************************/
static void CRYPTO_AES_CBCx(CRYPTO_TypeDef *  crypto,
                            uint8_t *         out,
                            const uint8_t *   in,
                            unsigned int      len,
                            const uint8_t *   key,
                            const uint8_t *   iv,
                            bool              encrypt,
                            CRYPTO_KeyWidth_TypeDef keyWidth)
{
  EFM_ASSERT((len % CRYPTO_AES_BLOCKSIZE) == 0U);

  /* Initialize control registers. */
  crypto->WAC = 0;

  CRYPTO_KeyBufWriteUnaligned(crypto, key, keyWidth);

  if (encrypt) {
    CRYPTO_DataWriteUnaligned(&crypto->DATA0, iv);

    crypto->SEQ0 = CRYPTO_CMD_INSTR_DATA1TODATA0XOR << _CRYPTO_SEQ0_INSTR0_SHIFT
                   | CRYPTO_CMD_INSTR_AESENC        << _CRYPTO_SEQ0_INSTR1_SHIFT;

    CRYPTO_AES_ProcessLoop(crypto, len,
                           &crypto->DATA1, in,
                           &crypto->DATA0, out);
  } else {
    CRYPTO_DataWriteUnaligned(&crypto->DATA2, iv);

    crypto->SEQ0 = CRYPTO_CMD_INSTR_DATA1TODATA0      << _CRYPTO_SEQ0_INSTR0_SHIFT
                   | CRYPTO_CMD_INSTR_AESDEC          << _CRYPTO_SEQ0_INSTR1_SHIFT
                   | CRYPTO_CMD_INSTR_DATA2TODATA0XOR << _CRYPTO_SEQ0_INSTR2_SHIFT
                   | CRYPTO_CMD_INSTR_DATA1TODATA2    << _CRYPTO_SEQ0_INSTR3_SHIFT;

    crypto->SEQ1 = 0;

    /* The following call is equivalent to the last call in the
       'if( encrypt )' branch. However moving this
       call outside the conditional scope results in slightly poorer
       performance for some compiler optimizations. */
    CRYPTO_AES_ProcessLoop(crypto, len,
                           &crypto->DATA1, in,
                           &crypto->DATA0, out);
  }
}

/***************************************************************************//**
 * @brief
 *   Cipher feedback (CFB) cipher mode encryption/decryption, 128/256 bit key.
 *
 * @details
 *   See CRYPTO_AES_CFB128() for CFB figure.
 *
 *   See general comments on layout and byte ordering of parameters.
 *
 * @param[in]  crypto
 *   A pointer to the CRYPTO peripheral register block.
 *
 * @param[out] out
 *   A buffer to place encrypted/decrypted data. Must be at least @p len long. It
 *   may be set equal to @p in, in which case the input buffer is overwritten.
 *
 * @param[in] in
 *   A buffer holding data to encrypt/decrypt. Must be at least @p len long.
 *
 * @param[in] len
 *   A number of bytes to encrypt/decrypt. Must be a multiple of 16.
 *
 * @param[in] key
 *   256 bit encryption key is used for both encryption and decryption modes.
 *
 * @param[in] iv
 *   128 bit initialization vector to use.
 *
 * @param[in] encrypt
 *   Set to true to encrypt, false to decrypt.
 *
 * @param[in] keyWidth
 *   Set to cryptoKey128Bits or cryptoKey256Bits.
 ******************************************************************************/
static void CRYPTO_AES_CFBx(CRYPTO_TypeDef *  crypto,
                            uint8_t *         out,
                            const uint8_t *   in,
                            unsigned int      len,
                            const uint8_t *   key,
                            const uint8_t *   iv,
                            bool              encrypt,
                            CRYPTO_KeyWidth_TypeDef keyWidth)
{
  EFM_ASSERT((len % CRYPTO_AES_BLOCKSIZE) == 0U);

  /* Initialize control registers. */
  crypto->WAC = 0;

  /* Load the key. */
  CRYPTO_KeyBufWriteUnaligned(crypto, key, keyWidth);

  /* Load instructions to the CRYPTO sequencer. */
  if (encrypt) {
    /* Load IV */
    CRYPTO_DataWriteUnaligned(&crypto->DATA0, iv);

    crypto->SEQ0 = CRYPTO_CMD_INSTR_AESENC            << _CRYPTO_SEQ0_INSTR0_SHIFT
                   | CRYPTO_CMD_INSTR_DATA1TODATA0XOR << _CRYPTO_SEQ0_INSTR1_SHIFT;

    CRYPTO_AES_ProcessLoop(crypto, len,
                           &crypto->DATA1, in,
                           &crypto->DATA0, out
                           );
  } else {
    /* Load IV */
    CRYPTO_DataWriteUnaligned(&crypto->DATA2, iv);

    crypto->SEQ0 = CRYPTO_CMD_INSTR_DATA2TODATA0      << _CRYPTO_SEQ0_INSTR0_SHIFT
                   | CRYPTO_CMD_INSTR_AESENC          << _CRYPTO_SEQ0_INSTR1_SHIFT
                   | CRYPTO_CMD_INSTR_DATA1TODATA0XOR << _CRYPTO_SEQ0_INSTR2_SHIFT
                   | CRYPTO_CMD_INSTR_DATA1TODATA2    << _CRYPTO_SEQ0_INSTR3_SHIFT;
    crypto->SEQ1 = 0;

    CRYPTO_AES_ProcessLoop(crypto, len,
                           &crypto->DATA1, in,
                           &crypto->DATA0, out
                           );
  }
}

/***************************************************************************//**
 * @brief
 *   Counter (CTR) cipher mode encryption/decryption, 128/256 bit key.
 *
 * @details
 *   See CRYPTO_AES_CTR128() for CTR figure.
 *
 *   See general comments on layout and byte ordering of parameters.
 *
 * @param[in]  crypto
 *   A pointer to the CRYPTO peripheral register block.
 *
 * @param[out] out
 *   A buffer to place encrypted/decrypted data. Must be at least @p len long. It
 *   may be set equal to @p in, in which case the input buffer is overwritten.
 *
 * @param[in] in
 *   A buffer holding data to encrypt/decrypt. Must be at least @p len long.
 *
 * @param[in] len
 *   A number of bytes to encrypt/decrypt. Must be a multiple of 16.
 *
 * @param[in] key
 *   256 bit encryption key.
 *
 * @param[in,out] ctr
 *   128 bit initial counter value. The counter is updated after each AES
 *   block encoding through use of @p ctrFunc.
 *
 * @param[in] ctrFunc
 *   A function used to update counter value. Not supported by CRYPTO.
 *   This parameter is included for backwards compatibility with
 *   the EFM32 em_aes.h API.
 *
 * @param[in] keyWidth
 *   Set to cryptoKey128Bits or cryptoKey256Bits.
 ******************************************************************************/
static void CRYPTO_AES_CTRx(CRYPTO_TypeDef *  crypto,
                            uint8_t *         out,
                            const uint8_t *   in,
                            unsigned int      len,
                            const uint8_t *   key,
                            uint8_t *         ctr,
                            CRYPTO_AES_CtrFuncPtr_TypeDef ctrFunc,
                            CRYPTO_KeyWidth_TypeDef keyWidth)
{
  (void) ctrFunc;

  EFM_ASSERT((len % CRYPTO_AES_BLOCKSIZE) == 0U);

  /* Initialize control registers. */
  crypto->CTRL |= CRYPTO_CTRL_INCWIDTH_INCWIDTH4;
  crypto->WAC   = 0;

  CRYPTO_KeyBufWriteUnaligned(crypto, key, keyWidth);

  CRYPTO_DataWriteUnaligned(&crypto->DATA1, ctr);

  crypto->SEQ0 = CRYPTO_CMD_INSTR_DATA1TODATA0    << _CRYPTO_SEQ0_INSTR0_SHIFT
                 | CRYPTO_CMD_INSTR_AESENC        << _CRYPTO_SEQ0_INSTR1_SHIFT
                 | CRYPTO_CMD_INSTR_DATA0TODATA3  << _CRYPTO_SEQ0_INSTR2_SHIFT
                 | CRYPTO_CMD_INSTR_DATA1INC      << _CRYPTO_SEQ0_INSTR3_SHIFT;

  crypto->SEQ1 = CRYPTO_CMD_INSTR_DATA2TODATA0XOR << _CRYPTO_SEQ1_INSTR4_SHIFT;

  CRYPTO_AES_ProcessLoop(crypto, len,
                         &crypto->DATA2, in,
                         &crypto->DATA0, out);

  CRYPTO_DataReadUnaligned(&crypto->DATA1, ctr);
}

/***************************************************************************//**
 * @brief
 *   Electronic Codebook (ECB) cipher mode encryption/decryption, 128/256 bit key.
 *
 * @details
 *   See CRYPTO_AES_ECB128() for ECB figure.
 *
 *   See general comments on layout and byte ordering of parameters.
 *
 * @param[in]  crypto
 *   A pointer to the CRYPTO peripheral register block.
 *
 * @param[out] out
 *   A buffer to place encrypted/decrypted data. Must be at least @p len long. It
 *   may be set equal to @p in, in which case the input buffer is overwritten.
 *
 * @param[in] in
 *   A buffer holding data to encrypt/decrypt. Must be at least @p len long.
 *
 * @param[in] len
 *   A number of bytes to encrypt/decrypt. Must be a multiple of 16.
 *
 * @param[in] key
 *   When encrypting, this is the 256 bit encryption key. When
 *   decrypting, this is the 256 bit decryption key. The decryption key may
 *   be generated from the encryption key with CRYPTO_AES_DecryptKey256().
 *
 * @param[in] encrypt
 *   Set to true to encrypt, false to decrypt.
 *
 * @param[in] keyWidth
 *   Set to cryptoKey128Bits or cryptoKey256Bits.
 ******************************************************************************/
static void CRYPTO_AES_ECBx(CRYPTO_TypeDef *  crypto,
                            uint8_t *         out,
                            const uint8_t *   in,
                            unsigned int      len,
                            const uint8_t *   key,
                            bool              encrypt,
                            CRYPTO_KeyWidth_TypeDef keyWidth)
{
  EFM_ASSERT((len % CRYPTO_AES_BLOCKSIZE) == 0U);

  crypto->WAC = 0;

  CRYPTO_KeyBufWriteUnaligned(crypto, key, keyWidth);

  if (encrypt) {
    crypto->SEQ0 = CRYPTO_CMD_INSTR_AESENC         << _CRYPTO_SEQ0_INSTR0_SHIFT
                   | CRYPTO_CMD_INSTR_DATA0TODATA1 << _CRYPTO_SEQ0_INSTR1_SHIFT;
  } else {
    crypto->SEQ0 = CRYPTO_CMD_INSTR_AESDEC         << _CRYPTO_SEQ0_INSTR0_SHIFT
                   | CRYPTO_CMD_INSTR_DATA0TODATA1 << _CRYPTO_SEQ0_INSTR1_SHIFT;
  }

  CRYPTO_AES_ProcessLoop(crypto, len,
                         &crypto->DATA0, in,
                         &crypto->DATA1, out);
}

/***************************************************************************//**
 * @brief
 *   Output feedback (OFB) cipher mode encryption/decryption, 128/256 bit key.
 *
 * @details
 *   See CRYPTO_AES_OFB128() for OFB figure.
 *
 *   See general comments on layout and byte ordering of parameters.
 *
 * @param[in]  crypto
 *   A pointer to CRYPTO peripheral register block.
 *
 * @param[out] out
 *   A buffer to place encrypted/decrypted data. Must be at least @p len long. It
 *   may be set equal to @p in, in which case the input buffer is overwritten.
 *
 * @param[in] in
 *   A buffer holding data to encrypt/decrypt. Must be at least @p len long.
 *
 * @param[in] len
 *   A number of bytes to encrypt/decrypt. Must be a multiple of 16.
 *
 * @param[in] key
 *   256 bit encryption key.
 *
 * @param[in] iv
 *   128 bit initialization vector to use.
 *
 * @param[in] keyWidth
 *   Set to cryptoKey128Bits or cryptoKey256Bits.
 ******************************************************************************/
static void CRYPTO_AES_OFBx(CRYPTO_TypeDef *  crypto,
                            uint8_t *         out,
                            const uint8_t *   in,
                            unsigned int      len,
                            const uint8_t *   key,
                            const uint8_t *   iv,
                            CRYPTO_KeyWidth_TypeDef keyWidth)
{
  EFM_ASSERT((len % CRYPTO_AES_BLOCKSIZE) == 0U);

  crypto->WAC = 0;

  CRYPTO_KeyBufWriteUnaligned(crypto, key, keyWidth);

  CRYPTO_DataWriteUnaligned(&crypto->DATA2, iv);

  crypto->SEQ0 = CRYPTO_CMD_INSTR_DATA0TODATA1   << _CRYPTO_SEQ0_INSTR0_SHIFT
                 | CRYPTO_CMD_INSTR_DATA2TODATA0 << _CRYPTO_SEQ0_INSTR1_SHIFT
                 | CRYPTO_CMD_INSTR_AESENC       << _CRYPTO_SEQ0_INSTR2_SHIFT
                 | CRYPTO_CMD_INSTR_DATA0TODATA2 << _CRYPTO_SEQ0_INSTR3_SHIFT;
  crypto->SEQ1 = CRYPTO_CMD_INSTR_DATA1TODATA0XOR << _CRYPTO_SEQ1_INSTR4_SHIFT
                 | CRYPTO_CMD_INSTR_DATA0TODATA1  << _CRYPTO_SEQ1_INSTR5_SHIFT;

  CRYPTO_AES_ProcessLoop(crypto, len,
                         &crypto->DATA0, in,
                         &crypto->DATA1, out);
}

/***************************************************************************//**
 * @brief
 *   Perform generic AES loop.
 *
 * @details
 *   Function loads given register with provided input data. Triggers CRYPTO to
 *   perform sequence of instructions and read specified output register to
 *   output buffer.
 *
 * @param[in]  crypto
 *   A pointer to the CRYPTO peripheral register block.
 *
 * @param[in] len
 *   A number of bytes to encrypt/decrypt. Must be a multiple of 16.
 *
 * @param[in] inReg
 *   An input register - one of DATA0,DATA1,DATA2,DATA3
 *
 * @param[in] in
 *   A buffer holding data to encrypt/decrypt. Must be at least @p len long.
 *
 * @param[in] outReg
 *   An output register - one of DATA0,DATA1,DATA2,DATA3
 *
 * @param[out] out
 *   A buffer to place encrypted/decrypted data. Must be at least @p len long. It
 *   may be set equal to @p in, in which case the input buffer is overwritten.
 ******************************************************************************/
__STATIC_INLINE void CRYPTO_AES_ProcessLoop(CRYPTO_TypeDef *        crypto,
                                            uint32_t                len,
                                            CRYPTO_DataReg_TypeDef  inReg,
                                            const uint8_t  *        in,
                                            CRYPTO_DataReg_TypeDef  outReg,
                                            uint8_t *               out)
{
  len /= CRYPTO_AES_BLOCKSIZE;
  crypto->SEQCTRL = 16UL << _CRYPTO_SEQCTRL_LENGTHA_SHIFT;

  while (len > 0UL) {
    len--;
    /* Load data and trigger encryption. */
    CRYPTO_DataWriteUnaligned(inReg, in);

    crypto->CMD = CRYPTO_CMD_SEQSTART;

    /* Save encrypted/decrypted data. */
    CRYPTO_DataReadUnaligned(outReg, out);

    out += 16;
    in  += 16;
  }
}

/** @} (end addtogroup CRYPTO) */
/** @} (end addtogroup emlib) */

#endif /* defined(CRYPTO_COUNT) && (CRYPTO_COUNT > 0) */
