/***************************************************************************//**
 * @file em_crypto.c
 * @brief Cryptography accelerator peripheral API
 * @version 5.1.2
 *******************************************************************************
 * @section License
 * <b>Copyright 2016 Silicon Laboratories, Inc. http://www.silabs.com</b>
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

#define CRYPTO_INSTRUCTIONS_PER_REG              (4)
#define CRYPTO_INSTRUCTIONS_MAX                  (12)
#define CRYPTO_INSTRUCTION_REGS                  (CRYPTO_INSTRUCTIONS_MAX/CRYPTO_INSTRUCTIONS_PER_REG)

#define CRYPTO_SHA1_BLOCK_SIZE_IN_BITS           (512)
#define CRYPTO_SHA1_BLOCK_SIZE_IN_BYTES          (CRYPTO_SHA1_BLOCK_SIZE_IN_BITS/8)
#define CRYPTO_SHA1_BLOCK_SIZE_IN_32BIT_WORDS    (CRYPTO_SHA1_BLOCK_SIZE_IN_BYTES/sizeof(uint32_t))
#define CRYPTO_SHA1_DIGEST_SIZE_IN_32BIT_WORDS   (CRYPTO_SHA1_DIGEST_SIZE_IN_BYTES/sizeof(uint32_t))

#define CRYPTO_SHA256_BLOCK_SIZE_IN_BITS         (512)
#define CRYPTO_SHA256_BLOCK_SIZE_IN_BYTES        (CRYPTO_SHA256_BLOCK_SIZE_IN_BITS/8)
#define CRYPTO_SHA256_BLOCK_SIZE_IN_32BIT_WORDS  (CRYPTO_SHA256_BLOCK_SIZE_IN_BYTES/sizeof(uint32_t))

#define CRYPTO_SHA256_DIGEST_SIZE_IN_32BIT_WORDS (CRYPTO_SHA256_DIGEST_SIZE_IN_BYTES/sizeof(uint32_t))

#define PARTIAL_OPERAND_WIDTH_LOG2               (7)  /* 2^7 = 128 */
#define PARTIAL_OPERAND_WIDTH                    (1<<PARTIAL_OPERAND_WIDTH_LOG2)
#define PARTIAL_OPERAND_WIDTH_MASK               (PARTIAL_OPERAND_WIDTH-1)
#define PARTIAL_OPERAND_WIDTH_IN_BYTES           (PARTIAL_OPERAND_WIDTH/8)
#define PARTIAL_OPERAND_WIDTH_IN_32BIT_WORDS     (PARTIAL_OPERAND_WIDTH_IN_BYTES/sizeof(uint32_t))

#define SWAP32(x)                                (__REV(x))

#define CRYPTO_AES_BLOCKSIZE                     (16)

/*******************************************************************************
 ***********************   STATIC FUNCTIONS   **********************************
 ******************************************************************************/

static inline void CRYPTO_AES_ProcessLoop(CRYPTO_TypeDef *crypto,
                                          uint32_t len,
                                          CRYPTO_DataReg_TypeDef inReg,
                                          uint32_t * in,
                                          CRYPTO_DataReg_TypeDef outReg,
                                          uint32_t * out);

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

#ifdef USE_VARIABLE_SIZED_DATA_LOADS
/***************************************************************************//**
 * @brief
 *   Write variable sized 32 bit data array (max 128 bits) to a DATAX register
 *
 * @details
 *   Write variable sized 32 bit array (max 128 bits / 4 words) to a DATAX
 *   register in the CRYPTO module.
 *
 * @param[in]  dataReg    The 128 bits DATA register.
 * @param[in]  val        Value of the data to write to the DATA register.
 * @param[in]  valSize    Size of @ref val in number of 32bit words.
 ******************************************************************************/
__STATIC_INLINE
void CRYPTO_DataWriteVariableSize(CRYPTO_DataReg_TypeDef    dataReg,
                                  const CRYPTO_Data_TypeDef val,
                                  int                       valSize)
{
  int i;
  volatile uint32_t * reg = (volatile uint32_t *) dataReg;

  if (valSize < 4)
  {
    /* Non optimal write of data. */
    for (i = 0; i < valSize; i++)
      *reg = *val++;
    for (; i < 4; i++)
      *reg = 0;
  }
  else
  {
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
 *   Pointer to CRYPTO peripheral register block.
 *
 * @param[in]  modulusId
 *   Modulus identifier.
 ******************************************************************************/
void CRYPTO_ModulusSet(CRYPTO_TypeDef *          crypto,
                       CRYPTO_ModulusId_TypeDef  modulusId)
{
  uint32_t temp = crypto->WAC & (~(_CRYPTO_WAC_MODULUS_MASK | _CRYPTO_WAC_MODOP_MASK));

  switch (modulusId)
  {
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
      crypto->WAC = temp | modulusId | CRYPTO_WAC_MODOP_BINARY;
      break;

    case cryptoModulusEccP256:
    case cryptoModulusEccP224:
    case cryptoModulusEccP192:
#ifdef _CRYPTO_WAC_MODULUS_ECCPRIME256P
    case cryptoModulusEccP256Order:
    case cryptoModulusEccP224Order:
    case cryptoModulusEccP192Order:
#endif
      crypto->WAC = temp | modulusId | CRYPTO_WAC_MODOP_REGULAR;
      break;

    default:
      /* Unknown modulus identifier. */
      EFM_ASSERT(0);
  }
}

/***************************************************************************//**
 * @brief
 *   Read the key value currently used by the CRYPTO module.
 *
 * @details
 *   Read 128 bits or 256 bits from KEY register in the CRYPTO module.
 *
 * @param[in]  crypto
 *   Pointer to CRYPTO peripheral register block.
 *
 * @param[in]  val
 *   Value of the data to write to the KEYBUF register.
 *
 * @param[in]  keyWidth
 *   Key width - 128 or 256 bits
 ******************************************************************************/
void CRYPTO_KeyRead(CRYPTO_TypeDef *         crypto,
                    CRYPTO_KeyBuf_TypeDef    val,
                    CRYPTO_KeyWidth_TypeDef  keyWidth)
{
  EFM_ASSERT(val);

  CRYPTO_BurstFromCrypto(&crypto->KEY, &val[0]);
  if (keyWidth == cryptoKey256Bits)
  {
    CRYPTO_BurstFromCrypto(&crypto->KEY, &val[4]);
  }
}

/***************************************************************************//**
 * @brief
 *   Perform a SHA-1 hash operation on a message.
 *
 * @details
 *   This function performs a SHA-1 hash operation on the message specified by
 *   msg with length msgLen, and returns the message digest in msgDigest.
 *
 * @param[in]  crypto
 *   Pointer to CRYPTO peripheral register block.
 *
 * @param[in]  msg
 *   Message to hash.
 *
 * @param[in]  msgLen
 *   Length of message in bytes.
 *
 * @param[out] msgDigest
 *   Message digest.
 ******************************************************************************/
void CRYPTO_SHA_1(CRYPTO_TypeDef *             crypto,
                  const uint8_t *              msg,
                  uint64_t                     msgLen,
                  CRYPTO_SHA1_Digest_TypeDef   msgDigest)
{
  uint32_t  temp;
  uint32_t  len;
  int       blockLen;
  uint32_t  shaBlock[CRYPTO_SHA1_BLOCK_SIZE_IN_32BIT_WORDS]=
  {
    /* Initial value */
    0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476, 0xc3d2e1f0
  };
  uint8_t * p8ShaBlock = (uint8_t *) shaBlock;

  /* Initialize crypto module to do SHA-1. */
  crypto->CTRL     = CRYPTO_CTRL_SHA_SHA1;
  crypto->SEQCTRL  = 0;
  crypto->SEQCTRLB = 0;

  /* Set result width of MADD32 operation. */
  CRYPTO_ResultWidthSet(crypto, cryptoResult256Bits);

  /* Write init value to DDATA1.  */
  CRYPTO_DDataWrite(&crypto->DDATA1, shaBlock);

  /* Copy data to DDATA0 and select DDATA0 and DDATA1 for SHA operation. */
  CRYPTO_EXECUTE_2(crypto,
                   CRYPTO_CMD_INSTR_DDATA1TODDATA0,
                   CRYPTO_CMD_INSTR_SELDDATA0DDATA1);

  len = msgLen;

  while (len >= CRYPTO_SHA1_BLOCK_SIZE_IN_BYTES)
  {
    /* Write block to QDATA1.  */
    CRYPTO_QDataWrite(&crypto->QDATA1BIG, (uint32_t *) msg);

    /* Execute SHA */
    CRYPTO_EXECUTE_3(crypto,
                     CRYPTO_CMD_INSTR_SHA,
                     CRYPTO_CMD_INSTR_MADD32,
                     CRYPTO_CMD_INSTR_DDATA0TODDATA1);

    len -= CRYPTO_SHA1_BLOCK_SIZE_IN_BYTES;
    msg += CRYPTO_SHA1_BLOCK_SIZE_IN_BYTES;
  }

  blockLen = 0;

  /* Build the last (or second to last) block */
  for (; len; len--)
    p8ShaBlock[blockLen++] = *msg++;

  /* append the '1' bit */
  p8ShaBlock[blockLen++] = 0x80;

  /* if the length is currently above 56 bytes we append zeros
   * then compress.  Then we can fall back to padding zeros and length
   * encoding like normal.
   */
  if (blockLen > 56)
  {
    while (blockLen < 64)
      p8ShaBlock[blockLen++] = 0;

    /* Write block to QDATA1BIG. */
    CRYPTO_QDataWrite(&crypto->QDATA1BIG, shaBlock);

    /* Execute SHA */
    CRYPTO_EXECUTE_3(crypto,
                     CRYPTO_CMD_INSTR_SHA,
                     CRYPTO_CMD_INSTR_MADD32,
                     CRYPTO_CMD_INSTR_DDATA0TODDATA1);
    blockLen = 0;
  }

  /* pad upto 56 bytes of zeroes */
  while (blockLen < 56)
    p8ShaBlock[blockLen++] = 0;

  /* And finally, encode the message length. */
  {
    uint64_t msgLenInBits = msgLen << 3;
    temp = msgLenInBits >> 32;
    *(uint32_t*)&p8ShaBlock[56] = SWAP32(temp);
    temp = msgLenInBits & 0xFFFFFFFF;
    *(uint32_t*)&p8ShaBlock[60] = SWAP32(temp);
  }

  /* Write block to QDATA1BIG. */
  CRYPTO_QDataWrite(&crypto->QDATA1BIG, shaBlock);

  /* Execute SHA */
  CRYPTO_EXECUTE_3(crypto,
                   CRYPTO_CMD_INSTR_SHA,
                   CRYPTO_CMD_INSTR_MADD32,
                   CRYPTO_CMD_INSTR_DDATA0TODDATA1);

  /* Read resulting message digest from DDATA0BIG.  */
  ((uint32_t*)msgDigest)[0] = crypto->DDATA0BIG;
  ((uint32_t*)msgDigest)[1] = crypto->DDATA0BIG;
  ((uint32_t*)msgDigest)[2] = crypto->DDATA0BIG;
  ((uint32_t*)msgDigest)[3] = crypto->DDATA0BIG;
  ((uint32_t*)msgDigest)[4] = crypto->DDATA0BIG;
  temp = crypto->DDATA0BIG;
  temp = crypto->DDATA0BIG;
  temp = crypto->DDATA0BIG;
}

/***************************************************************************//**
 * @brief
 *   Perform a SHA-256 hash operation on a message.
 *
 * @details
 *   This function performs a SHA-256 hash operation on the message specified
 *   by msg with length msgLen, and returns the message digest in msgDigest.
 *
 * @param[in]  crypto
 *   Pointer to CRYPTO peripheral register block.
 *
 * @param[in]  msg
 *   Message to hash.
 *
 * @param[in]  msgLen
 *   Length of message in bytes.
 *
 * @param[out] msgDigest
 *   Message digest.
 ******************************************************************************/
void CRYPTO_SHA_256(CRYPTO_TypeDef *             crypto,
                    const uint8_t *              msg,
                    uint64_t                     msgLen,
                    CRYPTO_SHA256_Digest_TypeDef msgDigest)
{
  uint32_t  temp;
  uint32_t  len;
  int       blockLen;
  uint32_t  shaBlock[CRYPTO_SHA256_BLOCK_SIZE_IN_32BIT_WORDS]=
  {
    /* Initial value */
    0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
    0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
  };
  uint8_t * p8ShaBlock = (uint8_t *) shaBlock;

  /* Initialize crypyo module to do SHA-256 (SHA-2). */
  crypto->CTRL     = CRYPTO_CTRL_SHA_SHA2;
  crypto->SEQCTRL  = 0;
  crypto->SEQCTRLB = 0;

  /* Set result width of MADD32 operation. */
  CRYPTO_ResultWidthSet(crypto, cryptoResult256Bits);

  /* Write init value to DDATA1.  */
  CRYPTO_DDataWrite(&crypto->DDATA1, shaBlock);

  /* Copy data ot DDATA0 and select DDATA0 and DDATA1 for SHA operation. */
  CRYPTO_EXECUTE_2(crypto,
                   CRYPTO_CMD_INSTR_DDATA1TODDATA0,
                   CRYPTO_CMD_INSTR_SELDDATA0DDATA1);
  len = msgLen;

  while (len >= CRYPTO_SHA256_BLOCK_SIZE_IN_BYTES)
  {
    /* Write block to QDATA1BIG.  */
    CRYPTO_QDataWrite(&crypto->QDATA1BIG, (uint32_t *) msg);

    /* Execute SHA */
    CRYPTO_EXECUTE_3(crypto,
                     CRYPTO_CMD_INSTR_SHA,
                     CRYPTO_CMD_INSTR_MADD32,
                     CRYPTO_CMD_INSTR_DDATA0TODDATA1);

    len -= CRYPTO_SHA256_BLOCK_SIZE_IN_BYTES;
    msg += CRYPTO_SHA256_BLOCK_SIZE_IN_BYTES;
  }

  blockLen = 0;

  /* Build the last (or second to last) block */
  for (; len; len--)
    p8ShaBlock[blockLen++] = *msg++;

  /* append the '1' bit */
  p8ShaBlock[blockLen++] = 0x80;

  /* if the length is currently above 56 bytes we append zeros
   * then compress.  Then we can fall back to padding zeros and length
   * encoding like normal.
   */
  if (blockLen > 56)
  {
    while (blockLen < 64)
      p8ShaBlock[blockLen++] = 0;

    /* Write block to QDATA1BIG. */
    CRYPTO_QDataWrite(&crypto->QDATA1BIG, shaBlock);

    /* Execute SHA */
    CRYPTO_EXECUTE_3(crypto,
                     CRYPTO_CMD_INSTR_SHA,
                     CRYPTO_CMD_INSTR_MADD32,
                     CRYPTO_CMD_INSTR_DDATA0TODDATA1);
    blockLen = 0;
  }

  /* Pad upto 56 bytes of zeroes */
  while (blockLen < 56)
    p8ShaBlock[blockLen++] = 0;

  /* And finally, encode the message length. */
  {
    uint64_t msgLenInBits = msgLen << 3;
    temp = msgLenInBits >> 32;
    *(uint32_t *)&p8ShaBlock[56] = SWAP32(temp);
    temp = msgLenInBits & 0xFFFFFFFF;
    *(uint32_t *)&p8ShaBlock[60] = SWAP32(temp);
  }

  /* Write the final block to QDATA1BIG. */
  CRYPTO_QDataWrite(&crypto->QDATA1BIG, shaBlock);

  /* Execute SHA */
  CRYPTO_EXECUTE_3(crypto,
                   CRYPTO_CMD_INSTR_SHA,
                   CRYPTO_CMD_INSTR_MADD32,
                   CRYPTO_CMD_INSTR_DDATA0TODDATA1);

  /* Read resulting message digest from DDATA0BIG.  */
  CRYPTO_DDataRead(&crypto->DDATA0BIG, (uint32_t *)msgDigest);
}

/***************************************************************************//**
 * @brief
 *   Set 32bit word array to zero.
 *
 * @param[in]  words32bits    Pointer to 32bit word array
 * @param[in]  num32bitWords  Number of 32bit words in array
 ******************************************************************************/
__STATIC_INLINE void cryptoBigintZeroize(uint32_t * words32bits,
                                         int        num32bitWords)
{
  while (num32bitWords--)
    *words32bits++ = 0;
}

/***************************************************************************//**
 * @brief
 *   Increment value of 32bit word array by one.
 *
 * @param[in] words32bits    Pointer to 32bit word array
 * @param[in] num32bitWords  Number of 32bit words in array
 ******************************************************************************/
__STATIC_INLINE void cryptoBigintIncrement(uint32_t * words32bits,
                                           int        num32bitWords)
{
  int i;
  for (i=0; i<num32bitWords; i++)
    if (++words32bits[i] != 0)
      break;
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
 * @param[in]  A        operand A
 * @param[in]  aSize    size of operand A in bits
 * @param[in]  B        operand B
 * @param[in]  bSize    size of operand B in bits
 * @param[out] R        result of multiplication
 * @param[in]  rSize    size of result buffer R in bits
 ******************************************************************************/
void CRYPTO_Mul(CRYPTO_TypeDef * crypto,
                uint32_t * A, int aSize,
                uint32_t * B, int bSize,
                uint32_t * R, int rSize)
{
  int i, j;

  /****************   Initializations   ******************/

#ifdef USE_VARIABLE_SIZED_DATA_LOADS
  int numWordsLastOperandA = (aSize&PARTIAL_OPERAND_WIDTH_MASK)>>5;
  int numPartialOperandsA = numWordsLastOperandA ?
    (aSize >> PARTIAL_OPERAND_WIDTH_LOG2) + 1 :
    aSize >> PARTIAL_OPERAND_WIDTH_LOG2;
  int numWordsLastOperandB = (bSize&PARTIAL_OPERAND_WIDTH_MASK)>>5;
  int numPartialOperandsB = numWordsLastOperandB ?
    (bSize >> PARTIAL_OPERAND_WIDTH_LOG2) + 1 :
    bSize >> PARTIAL_OPERAND_WIDTH_LOG2;
  int numWordsLastOperandR = (rSize&PARTIAL_OPERAND_WIDTH_MASK)>>5;
  int numPartialOperandsR = numWordsLastOperandR ?
    (rSize >> PARTIAL_OPERAND_WIDTH_LOG2) + 1 :
    rSize >> PARTIAL_OPERAND_WIDTH_LOG2;
  EFM_ASSERT(numPartialOperandsA + numPartialOperandsB <= numPartialOperandsR);
#else
  int      numPartialOperandsA = aSize >> PARTIAL_OPERAND_WIDTH_LOG2;
  int      numPartialOperandsB = bSize >> PARTIAL_OPERAND_WIDTH_LOG2;
  EFM_ASSERT((aSize & PARTIAL_OPERAND_WIDTH_MASK) == 0);
  EFM_ASSERT((bSize & PARTIAL_OPERAND_WIDTH_MASK) == 0);
#endif
  EFM_ASSERT(aSize + bSize <= rSize);

  /* Set R to zero. */
  cryptoBigintZeroize(R, rSize >> 5);

  /* Set multiplication width. */
  crypto->WAC = CRYPTO_WAC_MULWIDTH_MUL128 | CRYPTO_WAC_RESULTWIDTH_256BIT;

  /* Setup DMA request signalling in order for MCU to run in parallel with
     CRYPTO instruction sequence execution, and prepare data loading which
     can take place immediately when CRYPTO is ready inside the instruction
     sequence. */
  crypto->CTRL =
    CRYPTO_CTRL_DMA0RSEL_DATA0 | CRYPTO_CTRL_DMA0MODE_FULL |
    CRYPTO_CTRL_DMA1RSEL_DATA1 | CRYPTO_CTRL_DMA1MODE_FULL;

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
                     /* Temporarily load partial operand B(j) to DATA0. */
                     /* R(i+j+1) is still in DATA1 */
                     CRYPTO_CMD_INSTR_DMA0TODATA,
                     /* Move B(j) to DDATA1 */
                     CRYPTO_CMD_INSTR_DDATA2TODDATA1,

                     /* Restore previous partial result (now R(i+j)) */
                     CRYPTO_CMD_INSTR_DATA1TODATA0,

                     /* Load next partial result R(i+j+1) */
                     CRYPTO_CMD_INSTR_DMA1TODATA,

                     /* Execute partial multiplication A(i)inDDATA1 * B(j)inDDATA3*/
                     CRYPTO_CMD_INSTR_MULO,

                     /* Add the result to the previous partial result */
                     /* AND take the previous carry value into account */
                     /* at the right place (bit 128, ADDIC instruction */
                     CRYPTO_CMD_INSTR_SELDDATA0DDATA2,
                     CRYPTO_CMD_INSTR_ADDIC,

                     /* Save the new partial result (lower half) */
                     CRYPTO_CMD_INSTR_DDATA0TODDATA2,
                     CRYPTO_CMD_INSTR_DATATODMA0,
                     /* Reset the operand selector for next*/
                     CRYPTO_CMD_INSTR_SELDDATA2DDATA3
                     );

  /****************   End Initializations   ******************/

  for(i=0; i<numPartialOperandsA; i++)
  {
    /* Load partial operand #1 A>>(i*PARTIAL_OPERAND_WIDTH) to DDATA1. */
#ifdef USE_VARIABLE_SIZED_DATA_LOADS
    if ( (numWordsLastOperandA != 0) && ( i == numPartialOperandsA-1 ) )
      CRYPTO_DataWriteVariableSize(&crypto->DATA2,
                                   &A[i*PARTIAL_OPERAND_WIDTH_IN_32BIT_WORDS],
                                   numWordsLastOperandA);
    else
      CRYPTO_DataWrite(&crypto->DATA2, &A[i*PARTIAL_OPERAND_WIDTH_IN_32BIT_WORDS]);
#else
    CRYPTO_DataWrite(&crypto->DATA2, &A[i*PARTIAL_OPERAND_WIDTH_IN_32BIT_WORDS]);
#endif

    /* Load partial result in R>>(i*PARTIAL_OPERAND_WIDTH) to DATA1. */
#ifdef USE_VARIABLE_SIZED_DATA_LOADS
    if ( (numWordsLastOperandR != 0) && ( i == numPartialOperandsR-1 ) )
      CRYPTO_DataWriteVariableSize(&crypto->DATA1,
                                   &R[i*PARTIAL_OPERAND_WIDTH_IN_32BIT_WORDS],
                                   numWordsLastOperandR);
    else
      CRYPTO_DataWrite(&crypto->DATA1, &R[i*PARTIAL_OPERAND_WIDTH_IN_32BIT_WORDS]);
#else
    CRYPTO_DataWrite(&crypto->DATA1, &R[i*PARTIAL_OPERAND_WIDTH_IN_32BIT_WORDS]);
#endif

    /* Clear carry */
    crypto->CMD = CRYPTO_CMD_INSTR_CCLR;

    /* Setup number of sequence iterations and block size. */
    crypto->SEQCTRL = CRYPTO_SEQCTRL_BLOCKSIZE_16BYTES
      | (PARTIAL_OPERAND_WIDTH_IN_BYTES * numPartialOperandsB);

    /* Execute the MULtiply instruction sequence. */
    CRYPTO_InstructionSequenceExecute(crypto);

    for (j=0; j<numPartialOperandsB; j++)
    {
      /* Load partial operand 2 B>>(j*`PARTIAL_OPERAND_WIDTH) to DDATA2
         (via DATA0). */
#ifdef USE_VARIABLE_SIZED_DATA_LOADS
      if ( (numWordsLastOperandB != 0) && ( j == numPartialOperandsB-1 ) )
        CRYPTO_DataWriteVariableSize(&crypto->DATA0,
                                     &B[j*PARTIAL_OPERAND_WIDTH_IN_32BIT_WORDS],
                                     numWordsLastOperandB);
      else
        CRYPTO_DataWrite(&crypto->DATA0,
                         &B[j*PARTIAL_OPERAND_WIDTH_IN_32BIT_WORDS]);
#else
      CRYPTO_DataWrite(&crypto->DATA0,
                       &B[j*PARTIAL_OPERAND_WIDTH_IN_32BIT_WORDS]);
#endif

      /* Load most significant partial result
         R>>((i+j+1)*`PARTIAL_OPERAND_WIDTH) into DATA1. */
#ifdef USE_VARIABLE_SIZED_DATA_LOADS
      if ( (numWordsLastOperandR != 0) && ( (i+j+1) == numPartialOperandsR-1 ) )
        CRYPTO_DataWriteVariableSize(&crypto->DATA1,
                                     &R[(i+j+1)*PARTIAL_OPERAND_WIDTH_IN_32BIT_WORDS],
                                     numWordsLastOperandR);
      else
        CRYPTO_DataWrite(&crypto->DATA1,
                         &R[(i+j+1)*PARTIAL_OPERAND_WIDTH_IN_32BIT_WORDS]);
#else
      CRYPTO_DataWrite(&crypto->DATA1,
                       &R[(i+j+1)*PARTIAL_OPERAND_WIDTH_IN_32BIT_WORDS]);
#endif
      /* Store least significant partial result */
      CRYPTO_DataRead(&crypto->DATA0,
                      &R[(i+j)*PARTIAL_OPERAND_WIDTH_IN_32BIT_WORDS]);

    } /* for (j=0; j<numPartialOperandsB; j++) */

    /* Handle carry at the end of the inner loop. */
    if (CRYPTO_CarryIsSet(crypto))
      cryptoBigintIncrement(&R[(i+numPartialOperandsB+1)
                               *PARTIAL_OPERAND_WIDTH_IN_32BIT_WORDS],
                            (numPartialOperandsA-i-1)
                            *PARTIAL_OPERAND_WIDTH_IN_32BIT_WORDS);

    CRYPTO_DataRead(&crypto->DATA1,
                    &R[(i+numPartialOperandsB)
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
 *   Please refer to general comments on layout and byte ordering of parameters.
 *
 * @param[in]  crypto
 *   Pointer to CRYPTO peripheral register block.
 *
 * @param[out] out
 *   Buffer to place encrypted/decrypted data. Must be at least @p len long. It
 *   may be set equal to @p in, in which case the input buffer is overwritten.
 *
 * @param[in] in
 *   Buffer holding data to encrypt/decrypt. Must be at least @p len long.
 *
 * @param[in] len
 *   Number of bytes to encrypt/decrypt. Must be a multiple of 16.
 *
 * @param[in] key
 *   When doing encryption, this is the 128 bit encryption key. When doing
 *   decryption, this is the 128 bit decryption key. The decryption key may
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
 *   Please see CRYPTO_AES_CBC128() for CBC figure.
 *
 *   Please refer to general comments on layout and byte ordering of parameters.
 *
 * @param[in]  crypto
 *   Pointer to CRYPTO peripheral register block.
 *
 * @param[out] out
 *   Buffer to place encrypted/decrypted data. Must be at least @p len long. It
 *   may be set equal to @p in, in which case the input buffer is overwritten.
 *
 * @param[in] in
 *   Buffer holding data to encrypt/decrypt. Must be at least @p len long.
 *
 * @param[in] len
 *   Number of bytes to encrypt/decrypt. Must be a multiple of 16.
 *
 * @param[in] key
 *   When doing encryption, this is the 256 bit encryption key. When doing
 *   decryption, this is the 256 bit decryption key. The decryption key may
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
 *   Please refer to general comments on layout and byte ordering of parameters.
 *
 * @param[in]  crypto
 *   Pointer to CRYPTO peripheral register block.
 *
 * @param[out] out
 *   Buffer to place encrypted/decrypted data. Must be at least @p len long. It
 *   may be set equal to @p in, in which case the input buffer is overwritten.
 *
 * @param[in] in
 *   Buffer holding data to encrypt/decrypt. Must be at least @p len long.
 *
 * @param[in] len
 *   Number of bytes to encrypt/decrypt. Must be a multiple of 16.
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
 *   Please see CRYPTO_AES_CFB128() for CFB figure.
 *
 *   Please refer to general comments on layout and byte ordering of parameters.
 *
 * @param[in]  crypto
 *   Pointer to CRYPTO peripheral register block.
 *
 * @param[out] out
 *   Buffer to place encrypted/decrypted data. Must be at least @p len long. It
 *   may be set equal to @p in, in which case the input buffer is overwritten.
 *
 * @param[in] in
 *   Buffer holding data to encrypt/decrypt. Must be at least @p len long.
 *
 * @param[in] len
 *   Number of bytes to encrypt/decrypt. Must be a multiple of 16.
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
 *   Please refer to general comments on layout and byte ordering of parameters.
 *
 * @param[in]  crypto
 *   Pointer to CRYPTO peripheral register block.
 *
 * @param[out] out
 *   Buffer to place encrypted/decrypted data. Must be at least @p len long. It
 *   may be set equal to @p in, in which case the input buffer is overwritten.
 *
 * @param[in] in
 *   Buffer holding data to encrypt/decrypt. Must be at least @p len long.
 *
 * @param[in] len
 *   Number of bytes to encrypt/decrypt. Must be a multiple of 16.
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
 *   Function used to update counter value. Not supported by CRYPTO.
 *   This parameter is included in order for backwards compatibility with
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
 *   Please see CRYPTO_AES_CTR128() for CTR figure.
 *
 *   Please refer to general comments on layout and byte ordering of parameters.
 *
 * @param[in]  crypto
 *   Pointer to CRYPTO peripheral register block.
 *
 * @param[out] out
 *   Buffer to place encrypted/decrypted data. Must be at least @p len long. It
 *   may be set equal to @p in, in which case the input buffer is overwritten.
 *
 * @param[in] in
 *   Buffer holding data to encrypt/decrypt. Must be at least @p len long.
 *
 * @param[in] len
 *   Number of bytes to encrypt/decrypt. Must be a multiple of 16.
 *
 * @param[in] key
 *   256 bit encryption key.
 *
 * @param[in,out] ctr
 *   128 bit initial counter value. The counter is updated after each AES
 *   block encoding through use of @p ctrFunc.
 *
 * @param[in] ctrFunc
 *   Function used to update counter value. Not supported by CRYPTO.
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
 *   Update last 32 bits of 128 bit counter, by incrementing with 1.
 *
 * @details
 *   Notice that no special consideration is given to possible wrap around. If
 *   32 least significant bits are 0xFFFFFFFF, they will be updated to 0x00000000,
 *   ignoring overflow.
 *
 *   Please refer to general comments on layout and byte ordering of parameters.
 *
 * @param[in,out] ctr
 *   Buffer holding 128 bit counter to be updated.
 ******************************************************************************/
void CRYPTO_AES_CTRUpdate32Bit(uint8_t * ctr)
{
  uint32_t * _ctr = (uint32_t *) ctr;

  _ctr[3] = __REV(__REV(_ctr[3]) + 1);
}

/***************************************************************************//**
 * @brief
 *   Generate 128 bit AES decryption key from 128 bit encryption key. The
 *   decryption key is used for some cipher modes when decrypting.
 *
 * @details
 *   Please refer to general comments on layout and byte ordering of parameters.
 *
 * @param[in]  crypto
 *   Pointer to CRYPTO peripheral register block.
 *
 * @param[out] out
 *   Buffer to place 128 bit decryption key. Must be at least 16 bytes long. It
 *   may be set equal to @p in, in which case the input buffer is overwritten.
 *
 * @param[in] in
 *   Buffer holding 128 bit encryption key. Must be at least 16 bytes long.
 ******************************************************************************/
void CRYPTO_AES_DecryptKey128(CRYPTO_TypeDef *  crypto,
                              uint8_t *         out,
                              const uint8_t *   in)
{
  uint32_t       * _out = (uint32_t *) out;
  const uint32_t * _in  = (const uint32_t *) in;

  /* Setup CRYPTO in AES-128 mode. */
  crypto->CTRL = CRYPTO_CTRL_AES_AES128;
  
  /* Load key */
  CRYPTO_BurstToCrypto(&crypto->KEYBUF, &_in[0]);

  /* Do dummy encryption to generate decrypt key */
  crypto->CMD  = CRYPTO_CMD_INSTR_AESENC;

  /* Save decryption key */
  CRYPTO_BurstFromCrypto(&crypto->KEY, &_out[0]);
}

/***************************************************************************//**
 * @brief
 *   Generate 256 bit AES decryption key from 256 bit encryption key. The
 *   decryption key is used for some cipher modes when decrypting.
 *
 * @details
 *   Please refer to general comments on layout and byte ordering of parameters.
 *
 * @param[in]  crypto
 *   Pointer to CRYPTO peripheral register block.
 *
 * @param[out] out
 *   Buffer to place 256 bit decryption key. Must be at least 32 bytes long. It
 *   may be set equal to @p in, in which case the input buffer is overwritten.
 *
 * @param[in] in
 *   Buffer holding 256 bit encryption key. Must be at least 32 bytes long.
 ******************************************************************************/
void CRYPTO_AES_DecryptKey256(CRYPTO_TypeDef *  crypto,
                              uint8_t *         out,
                              const uint8_t *   in)
{
  uint32_t       * _out = (uint32_t *) out;
  const uint32_t * _in  = (const uint32_t *) in;

  /* Setup CRYPTO in AES-256 mode. */
  crypto->CTRL = CRYPTO_CTRL_AES_AES256;
  
  /* Load key */
  CRYPTO_BurstToCrypto(&crypto->KEYBUF, &_in[0]);
  CRYPTO_BurstToCrypto(&crypto->KEYBUF, &_in[4]);

  /* Do dummy encryption to generate decrypt key */
  crypto->CMD  = CRYPTO_CMD_INSTR_AESENC;

  /* Save decryption key */
  CRYPTO_BurstFromCrypto(&crypto->KEY, &_out[0]);
  CRYPTO_BurstFromCrypto(&crypto->KEY, &_out[4]);
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
 *   Please refer to general comments on layout and byte ordering of parameters.
 *
 * @param[in]  crypto
 *   Pointer to CRYPTO peripheral register block.
 *
 * @param[out] out
 *   Buffer to place encrypted/decrypted data. Must be at least @p len long. It
 *   may be set equal to @p in, in which case the input buffer is overwritten.
 *
 * @param[in] in
 *   Buffer holding data to encrypt/decrypt. Must be at least @p len long.
 *
 * @param[in] len
 *   Number of bytes to encrypt/decrypt. Must be a multiple of 16.
 *
 * @param[in] key
 *   When doing encryption, this is the 128 bit encryption key. When doing
 *   decryption, this is the 128 bit decryption key. The decryption key may
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
 *   Please see CRYPTO_AES_ECB128() for ECB figure.
 *
 *   Please refer to general comments on layout and byte ordering of parameters.
 *
 * @param[in]  crypto
 *   Pointer to CRYPTO peripheral register block.
 *
 * @param[out] out
 *   Buffer to place encrypted/decrypted data. Must be at least @p len long. It
 *   may be set equal to @p in, in which case the input buffer is overwritten.
 *
 * @param[in] in
 *   Buffer holding data to encrypt/decrypt. Must be at least @p len long.
 *
 * @param[in] len
 *   Number of bytes to encrypt/decrypt. Must be a multiple of 16.
 *
 * @param[in] key
 *   When doing encryption, this is the 256 bit encryption key. When doing
 *   decryption, this is the 256 bit decryption key. The decryption key may
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
 *   Please refer to general comments on layout and byte ordering of parameters.
 *
 * @param[in]  crypto
 *   Pointer to CRYPTO peripheral register block.
 *
 * @param[out] out
 *   Buffer to place encrypted/decrypted data. Must be at least @p len long. It
 *   may be set equal to @p in, in which case the input buffer is overwritten.
 *
 * @param[in] in
 *   Buffer holding data to encrypt/decrypt. Must be at least @p len long.
 *
 * @param[in] len
 *   Number of bytes to encrypt/decrypt. Must be a multiple of 16.
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
 *   Please see CRYPTO_AES_OFB128() for OFB figure.
 *
 *   Please refer to general comments on layout and byte ordering of parameters.
 *
 * @param[in]  crypto
 *   Pointer to CRYPTO peripheral register block.
 *
 * @param[out] out
 *   Buffer to place encrypted/decrypted data. Must be at least @p len long. It
 *   may be set equal to @p in, in which case the input buffer is overwritten.
 *
 * @param[in] in
 *   Buffer holding data to encrypt/decrypt. Must be at least @p len long.
 *
 * @param[in] len
 *   Number of bytes to encrypt/decrypt. Must be a multiple of 16.
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
 *   Cipher-block chaining (CBC) cipher mode encryption/decryption, 128/256 bit key.
 *
 * @details
 *   Please see CRYPTO_AES_CBC128() for CBC figure.
 *
 *   Please refer to general comments on layout and byte ordering of parameters.
 *
 * @param[in]  crypto
 *   Pointer to CRYPTO peripheral register block.
 *
 * @param[out] out
 *   Buffer to place encrypted/decrypted data. Must be at least @p len long. It
 *   may be set equal to @p in, in which case the input buffer is overwritten.
 *
 * @param[in] in
 *   Buffer holding data to encrypt/decrypt. Must be at least @p len long.
 *
 * @param[in] len
 *   Number of bytes to encrypt/decrypt. Must be a multiple of 16.
 *
 * @param[in] key
 *   When doing encryption, this is the 256 bit encryption key. When doing
 *   decryption, this is the 256 bit decryption key. The decryption key may
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
  EFM_ASSERT(!(len % CRYPTO_AES_BLOCKSIZE));

  /* Initialize control registers. */
  crypto->WAC = 0;

  CRYPTO_KeyBufWrite(crypto, (uint32_t *)key, keyWidth);

  if (encrypt)
  {
    CRYPTO_DataWrite(&crypto->DATA0, (uint32_t *)iv);

    crypto->SEQ0 =
      CRYPTO_CMD_INSTR_DATA1TODATA0XOR << _CRYPTO_SEQ0_INSTR0_SHIFT |
      CRYPTO_CMD_INSTR_AESENC          << _CRYPTO_SEQ0_INSTR1_SHIFT;

    CRYPTO_AES_ProcessLoop(crypto, len,
                           &crypto->DATA1, (uint32_t *) in,
                           &crypto->DATA0, (uint32_t *) out);
  }
  else
  {
    CRYPTO_DataWrite(&crypto->DATA2, (uint32_t *) iv);

    crypto->SEQ0 =
      CRYPTO_CMD_INSTR_DATA1TODATA0    << _CRYPTO_SEQ0_INSTR0_SHIFT |
      CRYPTO_CMD_INSTR_AESDEC          << _CRYPTO_SEQ0_INSTR1_SHIFT |
      CRYPTO_CMD_INSTR_DATA2TODATA0XOR << _CRYPTO_SEQ0_INSTR2_SHIFT |
      CRYPTO_CMD_INSTR_DATA1TODATA2    << _CRYPTO_SEQ0_INSTR3_SHIFT;

    crypto->SEQ1 = 0;

    /* The following call is equivalent to the last call in the
       'if( encrypt )' branch. However moving this
       call outside the conditional scope results in slightly poorer
       performance for some compiler optimizations. */
    CRYPTO_AES_ProcessLoop(crypto, len,
                           &crypto->DATA1, (uint32_t *) in,
                           &crypto->DATA0, (uint32_t *) out);
  }
}

/***************************************************************************//**
 * @brief
 *   Cipher feedback (CFB) cipher mode encryption/decryption, 128/256 bit key.
 *
 * @details
 *   Please see CRYPTO_AES_CFB128() for CFB figure.
 *
 *   Please refer to general comments on layout and byte ordering of parameters.
 *
 * @param[in]  crypto
 *   Pointer to CRYPTO peripheral register block.
 *
 * @param[out] out
 *   Buffer to place encrypted/decrypted data. Must be at least @p len long. It
 *   may be set equal to @p in, in which case the input buffer is overwritten.
 *
 * @param[in] in
 *   Buffer holding data to encrypt/decrypt. Must be at least @p len long.
 *
 * @param[in] len
 *   Number of bytes to encrypt/decrypt. Must be a multiple of 16.
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
  EFM_ASSERT(!(len % CRYPTO_AES_BLOCKSIZE));

  /* Initialize control registers. */
  crypto->WAC = 0;

  /* Load Key */
  CRYPTO_KeyBufWrite(crypto, (uint32_t *)key, keyWidth);

  /* Load instructions to CRYPTO sequencer. */
  if (encrypt)
  {
    /* Load IV */
    CRYPTO_DataWrite(&crypto->DATA0, (uint32_t *)iv);

    crypto->SEQ0 =
      CRYPTO_CMD_INSTR_AESENC          << _CRYPTO_SEQ0_INSTR0_SHIFT |
      CRYPTO_CMD_INSTR_DATA1TODATA0XOR << _CRYPTO_SEQ0_INSTR1_SHIFT;

    CRYPTO_AES_ProcessLoop(crypto, len,
                           &crypto->DATA1, (uint32_t *)in,
                           &crypto->DATA0, (uint32_t *)out
                           );
  }
  else
  {
    /* Load IV */
    CRYPTO_DataWrite(&crypto->DATA2, (uint32_t *)iv);

    crypto->SEQ0 =
      CRYPTO_CMD_INSTR_DATA2TODATA0    << _CRYPTO_SEQ0_INSTR0_SHIFT |
      CRYPTO_CMD_INSTR_AESENC          << _CRYPTO_SEQ0_INSTR1_SHIFT |
      CRYPTO_CMD_INSTR_DATA1TODATA0XOR << _CRYPTO_SEQ0_INSTR2_SHIFT |
      CRYPTO_CMD_INSTR_DATA1TODATA2    << _CRYPTO_SEQ0_INSTR3_SHIFT;
    crypto->SEQ1 = 0;

    CRYPTO_AES_ProcessLoop(crypto, len,
                           &crypto->DATA1, (uint32_t *)in,
                           &crypto->DATA0, (uint32_t *)out
                           );
  }
}

/***************************************************************************//**
 * @brief
 *   Counter (CTR) cipher mode encryption/decryption, 128/256 bit key.
 *
 * @details
 *   Please see CRYPTO_AES_CTR128() for CTR figure.
 *
 *   Please refer to general comments on layout and byte ordering of parameters.
 *
 * @param[in]  crypto
 *   Pointer to CRYPTO peripheral register block.
 *
 * @param[out] out
 *   Buffer to place encrypted/decrypted data. Must be at least @p len long. It
 *   may be set equal to @p in, in which case the input buffer is overwritten.
 *
 * @param[in] in
 *   Buffer holding data to encrypt/decrypt. Must be at least @p len long.
 *
 * @param[in] len
 *   Number of bytes to encrypt/decrypt. Must be a multiple of 16.
 *
 * @param[in] key
 *   256 bit encryption key.
 *
 * @param[in,out] ctr
 *   128 bit initial counter value. The counter is updated after each AES
 *   block encoding through use of @p ctrFunc.
 *
 * @param[in] ctrFunc
 *   Function used to update counter value. Not supported by CRYPTO.
 *   This parameter is included in order for backwards compatibility with
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

  EFM_ASSERT(!(len % CRYPTO_AES_BLOCKSIZE));

  /* Initialize control registers. */
  crypto->CTRL |= CRYPTO_CTRL_INCWIDTH_INCWIDTH4;
  crypto->WAC   = 0;

  CRYPTO_KeyBufWrite(crypto, (uint32_t *)key, keyWidth);

  CRYPTO_DataWrite(&crypto->DATA1, (uint32_t *) ctr);

  crypto->SEQ0 = CRYPTO_CMD_INSTR_DATA1TODATA0  << _CRYPTO_SEQ0_INSTR0_SHIFT |
                 CRYPTO_CMD_INSTR_AESENC        << _CRYPTO_SEQ0_INSTR1_SHIFT |
                 CRYPTO_CMD_INSTR_DATA0TODATA3  << _CRYPTO_SEQ0_INSTR2_SHIFT |
                 CRYPTO_CMD_INSTR_DATA1INC      << _CRYPTO_SEQ0_INSTR3_SHIFT;

  crypto->SEQ1 = CRYPTO_CMD_INSTR_DATA2TODATA0XOR << _CRYPTO_SEQ1_INSTR4_SHIFT;

  CRYPTO_AES_ProcessLoop(crypto, len,
                         &crypto->DATA2, (uint32_t *) in,
                         &crypto->DATA0, (uint32_t *) out);

  CRYPTO_DataRead(&crypto->DATA1, (uint32_t *) ctr);
}

/***************************************************************************//**
 * @brief
 *   Electronic Codebook (ECB) cipher mode encryption/decryption, 128/256 bit key.
 *
 * @details
 *   Please see CRYPTO_AES_ECB128() for ECB figure.
 *
 *   Please refer to general comments on layout and byte ordering of parameters.
 *
 * @param[in]  crypto
 *   Pointer to CRYPTO peripheral register block.
 *
 * @param[out] out
 *   Buffer to place encrypted/decrypted data. Must be at least @p len long. It
 *   may be set equal to @p in, in which case the input buffer is overwritten.
 *
 * @param[in] in
 *   Buffer holding data to encrypt/decrypt. Must be at least @p len long.
 *
 * @param[in] len
 *   Number of bytes to encrypt/decrypt. Must be a multiple of 16.
 *
 * @param[in] key
 *   When doing encryption, this is the 256 bit encryption key. When doing
 *   decryption, this is the 256 bit decryption key. The decryption key may
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
  EFM_ASSERT(!(len % CRYPTO_AES_BLOCKSIZE));

  crypto->WAC = 0;

  CRYPTO_KeyBufWrite(crypto, (uint32_t *)key, keyWidth);

  if (encrypt)
  {
    crypto->SEQ0 =
      (CRYPTO_CMD_INSTR_AESENC       << _CRYPTO_SEQ0_INSTR0_SHIFT |
       CRYPTO_CMD_INSTR_DATA0TODATA1 << _CRYPTO_SEQ0_INSTR1_SHIFT);
  }
  else
  {
    crypto->SEQ0 =
      (CRYPTO_CMD_INSTR_AESDEC       << _CRYPTO_SEQ0_INSTR0_SHIFT |
       CRYPTO_CMD_INSTR_DATA0TODATA1 << _CRYPTO_SEQ0_INSTR1_SHIFT);
  }

  CRYPTO_AES_ProcessLoop(crypto, len,
                         &crypto->DATA0, (uint32_t *) in,
                         &crypto->DATA1, (uint32_t *) out);
}

/***************************************************************************//**
 * @brief
 *   Output feedback (OFB) cipher mode encryption/decryption, 128/256 bit key.
 *
 * @details
 *   Please see CRYPTO_AES_OFB128() for OFB figure.
 *
 *   Please refer to general comments on layout and byte ordering of parameters.
 *
 * @param[in]  crypto
 *   Pointer to CRYPTO peripheral register block.
 *
 * @param[out] out
 *   Buffer to place encrypted/decrypted data. Must be at least @p len long. It
 *   may be set equal to @p in, in which case the input buffer is overwritten.
 *
 * @param[in] in
 *   Buffer holding data to encrypt/decrypt. Must be at least @p len long.
 *
 * @param[in] len
 *   Number of bytes to encrypt/decrypt. Must be a multiple of 16.
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
  EFM_ASSERT(!(len % CRYPTO_AES_BLOCKSIZE));

  crypto->WAC = 0;

  CRYPTO_KeyBufWrite(crypto, (uint32_t *)key, keyWidth);

  CRYPTO_DataWrite(&crypto->DATA2, (uint32_t *)iv);

  crypto->SEQ0 =
    CRYPTO_CMD_INSTR_DATA0TODATA1    << _CRYPTO_SEQ0_INSTR0_SHIFT |
    CRYPTO_CMD_INSTR_DATA2TODATA0    << _CRYPTO_SEQ0_INSTR1_SHIFT |
    CRYPTO_CMD_INSTR_AESENC          << _CRYPTO_SEQ0_INSTR2_SHIFT |
    CRYPTO_CMD_INSTR_DATA0TODATA2    << _CRYPTO_SEQ0_INSTR3_SHIFT;
  crypto->SEQ1 =
    CRYPTO_CMD_INSTR_DATA1TODATA0XOR << _CRYPTO_SEQ1_INSTR4_SHIFT |
    CRYPTO_CMD_INSTR_DATA0TODATA1    << _CRYPTO_SEQ1_INSTR5_SHIFT;

  CRYPTO_AES_ProcessLoop(crypto, len,
                         &crypto->DATA0, (uint32_t *) in,
                         &crypto->DATA1, (uint32_t *) out);
}

/***************************************************************************//**
 * @brief
 *   Function performs generic AES loop.
 *
 * @details
 *   Function loads given register with provided input data. Triggers CRYPTO to
 *   perform sequence of instructions and read specified output register to
 *   output buffer.
 *
 * @param[in]  crypto
 *   Pointer to CRYPTO peripheral register block.
 *
 * @param[in] len
 *   Number of bytes to encrypt/decrypt. Must be a multiple of 16.
 *
 * @param[in] inReg
 *   Input register - one of DATA0,DATA1,DATA2,DATA3
 *
 * @param[in] in
 *   Buffer holding data to encrypt/decrypt. Must be at least @p len long.
 *
 * @param[in] outReg
 *   Output register - one of DATA0,DATA1,DATA2,DATA3
 *
 * @param[out] out
 *   Buffer to place encrypted/decrypted data. Must be at least @p len long. It
 *   may be set equal to @p in, in which case the input buffer is overwritten.
 ******************************************************************************/
static inline void CRYPTO_AES_ProcessLoop(CRYPTO_TypeDef *        crypto,
                                          uint32_t                len,
                                          CRYPTO_DataReg_TypeDef  inReg,
                                          uint32_t *              in,
                                          CRYPTO_DataReg_TypeDef  outReg,
                                          uint32_t *              out)
{
  len /= CRYPTO_AES_BLOCKSIZE;
  crypto->SEQCTRL = 16 << _CRYPTO_SEQCTRL_LENGTHA_SHIFT;

  while (len--)
  {
    /* Load data and trigger encryption */
    CRYPTO_DataWrite(inReg, (uint32_t *)in);

    crypto->CMD = CRYPTO_CMD_SEQSTART;

    /* Save encrypted/decrypted data */
    CRYPTO_DataRead(outReg, (uint32_t *)out);

    out += 4;
    in  += 4;
  }
}

/** @} (end addtogroup CRYPTO) */
/** @} (end addtogroup emlib) */

#endif /* defined(CRYPTO_COUNT) && (CRYPTO_COUNT > 0) */
