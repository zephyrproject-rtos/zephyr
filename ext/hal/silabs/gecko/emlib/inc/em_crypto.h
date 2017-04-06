/***************************************************************************//**
 * @file em_crypto.h
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
 *    claim that you wrote the original software.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
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
#ifndef EM_CRYPTO_H
#define EM_CRYPTO_H

#include "em_device.h"

#if defined(CRYPTO_COUNT) && (CRYPTO_COUNT > 0)

#include "em_bus.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************//**
 * @addtogroup emlib
 * @{
 ******************************************************************************/

/***************************************************************************//**
 * @addtogroup CRYPTO
 *
 * @brief Cryptography accelerator peripheral API
 *
 * @details
 *   In order for cryptographic support, users are recommended to consider the
 *   crypto APIs of the mbedTLS library provided by Silicon Labs instead of the
 *   interface provided in em_crypto.h. The mbedTLS library provides a much
 *   richer crypto API, including hardware acceleration of several functions.
 *
 *   The main purpose of em_crypto.h is to implement a thin software interface
 *   for the CRYPTO hardware functions especially for the accelerated APIs of
 *   the mbedTLS library. Additionally em_crypto.h implement the AES API of the
 *   em_aes.h (supported by classic EFM32) for backwards compatibility. The
 *   following list summarizes the em_crypto.h inteface:
 *   @li AES (Advanced Encryption Standard) @ref crypto_aes
 *   @li SHA (Secure Hash Algorithm) @ref crypto_sha
 *   @li Big Integer multiplier @ref crypto_mul
 *   @li Functions for loading data and executing instruction sequences @ref crypto_exec
 *
 *   @n @section crypto_aes AES
 *   The AES APIs include support for AES-128 and AES-256 with block cipher
 *   modes:
 *   @li CBC - Cipher Block Chaining mode
 *   @li CFB - Cipher Feedback mode
 *   @li CTR - Counter mode
 *   @li ECB - Electronic Code Book mode
 *   @li OFB - Output Feedback mode
 *
 *   For the AES APIs Input/output data (plaintext, ciphertext, key etc) are
 *   treated as byte arrays, starting with most significant byte. Ie, 32 bytes
 *   of plaintext (B0...B31) is located in memory in the same order, with B0 at
 *   the lower address and B31 at the higher address.
 *
 *   Byte arrays must always be a multiple of AES block size, ie. a multiple
 *   of 16. Padding, if required, is done at the end of the byte array.
 *
 *   Byte arrays should be word (32 bit) aligned for performance
 *   considerations, since the array is accessed with 32 bit access type.
 *   The core MCUs supports unaligned accesses, but with a performance penalty.
 *
 *   It is possible to specify the same output buffer as input buffer as long
 *   as they point to the same address. In that case the provided input buffer
 *   is replaced with the encrypted/decrypted output. Notice that the buffers
 *   must be exactly overlapping. If partly overlapping, the behavior is
 *   undefined.
 *
 *   It is up to the user to use a cipher mode according to its requirements
 *   in order to not break security. Please refer to specific cipher mode
 *   theory for details.
 *
 *   References:
 *   @li Wikipedia - Cipher modes, http://en.wikipedia.org/wiki/Cipher_modes
 *
 *   @li Recommendation for Block Cipher Modes of Operation,
 *      NIST Special Publication 800-38A, 2001 Edition,
 *      http://csrc.nist.gov/publications/nistpubs/800-38a/sp800-38a.pdf
 *
 *   @li Recommendation for Block Cipher Modes of Operation,
 *      http://csrc.nist.gov/publications/fips/fips180-4/fips-180-4.pdf
 *
 *   @n @section crypto_sha SHA
 *   The SHA APIs include support for
 *   @li SHA-1 @ref CRYPTO_SHA_1
 *   @li SHA-256 @ref CRYPTO_SHA_256
 *
 *   The SHA-1 implementation is FIPS-180-1 compliant, ref:
 *   @li Wikipedia -  SHA-1, https://en.wikipedia.org/wiki/SHA-1
 *   @li SHA-1 spec - http://www.itl.nist.gov/fipspubs/fip180-1.htm
 *
 *   The SHA-256 implementation is FIPS-180-2 compliant, ref:
 *   @li Wikipedia -  SHA-2, https://en.wikipedia.org/wiki/SHA-2
 *   @li SHA-2 spec - http://csrc.nist.gov/publications/fips/fips180-2/fips180-2.pdf
 *
 *   @n @section crypto_mul CRYPTO_Mul
 *   @ref CRYPTO_Mul is a function for multiplying big integers that are
 *   bigger than the operand size of the MUL instruction which is 128 bits.
 *   CRYPTO_Mul multiplies all partial operands of the input operands using
 *   MUL to form a resulting number which may be twice the size of
 *   the operands.
 *
 *   CRPYTO_Mul is typically used by RSA implementations which perform a
 *   huge amount of multiplication and square operations in order to
 *   implement modular exponentiation.
 *   Some RSA implementations use a number representation including arrays
 *   of 32bit words of variable size. The user should compile with
 *   -D USE_VARIABLE_SIZED_DATA_LOADS in order to load these numbers
 *   directly into CRYPTO without converting the number representation.
 *
 *   @n @section crypto_exec Load and Execute Instruction Sequences
 *   The functions for loading data and executing instruction sequences can
 *   be used to implement complex algorithms like elliptic curve cryptography
 *   (ECC)) and authenticated encryption algorithms. There are two typical
 *   modes of operation:
 *   @li Multi sequence operation
 *   @li Single static instruction sequence operation
 *
 *   In multi sequence mode the software starts by loading input data, then
 *   an instruction sequence, execute, and finally read the result. This
 *   process is repeated until the full crypto operation is complete.
 *
 *   When using a single static instruction sequence, there is just one
 *   instruction sequence which is loaded initially. The sequence can be setup
 *   to run multiple times. The data can be loaded during the execution of the
 *   sequence by using DMA, BUFC and/or programmed I/O directly from the MCU
 *   core. For details on how to program the instruction sequences please refer
 *   to the reference manual of the particular Silicon Labs device.
 *
 *   In order to load input data to the CRYPTO module use any of the following
 *   functions:
 *   @li @ref CRYPTO_DataWrite  - Write 128 bits to a DATA register.
 *   @li @ref CRYPTO_DDataWrite - Write 256 bits to a DDATA register.
 *   @li @ref CRYPTO_QDataWrite - Write 512 bits to a QDATA register.
 *
 *   In order to read output data from the CRYPTO module use any of the
 *   following functions:
 *   @li @ref CRYPTO_DataRead  - Read 128 bits from a DATA register.
 *   @li @ref CRYPTO_DDataRead - Read 256 bits from a DDATA register.
 *   @li @ref CRYPTO_QDataRead - Read 512 bits from a QDATA register.
 *
 *   In order to load an instruction sequence to the CRYPTO module use
 *   @ref CRYPTO_InstructionSequenceLoad.
 *
 *   In order to execute the current instruction sequence in the CRYPTO module
 *   use @ref CRYPTO_InstructionSequenceExecute.
 *
 *   In order to check whether an instruction sequence has completed
 *   use @ref CRYPTO_InstructionSequenceDone.
 *
 *   In order to wait for an instruction sequence to complete
 *   use @ref CRYPTO_InstructionSequenceWait.
 *
 *   In order to optimally load (with regards to speed) and execute an
 *   instruction sequence use any of the CRYPTO_EXECUTE_X macros (where X is
 *   in the range 1-20) defined in @ref em_crypto.h. E.g. CRYPTO_EXECUTE_19.
 * @{
 ******************************************************************************/

 /*******************************************************************************
 ******************************   DEFINES    ***********************************
 ******************************************************************************/

/** @cond DO_NOT_INCLUDE_WITH_DOXYGEN */
/** Data sizes used by CRYPTO operations. */
#define CRYPTO_DATA_SIZE_IN_BITS           (128)
#define CRYPTO_DATA_SIZE_IN_BYTES          (CRYPTO_DATA_SIZE_IN_BITS/8)
#define CRYPTO_DATA_SIZE_IN_32BIT_WORDS    (CRYPTO_DATA_SIZE_IN_BYTES/sizeof(uint32_t))

#define CRYPTO_KEYBUF_SIZE_IN_BITS         (256)
#define CRYPTO_KEYBUF_SIZE_IN_BYTES        (CRYPTO_DDATA_SIZE_IN_BITS/8)
#define CRYPTO_KEYBUF_SIZE_IN_32BIT_WORDS  (CRYPTO_DDATA_SIZE_IN_BYTES/sizeof(uint32_t))

#define CRYPTO_DDATA_SIZE_IN_BITS          (256)
#define CRYPTO_DDATA_SIZE_IN_BYTES         (CRYPTO_DDATA_SIZE_IN_BITS/8)
#define CRYPTO_DDATA_SIZE_IN_32BIT_WORDS   (CRYPTO_DDATA_SIZE_IN_BYTES/sizeof(uint32_t))

#define CRYPTO_QDATA_SIZE_IN_BITS          (512)
#define CRYPTO_QDATA_SIZE_IN_BYTES         (CRYPTO_QDATA_SIZE_IN_BITS/8)
#define CRYPTO_QDATA_SIZE_IN_32BIT_WORDS   (CRYPTO_QDATA_SIZE_IN_BYTES/sizeof(uint32_t))

#define CRYPTO_DATA260_SIZE_IN_32BIT_WORDS (9)

/** SHA-1 digest sizes */
#define CRYPTO_SHA1_DIGEST_SIZE_IN_BITS    (160)
#define CRYPTO_SHA1_DIGEST_SIZE_IN_BYTES   (CRYPTO_SHA1_DIGEST_SIZE_IN_BITS/8)

/** SHA-256 digest sizes */
#define CRYPTO_SHA256_DIGEST_SIZE_IN_BITS  (256)
#define CRYPTO_SHA256_DIGEST_SIZE_IN_BYTES (CRYPTO_SHA256_DIGEST_SIZE_IN_BITS/8)

/**
 * Read and write all 260 bits of DDATA0 when in 260 bit mode.
 */
#define CRYPTO_DDATA0_260_BITS_READ(crypto, bigint260)  CRYPTO_DData0Read260(crypto, bigint260)
#define CRYPTO_DDATA0_260_BITS_WRITE(crypto, bigint260) CRYPTO_DData0Write260(crypto, bigint260)
/** @endcond */

/** @cond DO_NOT_INCLUDE_WITH_DOXYGEN */
/**
 * Instruction sequence load macros CRYPTO_SEQ_LOAD_X (where X is in the range
 * 1-20). E.g. @ref CRYPTO_SEQ_LOAD_20.
 * Use these macros in order for faster execution than the function API.
 */
#define CRYPTO_SEQ_LOAD_1(crypto, a1) { \
    crypto->SEQ0 =  a1 |  (CRYPTO_CMD_INSTR_END<<8);}
#define CRYPTO_SEQ_LOAD_2(crypto, a1, a2) { \
    crypto->SEQ0 =  a1 |  (a2<<8) |  (CRYPTO_CMD_INSTR_END<<16);}
#define CRYPTO_SEQ_LOAD_3(crypto, a1, a2, a3) { \
    crypto->SEQ0 =  a1 |  (a2<<8) |  (a3<<16) | (CRYPTO_CMD_INSTR_END<<24);}
#define CRYPTO_SEQ_LOAD_4(crypto, a1, a2, a3, a4) { \
    crypto->SEQ0 =  a1 |  (a2<<8) |  (a3<<16) |  (a4<<24);              \
    crypto->SEQ1 =  CRYPTO_CMD_INSTR_END;}
#define CRYPTO_SEQ_LOAD_5(crypto, a1, a2, a3, a4, a5) { \
    crypto->SEQ0 =  a1 |  (a2<<8) |  (a3<<16) |  (a4<<24);              \
    crypto->SEQ1 =  a5 |  (CRYPTO_CMD_INSTR_END<<8);}
#define CRYPTO_SEQ_LOAD_6(crypto, a1, a2, a3, a4, a5, a6) { \
    crypto->SEQ0 =  a1 |  (a2<<8) |  (a3<<16) |  (a4<<24);              \
    crypto->SEQ1 =  a5 |  (a6<<8) |  (CRYPTO_CMD_INSTR_END<<16);}
#define CRYPTO_SEQ_LOAD_7(crypto, a1, a2, a3, a4, a5, a6, a7) { \
    crypto->SEQ0 =  a1 |  (a2<<8) |  (a3<<16) |  (a4<<24);              \
    crypto->SEQ1 =  a5 |  (a6<<8) |  (a7<<16) |  (CRYPTO_CMD_INSTR_END<<24);}
#define CRYPTO_SEQ_LOAD_8(crypto, a1, a2, a3, a4, a5, a6, a7, a8) { \
    crypto->SEQ0 =  a1 |  (a2<<8) |  (a3<<16) |  (a4<<24);              \
    crypto->SEQ1 =  a5 |  (a6<<8) |  (a7<<16) |  (a8<<24);              \
    crypto->SEQ2 =  CRYPTO_CMD_INSTR_END;}
#define CRYPTO_SEQ_LOAD_9(crypto, a1, a2, a3, a4, a5, a6, a7, a8, a9) { \
    crypto->SEQ0 =  a1 |  (a2<<8) |  (a3<<16) |  (a4<<24);              \
    crypto->SEQ1 =  a5 |  (a6<<8) |  (a7<<16) |  (a8<<24);              \
    crypto->SEQ2 =  a9 | (CRYPTO_CMD_INSTR_END<<8);}
#define CRYPTO_SEQ_LOAD_10(crypto, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10) { \
    crypto->SEQ0 =  a1 |  (a2<<8) |  (a3<<16) |  (a4<<24);              \
    crypto->SEQ1 =  a5 |  (a6<<8) |  (a7<<16) |  (a8<<24);              \
    crypto->SEQ2 =  a9 | (a10<<8) | (CRYPTO_CMD_INSTR_END<<16);}
#define CRYPTO_SEQ_LOAD_11(crypto, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11) { \
    crypto->SEQ0 =  a1 |  (a2<<8) |  (a3<<16) |  (a4<<24);              \
    crypto->SEQ1 =  a5 |  (a6<<8) |  (a7<<16) |  (a8<<24);              \
    crypto->SEQ2 =  a9 | (a10<<8) | (a11<<16) | (CRYPTO_CMD_INSTR_END<<24);}
#define CRYPTO_SEQ_LOAD_12(crypto, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12) { \
    crypto->SEQ0 =  a1 |  (a2<<8) |  (a3<<16) |  (a4<<24);              \
    crypto->SEQ1 =  a5 |  (a6<<8) |  (a7<<16) |  (a8<<24);              \
    crypto->SEQ2 =  a9 | (a10<<8) | (a11<<16) | (a12<<24);              \
    crypto->SEQ3 = CRYPTO_CMD_INSTR_END;}
#define CRYPTO_SEQ_LOAD_13(crypto, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13) { \
    crypto->SEQ0 =  a1 |  (a2<<8) |  (a3<<16) |  (a4<<24);              \
    crypto->SEQ1 =  a5 |  (a6<<8) |  (a7<<16) |  (a8<<24);              \
    crypto->SEQ2 =  a9 | (a10<<8) | (a11<<16) | (a12<<24);              \
    crypto->SEQ3 = a13 | (CRYPTO_CMD_INSTR_END<<8);}
#define CRYPTO_SEQ_LOAD_14(crypto, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14) { \
    crypto->SEQ0 =  a1 |  (a2<<8) |  (a3<<16) |  (a4<<24);              \
    crypto->SEQ1 =  a5 |  (a6<<8) |  (a7<<16) |  (a8<<24);              \
    crypto->SEQ2 =  a9 | (a10<<8) | (a11<<16) | (a12<<24);              \
    crypto->SEQ3 = a13 | (a14<<8) | (CRYPTO_CMD_INSTR_END<<16);}
#define CRYPTO_SEQ_LOAD_15(crypto, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15) { \
    crypto->SEQ0 =  a1 |  (a2<<8) |  (a3<<16) |  (a4<<24);              \
    crypto->SEQ1 =  a5 |  (a6<<8) |  (a7<<16) |  (a8<<24);              \
    crypto->SEQ2 =  a9 | (a10<<8) | (a11<<16) | (a12<<24);              \
    crypto->SEQ3 = a13 | (a14<<8) | (a15<<16) | (CRYPTO_CMD_INSTR_END<<24);}
#define CRYPTO_SEQ_LOAD_16(crypto, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16) { \
    crypto->SEQ0 =  a1 |  (a2<<8) |  (a3<<16) |  (a4<<24);              \
    crypto->SEQ1 =  a5 |  (a6<<8) |  (a7<<16) |  (a8<<24);              \
    crypto->SEQ2 =  a9 | (a10<<8) | (a11<<16) | (a12<<24);              \
    crypto->SEQ3 = a13 | (a14<<8) | (a15<<16) | (a16<<24);              \
    crypto->SEQ4 = CRYPTO_CMD_INSTR_END;}
#define CRYPTO_SEQ_LOAD_17(crypto, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17) { \
    crypto->SEQ0 =  a1 |  (a2<<8) |  (a3<<16) |  (a4<<24);              \
    crypto->SEQ1 =  a5 |  (a6<<8) |  (a7<<16) |  (a8<<24);              \
    crypto->SEQ2 =  a9 | (a10<<8) | (a11<<16) | (a12<<24);              \
    crypto->SEQ3 = a13 | (a14<<8) | (a15<<16) | (a16<<24);              \
    crypto->SEQ4 = a17 | (CRYPTO_CMD_INSTR_END<<8);}
#define CRYPTO_SEQ_LOAD_18(crypto, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18) { \
    crypto->SEQ0 =  a1 |  (a2<<8) |  (a3<<16) |  (a4<<24);              \
    crypto->SEQ1 =  a5 |  (a6<<8) |  (a7<<16) |  (a8<<24);              \
    crypto->SEQ2 =  a9 | (a10<<8) | (a11<<16) | (a12<<24);              \
    crypto->SEQ3 = a13 | (a14<<8) | (a15<<16) | (a16<<24);              \
    crypto->SEQ4 = a17 | (a18<<8) | (CRYPTO_CMD_INSTR_END<<16);}
#define CRYPTO_SEQ_LOAD_19(crypto, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19) { \
    crypto->SEQ0 =  a1 |  (a2<<8) |  (a3<<16) |  (a4<<24);              \
    crypto->SEQ1 =  a5 |  (a6<<8) |  (a7<<16) |  (a8<<24);              \
    crypto->SEQ2 =  a9 | (a10<<8) | (a11<<16) | (a12<<24);              \
    crypto->SEQ3 = a13 | (a14<<8) | (a15<<16) | (a16<<24);              \
    crypto->SEQ4 = a17 | (a18<<8) | (a19<<16) | (CRYPTO_CMD_INSTR_END<<24);}
#define CRYPTO_SEQ_LOAD_20(crypto, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20) { \
    crypto->SEQ0 =  a1 |  (a2<<8) |  (a3<<16) |  (a4<<24);              \
    crypto->SEQ1 =  a5 |  (a6<<8) |  (a7<<16) |  (a8<<24);              \
    crypto->SEQ2 =  a9 | (a10<<8) | (a11<<16) | (a12<<24);              \
    crypto->SEQ3 = a13 | (a14<<8) | (a15<<16) | (a16<<24);              \
    crypto->SEQ4 = a17 | (a18<<8) | (a19<<16) | (a20<<24);}
/** @endcond */

/** @cond DO_NOT_INCLUDE_WITH_DOXYGEN */
/**
 * Instruction sequence execution macros CRYPTO_EXECUTE_X (where X is in the range
 * 1-20). E.g. @ref CRYPTO_EXECUTE_19.
 * Use these macros in order for faster execution than the function API.
 */
#define CRYPTO_EXECUTE_1(crypto, a1) {                                          \
    crypto->SEQ0 = a1 | (CRYPTO_CMD_INSTR_EXEC<<8);                    }
#define CRYPTO_EXECUTE_2(crypto, a1, a2) {                                      \
    crypto->SEQ0 = a1 | (a2<<8) | (CRYPTO_CMD_INSTR_EXEC<<16);         }
#define CRYPTO_EXECUTE_3(crypto, a1, a2, a3) {                                  \
    crypto->SEQ0 = a1 | (a2<<8) | (a3<<16) | (CRYPTO_CMD_INSTR_EXEC<<24); }
#define CRYPTO_EXECUTE_4(crypto, a1, a2, a3, a4) {                              \
    crypto->SEQ0 = a1 | (a2<<8) | (a3<<16) | (a4<<24);                  \
    crypto->SEQ1 = CRYPTO_CMD_INSTR_EXEC;                              }
#define CRYPTO_EXECUTE_5(crypto, a1, a2, a3, a4, a5) {                          \
    crypto->SEQ0 = a1 | (a2<<8) | (a3<<16) | (a4<<24);                  \
    crypto->SEQ1 = a5 | (CRYPTO_CMD_INSTR_EXEC<<8);                    }
#define CRYPTO_EXECUTE_6(crypto, a1, a2, a3, a4, a5, a6) {                      \
    crypto->SEQ0 = a1 | (a2<<8) | (a3<<16) | (a4<<24);                  \
    crypto->SEQ1 = a5 | (a6<<8) | (CRYPTO_CMD_INSTR_EXEC<<16);         }
#define CRYPTO_EXECUTE_7(crypto, a1, a2, a3, a4, a5, a6, a7) {                  \
    crypto->SEQ0 = a1 | (a2<<8) | (a3<<16) | (a4<<24);                  \
    crypto->SEQ1 = a5 | (a6<<8) | (a7<<16) | (CRYPTO_CMD_INSTR_EXEC<<24); }
#define CRYPTO_EXECUTE_8(crypto, a1, a2, a3, a4, a5, a6, a7, a8) {              \
    crypto->SEQ0 = a1 | (a2<<8) | (a3<<16) | (a4<<24);                  \
    crypto->SEQ1 = a5 | (a6<<8) | (a7<<16) | (a8<<24);                  \
    crypto->SEQ2 = CRYPTO_CMD_INSTR_EXEC;                              }
#define CRYPTO_EXECUTE_9(crypto, a1, a2, a3, a4, a5, a6, a7, a8, a9) {          \
    crypto->SEQ0 = a1 | (a2<<8) | (a3<<16) | (a4<<24);                  \
    crypto->SEQ1 = a5 | (a6<<8) | (a7<<16) | (a8<<24);                  \
    crypto->SEQ2 = a9 | (CRYPTO_CMD_INSTR_EXEC<<8);                    }
#define CRYPTO_EXECUTE_10(crypto, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10) {    \
    crypto->SEQ0 = a1 | (a2<<8) | (a3<<16) | (a4<<24);                  \
    crypto->SEQ1 = a5 | (a6<<8) | (a7<<16) | (a8<<24);                  \
    crypto->SEQ2 = a9 | (a10<<8) | (CRYPTO_CMD_INSTR_EXEC<<16);        }
#define CRYPTO_EXECUTE_11(crypto, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11) { \
    crypto->SEQ0 = a1 | (a2<<8) | (a3<<16) | (a4<<24);                  \
    crypto->SEQ1 = a5 | (a6<<8) | (a7<<16) | (a8<<24);                  \
    crypto->SEQ2 = a9 | (a10<<8) | (a11<<16) | (CRYPTO_CMD_INSTR_EXEC<<24); }
#define CRYPTO_EXECUTE_12(crypto, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12) { \
    crypto->SEQ0 = a1 |  (a2<<8) |  (a3<<16) | (a4<<24);                \
    crypto->SEQ1 = a5 |  (a6<<8) |  (a7<<16) | (a8<<24);                \
    crypto->SEQ2 = a9 | (a10<<8) | (a11<<16) | (a12<<24);               \
    crypto->SEQ3 = CRYPTO_CMD_INSTR_EXEC;                              }
#define CRYPTO_EXECUTE_13(crypto, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13) { \
    crypto->SEQ0 = a1  | (a2<<8)  | (a3<<16)  | (a4<<24);               \
    crypto->SEQ1 = a5  | (a6<<8)  | (a7<<16)  | (a8<<24);               \
    crypto->SEQ2 = a9  | (a10<<8) | (a11<<16) | (a12<<24);              \
    crypto->SEQ3 = a13 | (CRYPTO_CMD_INSTR_EXEC<<8);                   }
#define CRYPTO_EXECUTE_14(crypto, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14) { \
    crypto->SEQ0 = a1 | (a2<<8) | (a3<<16) | (a4<<24);                  \
    crypto->SEQ1 = a5 | (a6<<8) | (a7<<16) | (a8<<24);                  \
    crypto->SEQ2 = a9 | (a10<<8) | (a11<<16) | (a12<<24);               \
    crypto->SEQ3 = a13 | (a14<<8) | (CRYPTO_CMD_INSTR_EXEC<<16);       }
#define CRYPTO_EXECUTE_15(crypto, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15) { \
    crypto->SEQ0 =  a1 |  (a2<<8) |  (a3<<16) |  (a4<<24);              \
    crypto->SEQ1 =  a5 |  (a6<<8) |  (a7<<16) |  (a8<<24);              \
    crypto->SEQ2 =  a9 | (a10<<8) | (a11<<16) | (a12<<24);              \
    crypto->SEQ3 = a13 | (a14<<8) | (a15<<16) | (CRYPTO_CMD_INSTR_EXEC<<24); }
#define CRYPTO_EXECUTE_16(crypto, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16) { \
    crypto->SEQ0 =  a1 |  (a2<<8) |  (a3<<16) |  (a4<<24);              \
    crypto->SEQ1 =  a5 |  (a6<<8) |  (a7<<16) |  (a8<<24);              \
    crypto->SEQ2 =  a9 | (a10<<8) | (a11<<16) | (a12<<24);              \
    crypto->SEQ3 = a13 | (a14<<8) | (a15<<16) | (a16<<24);              \
    crypto->SEQ4 = CRYPTO_CMD_INSTR_EXEC;                              }
#define CRYPTO_EXECUTE_17(crypto, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17) { \
    crypto->SEQ0 =  a1 |  (a2<<8) |  (a3<<16) | (a4<<24);               \
    crypto->SEQ1 =  a5 |  (a6<<8) |  (a7<<16) | (a8<<24);               \
    crypto->SEQ2 =  a9 | (a10<<8) | (a11<<16) | (a12<<24);              \
    crypto->SEQ3 = a13 | (a14<<8) | (a15<<16) | (a16<<24);              \
    crypto->SEQ4 = a17 | (CRYPTO_CMD_INSTR_EXEC<<8);                   }
#define CRYPTO_EXECUTE_18(crypto, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18) { \
    crypto->SEQ0 =  a1 |  (a2<<8) |  (a3<<16) |  (a4<<24);              \
    crypto->SEQ1 =  a5 |  (a6<<8) |  (a7<<16) |  (a8<<24);              \
    crypto->SEQ2 =  a9 | (a10<<8) | (a11<<16) | (a12<<24);              \
    crypto->SEQ3 = a13 | (a14<<8) | (a15<<16) | (a16<<24);              \
    crypto->SEQ4 = a17 | (a18<<8) | (CRYPTO_CMD_INSTR_EXEC<<16);       }
#define CRYPTO_EXECUTE_19(crypto, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19) { \
    crypto->SEQ0 =  a1 |  (a2<<8) |  (a3<<16) |  (a4<<24);              \
    crypto->SEQ1 =  a5 |  (a6<<8) |  (a7<<16) |  (a8<<24);              \
    crypto->SEQ2 =  a9 | (a10<<8) | (a11<<16) | (a12<<24);              \
    crypto->SEQ3 = a13 | (a14<<8) | (a15<<16) | (a16<<24);              \
    crypto->SEQ4 = a17 | (a18<<8) | (a19<<16) | (CRYPTO_CMD_INSTR_EXEC<<24); }
#define CRYPTO_EXECUTE_20(crypto, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20) { \
    crypto->SEQ0 =  a1 |  (a2<<8) |  (a3<<16) |  (a4<<24);              \
    crypto->SEQ1 =  a5 |  (a6<<8) |  (a7<<16) |  (a8<<24);              \
    crypto->SEQ2 =  a9 | (a10<<8) | (a11<<16) | (a12<<24);              \
    crypto->SEQ3 = a13 | (a14<<8) | (a15<<16) | (a16<<24);              \
    crypto->SEQ4 = a17 | (a18<<8) | (a19<<16) | (a20<<24);              \
    CRYPTO_InstructionSequenceExecute();}
/** @endcond */

/*******************************************************************************
 ******************************   TYPEDEFS   ***********************************
 ******************************************************************************/

/**
 * CRYPTO data types used for data load functions. This data type is
 * capable of storing a 128 bits value as used in the crypto DATA
 * registers
 */
typedef uint32_t CRYPTO_Data_TypeDef[CRYPTO_DATA_SIZE_IN_32BIT_WORDS];

/**
 * CRYPTO data type used for data load functions. This data type
 * is capable of storing a 256 bits value as used in the crypto DDATA
 * registers
 */
typedef uint32_t CRYPTO_DData_TypeDef[CRYPTO_DDATA_SIZE_IN_32BIT_WORDS];

/** @cond DO_NOT_INCLUDE_WITH_DOXYGEN */
typedef uint32_t* CRYPTO_DDataPtr_TypeDef;
/** @endcond */

/**
 * CRYPTO data type used for data load functions. This data type is
 * capable of storing a 512 bits value as used in the crypto QDATA
 * registers
 */
typedef uint32_t CRYPTO_QData_TypeDef[CRYPTO_QDATA_SIZE_IN_32BIT_WORDS];

/**
 * CRYPTO data type used for data load functions. This data type is
 * capable of storing a 260 bits value as used by the @ref CRYPTO_DData0Write260
 * function.
 *
 * Note that this data type is multiple of 32 bit words, so the
 * actual storage used by this type is 32x9=288 bits.
 */
typedef uint32_t CRYPTO_Data260_TypeDef[CRYPTO_DATA260_SIZE_IN_32BIT_WORDS];

/**
 * CRYPTO data type used for data load functions. This data type is
 * capable of storing 256 bits as used in the crypto KEYBUF register.
 */
typedef uint32_t CRYPTO_KeyBuf_TypeDef[CRYPTO_KEYBUF_SIZE_IN_32BIT_WORDS];

/**
 * CRYPTO 128 bit Data register pointer type. The 128 bit registers are used to
 * load 128 bit values as input and output data for cryptographic and big
 * integer arithmetic functions of the CRYPTO module.
 */
typedef volatile uint32_t* CRYPTO_DataReg_TypeDef;

/**
 * CRYPTO 256 bit DData (Double Data) register pointer type. The 256 bit
 * registers are used to load 256 bit values as input and output data for
 * cryptographic and big integer arithmetic functions of the CRYPTO module.
 */
typedef volatile uint32_t* CRYPTO_DDataReg_TypeDef;

/**
 * CRYPTO 512 bit QData (Quad data) register pointer type. The 512 bit
 * registers are used to load 512 bit values as input and output data for
 * cryptographic and big integer arithmetic functions of the CRYPTO module.
 */
typedef volatile uint32_t* CRYPTO_QDataReg_TypeDef;

/** CRYPTO modulus identifiers. */
typedef enum
{
  cryptoModulusBin256        = CRYPTO_WAC_MODULUS_BIN256,       /**< Generic 256 bit modulus 2^256 */
  cryptoModulusBin128        = CRYPTO_WAC_MODULUS_BIN128,       /**< Generic 128 bit modulus 2^128 */
  cryptoModulusGcmBin128     = CRYPTO_WAC_MODULUS_GCMBIN128,    /**< GCM 128 bit modulus = 2^128 + 2^7 + 2^2 + 2 + 1 */
  cryptoModulusEccB233       = CRYPTO_WAC_MODULUS_ECCBIN233P,   /**< ECC B233 prime modulus = 2^233 + 2^74 + 1   */
  cryptoModulusEccB163       = CRYPTO_WAC_MODULUS_ECCBIN163P,   /**< ECC B163 prime modulus = 2^163 + 2^7 + 2^6 + 2^3 + 1 */
  cryptoModulusEccP256       = CRYPTO_WAC_MODULUS_ECCPRIME256P, /**< ECC P256 prime modulus = 2^256 - 2^224 + 2^192 + 2^96 - 1 */
  cryptoModulusEccP224       = CRYPTO_WAC_MODULUS_ECCPRIME224P, /**< ECC P224 prime modulus = 2^224 - 2^96 - 1 */
  cryptoModulusEccP192       = CRYPTO_WAC_MODULUS_ECCPRIME192P, /**< ECC P192 prime modulus = 2^192 - 2^64 - 1 */
  cryptoModulusEccB233Order  = CRYPTO_WAC_MODULUS_ECCBIN233N,   /**< ECC B233 order modulus   */
  cryptoModulusEccB233KOrder = CRYPTO_WAC_MODULUS_ECCBIN233KN,  /**< ECC B233K order modulus */
  cryptoModulusEccB163Order  = CRYPTO_WAC_MODULUS_ECCBIN163N,   /**< ECC B163 order modulus */
  cryptoModulusEccB163KOrder = CRYPTO_WAC_MODULUS_ECCBIN163KN,  /**< ECC B163K order modulus */
  cryptoModulusEccP256Order  = CRYPTO_WAC_MODULUS_ECCPRIME256N, /**< ECC P256 order modulus */
  cryptoModulusEccP224Order  = CRYPTO_WAC_MODULUS_ECCPRIME224N, /**< ECC P224 order modulus */
  cryptoModulusEccP192Order  = CRYPTO_WAC_MODULUS_ECCPRIME192N  /**< ECC P192 order modulus */
} CRYPTO_ModulusId_TypeDef;

/** CRYPTO multiplication widths for wide arithmetic operations. */
typedef enum
{
  cryptoMulOperand256Bits     = CRYPTO_WAC_MULWIDTH_MUL256, /**< 256 bits operands */
  cryptoMulOperand128Bits     = CRYPTO_WAC_MULWIDTH_MUL128, /**< 128 bits operands */
  cryptoMulOperandModulusBits = CRYPTO_WAC_MULWIDTH_MULMOD  /**< MUL operand width
                                                                 is specified by the
                                                                 modulus type.*/
} CRYPTO_MulOperandWidth_TypeDef;

/** CRYPTO result widths for MUL operations. */
typedef enum
{
  cryptoResult128Bits = CRYPTO_WAC_RESULTWIDTH_128BIT, /**< Multiplication result width is 128 bits*/
  cryptoResult256Bits = CRYPTO_WAC_RESULTWIDTH_256BIT, /**< Multiplication result width is 256 bits*/
  cryptoResult260Bits = CRYPTO_WAC_RESULTWIDTH_260BIT  /**< Multiplication result width is 260 bits*/
} CRYPTO_ResultWidth_TypeDef;

/** CRYPTO result widths for MUL operations. */
typedef enum
{
  cryptoInc1byte = CRYPTO_CTRL_INCWIDTH_INCWIDTH1, /**< inc width is 1 byte*/
  cryptoInc2byte = CRYPTO_CTRL_INCWIDTH_INCWIDTH2, /**< inc width is 2 byte*/
  cryptoInc3byte = CRYPTO_CTRL_INCWIDTH_INCWIDTH3, /**< inc width is 3 byte*/
  cryptoInc4byte = CRYPTO_CTRL_INCWIDTH_INCWIDTH4  /**< inc width is 4 byte*/
} CRYPTO_IncWidth_TypeDef;

/** CRYPTO key width. */
typedef enum
{
  cryptoKey128Bits = 8,     /**< Key width is 128 bits*/
  cryptoKey256Bits = 16,    /**< Key width is 256 bits*/
} CRYPTO_KeyWidth_TypeDef;

/**
 * The max number of crypto instructions in an instruction sequence
 */
#define CRYPTO_MAX_SEQUENCE_INSTRUCTIONS (20)

/**
 * Instruction sequence type.
 * The user should fill in the desired operations from step1, then step2 etc.
 * The CRYPTO_CMD_INSTR_END marks the end of the sequence.
 * Bit fields are used to format the memory layout of the struct equal to the
 * sequence registers in the CRYPTO module.
 */
typedef uint8_t CRYPTO_InstructionSequence_TypeDef[CRYPTO_MAX_SEQUENCE_INSTRUCTIONS];

/** Default instruction sequence consisting of all ENDs. The user can
    initialize the instruction sequence with this default value set, and fill
    in the desired operations from step 1. The first END instruction marks
    the end of the sequence. */
#define CRYPTO_INSTRUCTIONSEQUENSE_DEFAULT                             \
  {CRYPTO_CMD_INSTR_END, CRYPTO_CMD_INSTR_END, CRYPTO_CMD_INSTR_END, \
   CRYPTO_CMD_INSTR_END, CRYPTO_CMD_INSTR_END, CRYPTO_CMD_INSTR_END, \
   CRYPTO_CMD_INSTR_END, CRYPTO_CMD_INSTR_END, CRYPTO_CMD_INSTR_END, \
   CRYPTO_CMD_INSTR_END, CRYPTO_CMD_INSTR_END, CRYPTO_CMD_INSTR_END, \
   CRYPTO_CMD_INSTR_END, CRYPTO_CMD_INSTR_END, CRYPTO_CMD_INSTR_END, \
   CRYPTO_CMD_INSTR_END, CRYPTO_CMD_INSTR_END, CRYPTO_CMD_INSTR_END, \
   CRYPTO_CMD_INSTR_END, CRYPTO_CMD_INSTR_END}

/** SHA-1 Digest type. */
typedef uint8_t CRYPTO_SHA1_Digest_TypeDef[CRYPTO_SHA1_DIGEST_SIZE_IN_BYTES];

/** SHA-256 Digest type. */
typedef uint8_t CRYPTO_SHA256_Digest_TypeDef[CRYPTO_SHA256_DIGEST_SIZE_IN_BYTES];

/**
 * @brief
 *   AES counter modification function pointer.
 *
 * @note
 *   This is defined in order for backwards compatibility with EFM32 em_aes.h.
 *   The CRYPTO implementation of Counter mode does not support counter update
 *   callbacks.
 *
 * @param[in]  ctr   Counter value to be modified.
 */
typedef void (*CRYPTO_AES_CtrFuncPtr_TypeDef)(uint8_t * ctr);

/*******************************************************************************
 *****************************   PROTOTYPES   **********************************
 ******************************************************************************/

/***************************************************************************//**
 * @brief
 *   Set the modulus type used for wide arithmetic operations.
 *
 * @details
 *   This function sets the modulus type to be used by the Modulus instructions
 *   of the CRYPTO module.
 *
 * @param[in]  crypto
 *   Pointer to CRYPTO peripheral register block.
 *
 * @param[in]  modType
 *   Modulus type.
 ******************************************************************************/
void CRYPTO_ModulusSet(CRYPTO_TypeDef *          crypto,
                       CRYPTO_ModulusId_TypeDef  modType);

/***************************************************************************//**
 * @brief
 *   Set the number of bits in the operands of the MUL instruction.
 *
 * @details
 *   This function sets the number of bits to be used in the operands of
 *   the MUL instruction.
 *
 * @param[in]  crypto
 *   Pointer to CRYPTO peripheral register block.
 *
 * @param[in]  mulOperandWidth
 *   Multiplication width in bits.
 ******************************************************************************/
__STATIC_INLINE
void CRYPTO_MulOperandWidthSet(CRYPTO_TypeDef *crypto,
                               CRYPTO_MulOperandWidth_TypeDef mulOperandWidth)
{
  uint32_t temp = crypto->WAC & (~_CRYPTO_WAC_MULWIDTH_MASK);
  crypto->WAC = temp | mulOperandWidth;
}

/***************************************************************************//**
 * @brief
 *   Set the width of the results of the non-modulus instructions.
 *
 * @details
 *   This function sets the result width of the non-modulus instructions.
 *
 * @param[in]  crypto
 *   Pointer to CRYPTO peripheral register block.
 *
 * @param[in]  resultWidth
 *   Result width of non-modulus instructions.
 ******************************************************************************/
__STATIC_INLINE
void CRYPTO_ResultWidthSet(CRYPTO_TypeDef *crypto,
                           CRYPTO_ResultWidth_TypeDef resultWidth)
{
  uint32_t temp = crypto->WAC & (~_CRYPTO_WAC_RESULTWIDTH_MASK);
  crypto->WAC = temp | resultWidth;
}

/***************************************************************************//**
 * @brief
 *   Set the width of the DATA1 increment instruction DATA1INC.
 *
 * @details
 *   This function sets the width of the DATA1 increment instruction
 *   @ref CRYPTO_CMD_INSTR_DATA1INC.
 *
 * @param[in]  crypto
 *   Pointer to CRYPTO peripheral register block.
 *
 * @param[in]  incWidth
 *   incrementation width.
 ******************************************************************************/
__STATIC_INLINE void CRYPTO_IncWidthSet(CRYPTO_TypeDef *crypto,
                                        CRYPTO_IncWidth_TypeDef incWidth)
{
  uint32_t temp = crypto->CTRL & (~_CRYPTO_CTRL_INCWIDTH_MASK);
  crypto->CTRL = temp | incWidth;
}

/***************************************************************************//**
 * @brief
 *   Write a 128 bit value into a crypto register.
 *
 * @note
 *   This function provide a low-level api for writing to the multi-word
 *   registers in the crypto peripheral. Applications should prefer to use
 *   @ref CRYPTO_DataWrite, @ref CRYPTO_DDataWrite or @ref CRYPTO_QDataWrite
 *   for writing to the DATA, DDATA and QDATA registers.
 *
 * @param[in]  reg
 *   Pointer to the crypto register.
 *
 * @param[in]  val
 *   This is a pointer to 4 32 bit integers that contains the 128 bit value
 *   which will be written to the crypto register.
 ******************************************************************************/
__STATIC_INLINE void CRYPTO_BurstToCrypto(volatile uint32_t * reg,
                                          const uint32_t * val)
{
  /* Load data from memory into local registers. */
  register uint32_t v0 = val[0];
  register uint32_t v1 = val[1];
  register uint32_t v2 = val[2];
  register uint32_t v3 = val[3];
  /* Store data to CRYPTO */
  *reg = v0;
  *reg = v1;
  *reg = v2;
  *reg = v3;
}

/***************************************************************************//**
 * @brief
 *   Read a 128 bit value from a crypto register.
 *
 * @note
 *   This function provide a low-level api for reading one of the multi-word
 *   registers in the crypto peripheral. Applications should prefer to use
 *   @ref CRYPTO_DataRead, @ref CRYPTO_DDataRead or @ref CRYPTO_QDataRead
 *   for reading the value of the DATA, DDATA and QDATA registers.
 *
 * @param[in]  reg
 *   Pointer to the crypto register.
 *
 * @param[out]  val
 *   This is a pointer to an array that is capable of holding 4 32 bit integers
 *   that will be filled with the 128 bit value from the crypto register.
 ******************************************************************************/
__STATIC_INLINE void CRYPTO_BurstFromCrypto(volatile uint32_t * reg, uint32_t * val)
{
  /* Load data from CRYPTO into local registers. */
  register uint32_t v0 = *reg;
  register uint32_t v1 = *reg;
  register uint32_t v2 = *reg;
  register uint32_t v3 = *reg;
  /* Store data to memory */
  val[0] = v0;
  val[1] = v1;
  val[2] = v2;
  val[3] = v3;
}

/***************************************************************************//**
 * @brief
 *   Write 128 bits of data to a DATAX register in the CRYPTO module.
 *
 * @details
 *   Write 128 bits of data to a DATAX register in the crypto module. The data
 *   value is typically input to a big integer operation (see crypto
 *   instructions).
 *
 * @param[in]  dataReg    The 128 bit DATA register.
 * @param[in]  val        Value of the data to write to the DATA register.
 ******************************************************************************/
__STATIC_INLINE void CRYPTO_DataWrite(CRYPTO_DataReg_TypeDef dataReg,
                                      const CRYPTO_Data_TypeDef val)
{
  CRYPTO_BurstToCrypto((volatile uint32_t *)dataReg, val);
}

/***************************************************************************//**
 * @brief
 *   Read 128 bits of data from a DATAX register in the CRYPTO module.
 *
 * @details
 *   Read 128 bits of data from a DATAX register in the crypto module. The data
 *   value is typically output from a big integer operation (see crypto
 *   instructions)
 *
 * @param[in]  dataReg   The 128 bit DATA register.
 * @param[out] val       Location where to store the value in memory.
 ******************************************************************************/
__STATIC_INLINE void CRYPTO_DataRead(CRYPTO_DataReg_TypeDef  dataReg,
                                     CRYPTO_Data_TypeDef     val)
{
  CRYPTO_BurstFromCrypto((volatile uint32_t *)dataReg, val);
}

/***************************************************************************//**
 * @brief
 *   Write 256 bits of data to a DDATAX register in the CRYPTO module.
 *
 * @details
 *   Write 256 bits of data into a DDATAX (Double Data) register in the crypto
 *   module. The data value is typically input to a big integer operation (see
 *   crypto instructions).
 *
 * @param[in]  ddataReg   The 256 bit DDATA register.
 * @param[in]  val        Value of the data to write to the DDATA register.
 ******************************************************************************/
__STATIC_INLINE void CRYPTO_DDataWrite(CRYPTO_DDataReg_TypeDef ddataReg,
                                       const CRYPTO_DData_TypeDef val)
{
  CRYPTO_BurstToCrypto((volatile uint32_t *)ddataReg, &val[0]);
  CRYPTO_BurstToCrypto((volatile uint32_t *)ddataReg, &val[4]);
}

/***************************************************************************//**
 * @brief
 *   Read 256 bits of data from a DDATAX register in the CRYPTO module.
 *
 * @details
 *   Read 256 bits of data from a DDATAX (Double Data) register in the crypto
 *   module. The data value is typically output from a big integer operation
 *   (see crypto instructions).
 *
 * @param[in]  ddataReg   The 256 bit DDATA register.
 * @param[out] val        Location where to store the value in memory.
 ******************************************************************************/
__STATIC_INLINE void CRYPTO_DDataRead(CRYPTO_DDataReg_TypeDef  ddataReg,
                                      CRYPTO_DData_TypeDef     val)
{
  CRYPTO_BurstFromCrypto((volatile uint32_t *)ddataReg, &val[0]);
  CRYPTO_BurstFromCrypto((volatile uint32_t *)ddataReg, &val[4]);
}

/***************************************************************************//**
 * @brief
 *   Write 512 bits of data to a QDATAX register in the CRYPTO module.
 *
 * @details
 *   Write 512 bits of data into a QDATAX (Quad Data) register in the crypto module
 *   The data value is typically input to a big integer operation (see crypto
 *   instructions).
 *
 * @param[in]  qdataReg   The 512 bits QDATA register.
 * @param[in]  val        Value of the data to write to the QDATA register.
 ******************************************************************************/
__STATIC_INLINE void CRYPTO_QDataWrite(CRYPTO_QDataReg_TypeDef  qdataReg,
                                       CRYPTO_QData_TypeDef     val)
{
  CRYPTO_BurstToCrypto((volatile uint32_t *)qdataReg, &val[0]);
  CRYPTO_BurstToCrypto((volatile uint32_t *)qdataReg, &val[4]);
  CRYPTO_BurstToCrypto((volatile uint32_t *)qdataReg, &val[8]);
  CRYPTO_BurstToCrypto((volatile uint32_t *)qdataReg, &val[12]);
}

/***************************************************************************//**
 * @brief
 *   Read 512 bits of data from a QDATAX register in the CRYPTO module.
 *
 * @details
 * Read 512 bits of data from a QDATAX register in the crypto module. The data
 * value is typically input to a big integer operation (see crypto
 * instructions).
 *
 * @param[in]  qdataReg   The 512 bits QDATA register.
 * @param[in]  val        Value of the data to write to the QDATA register.
 ******************************************************************************/
__STATIC_INLINE void CRYPTO_QDataRead(CRYPTO_QDataReg_TypeDef qdataReg,
                                      CRYPTO_QData_TypeDef    val)
{
  CRYPTO_BurstFromCrypto((volatile uint32_t *)qdataReg, &val[0]);
  CRYPTO_BurstFromCrypto((volatile uint32_t *)qdataReg, &val[4]);
  CRYPTO_BurstFromCrypto((volatile uint32_t *)qdataReg, &val[8]);
  CRYPTO_BurstFromCrypto((volatile uint32_t *)qdataReg, &val[12]);
}

/***************************************************************************//**
 * @brief
 *   Set the key value to be used by the CRYPTO module.
 *
 * @details
 *   Write 128 or 256 bit key to the KEYBUF register in the crypto module.
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
__STATIC_INLINE void CRYPTO_KeyBufWrite(CRYPTO_TypeDef          *crypto,
                                        CRYPTO_KeyBuf_TypeDef    val,
                                        CRYPTO_KeyWidth_TypeDef  keyWidth)
{
  if (keyWidth == cryptoKey256Bits)
  {
    /* Set AES-256 mode */
    BUS_RegBitWrite(&crypto->CTRL, _CRYPTO_CTRL_AES_SHIFT, _CRYPTO_CTRL_AES_AES256);
    /* Load key in KEYBUF register (= DDATA4) */
    CRYPTO_DDataWrite(&crypto->DDATA4, (uint32_t *)val);
  }
  else
  {
    /* Set AES-128 mode */
    BUS_RegBitWrite(&crypto->CTRL, _CRYPTO_CTRL_AES_SHIFT, _CRYPTO_CTRL_AES_AES128);
    CRYPTO_BurstToCrypto(&crypto->KEYBUF, &val[0]);
  }
}

void CRYPTO_KeyRead(CRYPTO_TypeDef *crypto,
                    CRYPTO_KeyBuf_TypeDef   val,
                    CRYPTO_KeyWidth_TypeDef keyWidth);

/***************************************************************************//**
 * @brief
 *   Quick write 128 bit key to the CRYPTO module.
 *
 * @details
 *   Quick write 128 bit key to the KEYBUF register in the CRYPTO module.
 *
 * @param[in]  crypto
 *   Pointer to CRYPTO peripheral register block.
 *
 * @param[in]  val
 *   Value of the data to write to the KEYBUF register.
 ******************************************************************************/
__STATIC_INLINE void CRYPTO_KeyBuf128Write(CRYPTO_TypeDef *crypto,
                                           const uint32_t * val)
{
  CRYPTO_BurstToCrypto(&crypto->KEYBUF, val);
}

/***************************************************************************//**
 * @brief
 *   Quick read access of the Carry bit from arithmetic operations.
 *
 * @details
 *   This function reads the carry bit of the CRYPTO ALU.
 *
 * @param[in]  crypto
 *   Pointer to CRYPTO peripheral register block.
 *
 * @return
 *   Returns 'true' if carry is 1, and 'false' if carry is 0.
 ******************************************************************************/
__STATIC_INLINE bool CRYPTO_CarryIsSet(CRYPTO_TypeDef *crypto)
{
  return (crypto->DSTATUS & _CRYPTO_DSTATUS_CARRY_MASK)
    >> _CRYPTO_DSTATUS_CARRY_SHIFT;
}

/***************************************************************************//**
 * @brief
 *   Quick read access of the 4 LSbits of the DDATA0 register.
 *
 * @details
 *   This function quickly retrieves the 4 least significant bits of the
 *   DDATA0 register via the DDATA0LSBS bit field in the DSTATUS register.
 *
 * @param[in]  crypto
 *   Pointer to CRYPTO peripheral register block.
 *
 * @return
 *   Returns the 4 LSbits of DDATA0.
 ******************************************************************************/
__STATIC_INLINE uint8_t CRYPTO_DData0_4LSBitsRead(CRYPTO_TypeDef *crypto)
{
  return (crypto->DSTATUS & _CRYPTO_DSTATUS_DDATA0LSBS_MASK)
    >> _CRYPTO_DSTATUS_DDATA0LSBS_SHIFT;
}

/***************************************************************************//**
 * @brief
 *   Read 260 bits from the DDATA0 register.
 *
 * @details
 *   This functions reads 260 bits from the DDATA0 register in the CRYPTO
 *   module. The data value is typically output from a big integer operation
 *   (see crypto instructions) when the result width is set to 260 bits by
 *   calling @ref CRYPTO_ResultWidthSet(cryptoResult260Bits);
 *
 * @param[in]  crypto
 *   Pointer to CRYPTO peripheral register block.
 *
 * @param[out] val
 *   Location where to store the value in memory.
 ******************************************************************************/
__STATIC_INLINE void CRYPTO_DData0Read260(CRYPTO_TypeDef *crypto,
                                          CRYPTO_Data260_TypeDef val)
{
  CRYPTO_DDataRead(&crypto->DDATA0, val);
  val[8] = (crypto->DSTATUS & _CRYPTO_DSTATUS_DDATA0MSBS_MASK)
        >> _CRYPTO_DSTATUS_DDATA0MSBS_SHIFT;
}

/***************************************************************************//**
 * @brief
 *   Write 260 bits to the DDATA0 register.
 *
 * @details
 *   This functions writes 260 bits to the DDATA0 register in the CRYPTO
 *   module. The data value is typically input to a big integer operation
 *   (see crypto instructions) when the result width is set to 260 bits by
 *   calling @ref CRYPTO_ResultWidthSet(cryptoResult260Bits);
 *
 * @param[in]  crypto
 *   Pointer to CRYPTO peripheral register block.
 *
 * @param[out] val
 *   Location where of the value in memory.
 ******************************************************************************/
__STATIC_INLINE void CRYPTO_DData0Write260(CRYPTO_TypeDef *crypto,
                                           const CRYPTO_Data260_TypeDef val)
{
  CRYPTO_DDataWrite(&crypto->DDATA0, val);
  crypto->DDATA0BYTE32 = val[8] & _CRYPTO_DDATA0BYTE32_DDATA0BYTE32_MASK;
}

/***************************************************************************//**
 * @brief
 *   Quick read the MSbit of the DDATA1 register.
 *
 * @details
 *   This function reads the most significant bit (bit 255) of the DDATA1
 *   register via the DDATA1MSB bit field in the DSTATUS register. This can
 *   be used to quickly check the signedness of a big integer resident in the
 *   CRYPTO module.
 *
 * @param[in]  crypto
 *   Pointer to CRYPTO peripheral register block.
 *
 * @return
 *   Returns 'true' if MSbit is 1, and 'false' if MSbit is 0.
 ******************************************************************************/
__STATIC_INLINE bool CRYPTO_DData1_MSBitRead(CRYPTO_TypeDef *crypto)
{
  return (crypto->DSTATUS & _CRYPTO_DSTATUS_DDATA1MSB_MASK)
    >> _CRYPTO_DSTATUS_DDATA1MSB_SHIFT;
}

/***************************************************************************//**
 * @brief
 *   Load a sequence of instructions to be executed on the current values in
 *   the data registers.
 *
 * @details
 *   This function loads a sequence of instructions to the crypto module. The
 *   instructions will be executed when the CRYPTO_InstructionSequenceExecute
 *   function is called. The first END marks the end of the sequence.
 *
 * @param[in]  crypto
 *   Pointer to CRYPTO peripheral register block.
 *
 * @param[in]  instructionSequence
 *   Instruction sequence to load.
 ******************************************************************************/
__STATIC_INLINE
void CRYPTO_InstructionSequenceLoad(CRYPTO_TypeDef *crypto,
                                    const CRYPTO_InstructionSequence_TypeDef instructionSequence)
{
  const uint32_t * pas = (const uint32_t *) instructionSequence;

  crypto->SEQ0 = pas[0];
  crypto->SEQ1 = pas[1];
  crypto->SEQ2 = pas[2];
  crypto->SEQ3 = pas[3];
  crypto->SEQ4 = pas[4];
}

/***************************************************************************//**
 * @brief
 *   Execute the current programmed instruction sequence.
 *
 * @details
 *   This function starts the execution of the current instruction sequence
 *   in the CRYPTO module.
 *
 * @param[in]  crypto
 *   Pointer to CRYPTO peripheral register block.
 ******************************************************************************/
__STATIC_INLINE void CRYPTO_InstructionSequenceExecute(CRYPTO_TypeDef *crypto)
{
  /* Start the command sequence. */
  crypto->CMD = CRYPTO_CMD_SEQSTART;
}

/***************************************************************************//**
 * @brief
 *   Check whether the execution of an instruction sequence has completed.
 *
 * @details
 *   This function checks whether an instruction sequence has completed.
 *
 * @param[in]  crypto
 *   Pointer to CRYPTO peripheral register block.
 *
 * @return
 *   Returns 'true' if the instruction sequence is done, and 'false' if not.
 ******************************************************************************/
__STATIC_INLINE bool CRYPTO_InstructionSequenceDone(CRYPTO_TypeDef *crypto)
{
  /* Return true if operation has completed. */
  return !(crypto->STATUS
           & (CRYPTO_STATUS_INSTRRUNNING | CRYPTO_STATUS_SEQRUNNING));
}

/***************************************************************************//**
 * @brief
 *   Wait for completion of the current sequence of instructions.
 *
 * @details
 *   This function "busy"-waits until the execution of the ongoing instruction
 *   sequence has completed.
 *
 * @param[in]  crypto
 *   Pointer to CRYPTO peripheral register block.
 ******************************************************************************/
__STATIC_INLINE void CRYPTO_InstructionSequenceWait(CRYPTO_TypeDef *crypto)
{
  while (!CRYPTO_InstructionSequenceDone(crypto))
    ;
}

/***************************************************************************//**
 * @brief
 *   Wait for completion of the current command.
 *
 * @details
 *   This function "busy"-waits until the execution of the ongoing instruction
 *   has completed.
 *
 * @param[in]  crypto
 *   Pointer to CRYPTO peripheral register block.
 ******************************************************************************/
__STATIC_INLINE void CRYPTO_InstructionWait(CRYPTO_TypeDef *crypto)
{
  /* Wait for completion */
  while (!(crypto->IF & CRYPTO_IF_INSTRDONE))
    ;
  crypto->IFC = CRYPTO_IF_INSTRDONE;
}

void CRYPTO_SHA_1(CRYPTO_TypeDef                *crypto,
                  const uint8_t                 *msg,
                  uint64_t                       msgLen,
                  CRYPTO_SHA1_Digest_TypeDef     digest);

void CRYPTO_SHA_256(CRYPTO_TypeDef              *crypto,
                    const uint8_t               *msg,
                    uint64_t                     msgLen,
                    CRYPTO_SHA256_Digest_TypeDef digest);

void CRYPTO_Mul(CRYPTO_TypeDef *crypto,
                uint32_t * A, int aSize,
                uint32_t * B, int bSize,
                uint32_t * R, int rSize);

void CRYPTO_AES_CBC128(CRYPTO_TypeDef *crypto,
                       uint8_t * out,
                       const uint8_t * in,
                       unsigned int len,
                       const uint8_t * key,
                       const uint8_t * iv,
                       bool encrypt);

void CRYPTO_AES_CBC256(CRYPTO_TypeDef *crypto,
                       uint8_t * out,
                       const uint8_t * in,
                       unsigned int len,
                       const uint8_t * key,
                       const uint8_t * iv,
                       bool encrypt);

void CRYPTO_AES_CFB128(CRYPTO_TypeDef *crypto,
                       uint8_t * out,
                       const uint8_t * in,
                       unsigned int len,
                       const uint8_t * key,
                       const uint8_t * iv,
                       bool encrypt);

void CRYPTO_AES_CFB256(CRYPTO_TypeDef *crypto,
                       uint8_t * out,
                       const uint8_t * in,
                       unsigned int len,
                       const uint8_t * key,
                       const uint8_t * iv,
                       bool encrypt);

void CRYPTO_AES_CTR128(CRYPTO_TypeDef *crypto,
                       uint8_t * out,
                       const uint8_t * in,
                       unsigned int len,
                       const uint8_t * key,
                       uint8_t * ctr,
                       CRYPTO_AES_CtrFuncPtr_TypeDef ctrFunc);

void CRYPTO_AES_CTR256(CRYPTO_TypeDef *crypto,
                       uint8_t * out,
                       const uint8_t * in,
                       unsigned int len,
                       const uint8_t * key,
                       uint8_t * ctr,
                       CRYPTO_AES_CtrFuncPtr_TypeDef ctrFunc);

void CRYPTO_AES_CTRUpdate32Bit(uint8_t * ctr);
void CRYPTO_AES_DecryptKey128(CRYPTO_TypeDef *crypto, uint8_t * out, const uint8_t * in);
void CRYPTO_AES_DecryptKey256(CRYPTO_TypeDef *crypto, uint8_t * out, const uint8_t * in);

void CRYPTO_AES_ECB128(CRYPTO_TypeDef *crypto,
                       uint8_t * out,
                       const uint8_t * in,
                       unsigned int len,
                       const uint8_t * key,
                       bool encrypt);

void CRYPTO_AES_ECB256(CRYPTO_TypeDef *crypto,
                       uint8_t * out,
                       const uint8_t * in,
                       unsigned int len,
                       const uint8_t * key,
                       bool encrypt);

void CRYPTO_AES_OFB128(CRYPTO_TypeDef *crypto,
                       uint8_t * out,
                       const uint8_t * in,
                       unsigned int len,
                       const uint8_t * key,
                       const uint8_t * iv);

void CRYPTO_AES_OFB256(CRYPTO_TypeDef *crypto,
                       uint8_t * out,
                       const uint8_t * in,
                       unsigned int len,
                       const uint8_t * key,
                       const uint8_t * iv);

/***************************************************************************//**
 * @brief
 *   Clear one or more pending CRYPTO interrupts.
 *
 * @param[in] crypto
 *   Pointer to CRYPTO peripheral register block.
 *
 * @param[in] flags
 *   Pending CRYPTO interrupt source to clear. Use a bitwise logic OR combination of
 *   valid interrupt flags for the CRYPTO module (CRYPTO_IF_nnn).
 ******************************************************************************/
__STATIC_INLINE void CRYPTO_IntClear(CRYPTO_TypeDef *crypto, uint32_t flags)
{
  crypto->IFC = flags;
}

/***************************************************************************//**
 * @brief
 *   Disable one or more CRYPTO interrupts.
 *
 * @param[in] crypto
 *   Pointer to CRYPTO peripheral register block.
 *
 * @param[in] flags
 *   CRYPTO interrupt sources to disable. Use a bitwise logic OR combination of
 *   valid interrupt flags for the CRYPTO module (CRYPTO_IF_nnn).
 ******************************************************************************/
__STATIC_INLINE void CRYPTO_IntDisable(CRYPTO_TypeDef *crypto, uint32_t flags)
{
  crypto->IEN &= ~(flags);
}

/***************************************************************************//**
 * @brief
 *   Enable one or more CRYPTO interrupts.
 *
 * @note
 *   Depending on the use, a pending interrupt may already be set prior to
 *   enabling the interrupt. Consider using CRYPTO_IntClear() prior to enabling
 *   if such a pending interrupt should be ignored.
 *
 * @param[in] crypto
 *   Pointer to CRYPTO peripheral register block.
 *
 * @param[in] flags
 *   CRYPTO interrupt sources to enable. Use a bitwise logic OR combination of
 *   valid interrupt flags for the CRYPTO module (CRYPTO_IF_nnn).
 ******************************************************************************/
__STATIC_INLINE void CRYPTO_IntEnable(CRYPTO_TypeDef *crypto, uint32_t flags)
{
  crypto->IEN |= flags;
}

/***************************************************************************//**
 * @brief
 *   Get pending CRYPTO interrupt flags.
 *
 * @note
 *   The event bits are not cleared by the use of this function.
 *
 * @param[in] crypto
 *   Pointer to CRYPTO peripheral register block.
 *
 * @return
 *   CRYPTO interrupt sources pending. A bitwise logic OR combination of valid
 *   interrupt flags for the CRYPTO module (CRYPTO_IF_nnn).
 ******************************************************************************/
__STATIC_INLINE uint32_t CRYPTO_IntGet(CRYPTO_TypeDef *crypto)
{
  return crypto->IF;
}

/***************************************************************************//**
 * @brief
 *   Get enabled and pending CRYPTO interrupt flags.
 *   Useful for handling more interrupt sources in the same interrupt handler.
 *
 * @note
 *   Interrupt flags are not cleared by the use of this function.
 *
 * @param[in] crypto
 *   Pointer to CRYPTO peripheral register block.
 *
 * @return
 *   Pending and enabled CRYPTO interrupt sources
 *   The return value is the bitwise AND of
 *   - the enabled interrupt sources in CRYPTO_IEN and
 *   - the pending interrupt flags CRYPTO_IF
 ******************************************************************************/
__STATIC_INLINE uint32_t CRYPTO_IntGetEnabled(CRYPTO_TypeDef *crypto)
{
  uint32_t tmp;

  /* Store IEN in temporary variable in order to define explicit order
   * of volatile accesses. */
  tmp = crypto->IEN;

  /* Bitwise AND of pending and enabled interrupts */
  return crypto->IF & tmp;
}

/***************************************************************************//**
 * @brief
 *   Set one or more pending CRYPTO interrupts from SW.
 *
 * @param[in] crypto
 *   Pointer to CRYPTO peripheral register block.
 *
 * @param[in] flags
 *   CRYPTO interrupt sources to set to pending. Use a bitwise logic OR combination
 *   of valid interrupt flags for the CRYPTO module (CRYPTO_IF_nnn).
 ******************************************************************************/
__STATIC_INLINE void CRYPTO_IntSet(CRYPTO_TypeDef *crypto, uint32_t flags)
{
  crypto->IFS = flags;
}

/*******************************************************************************
 *****    Static inline wrappers for CRYPTO AES functions in order to      *****
 *****    preserve backwards compatibility with AES module API functions.  *****
 ******************************************************************************/

/***************************************************************************//**
 * @brief
 *   AES Cipher-block chaining (CBC) cipher mode encryption/decryption,
 *   128 bit key.
 *
 * @deprecated
 *   This function is present to preserve backwards compatibility. Use
 *   @ref CRYPTO_AES_CBC128 instead.
 ******************************************************************************/
__STATIC_INLINE void AES_CBC128(uint8_t * out,
                       const uint8_t * in,
                       unsigned int len,
                       const uint8_t * key,
                       const uint8_t * iv,
                       bool encrypt)
{
  CRYPTO_AES_CBC128(CRYPTO, out, in, len, key, iv, encrypt);
}

/***************************************************************************//**
 * @brief
 *   AES Cipher-block chaining (CBC) cipher mode encryption/decryption, 256 bit
 *   key.
 *
 * @deprecated
 *   This function is present to preserve backwards compatibility. Use
 *   @ref CRYPTO_AES_CBC256 instead.
 ******************************************************************************/
__STATIC_INLINE void AES_CBC256(uint8_t * out,
                       const uint8_t * in,
                       unsigned int len,
                       const uint8_t * key,
                       const uint8_t * iv,
                       bool encrypt)
{
  CRYPTO_AES_CBC256(CRYPTO, out, in, len, key, iv, encrypt);
}

/***************************************************************************//**
 * @brief
 *   AES Cipher feedback (CFB) cipher mode encryption/decryption, 128 bit key.
 *
 * @deprecated
 *   This function is present to preserve backwards compatibility. Use
 *   @ref CRYPTO_AES_CFB128 instead.
 ******************************************************************************/
__STATIC_INLINE void AES_CFB128(uint8_t * out,
                       const uint8_t * in,
                       unsigned int len,
                       const uint8_t * key,
                       const uint8_t * iv,
                       bool encrypt)
{
  CRYPTO_AES_CFB128(CRYPTO, out, in, len, key, iv, encrypt);
}

/***************************************************************************//**
 * @brief
 *   AES Cipher feedback (CFB) cipher mode encryption/decryption, 256 bit key.
 *
 * @deprecated
 *   This function is present to preserve backwards compatibility. Use
 *   @ref CRYPTO_AES_CFB256 instead.
 ******************************************************************************/
__STATIC_INLINE void AES_CFB256(uint8_t * out,
                       const uint8_t * in,
                       unsigned int len,
                       const uint8_t * key,
                       const uint8_t * iv,
                       bool encrypt)
{
  CRYPTO_AES_CFB256(CRYPTO, out, in, len, key, iv, encrypt);
}

/***************************************************************************//**
 * @brief
 *   AES Counter (CTR) cipher mode encryption/decryption, 128 bit key.
 *
 * @deprecated
 *   This function is present to preserve backwards compatibility. Use
 *   @ref CRYPTO_AES_CTR128 instead.
 ******************************************************************************/
__STATIC_INLINE void AES_CTR128(uint8_t * out,
                       const uint8_t * in,
                       unsigned int len,
                       const uint8_t * key,
                       uint8_t * ctr,
                       CRYPTO_AES_CtrFuncPtr_TypeDef ctrFunc)
{
  CRYPTO_AES_CTR128(CRYPTO, out, in, len, key, ctr, ctrFunc);
}

/***************************************************************************//**
 * @brief
 *   AES Counter (CTR) cipher mode encryption/decryption, 256 bit key.
 *
 * @deprecated
 *   This function is present to preserve backwards compatibility. Use
 *   @ref CRYPTO_AES_CTR256 instead.
 ******************************************************************************/
__STATIC_INLINE void AES_CTR256(uint8_t * out,
                       const uint8_t * in,
                       unsigned int len,
                       const uint8_t * key,
                       uint8_t * ctr,
                       CRYPTO_AES_CtrFuncPtr_TypeDef ctrFunc)
{
  CRYPTO_AES_CTR256(CRYPTO, out, in, len, key, ctr, ctrFunc);
}

/***************************************************************************//**
 * @brief
 *   Update last 32 bits of 128 bit counter, by incrementing with 1.
 *
 * @deprecated
 *   This function is present to preserve backwards compatibility. Use
 *   @ref CRYPTO_AES_CTRUpdate32Bit instead.
 ******************************************************************************/
__STATIC_INLINE void AES_CTRUpdate32Bit(uint8_t * ctr)
{
  CRYPTO_AES_CTRUpdate32Bit(ctr);
}

/***************************************************************************//**
 * @brief
 *   Generate 128 bit AES decryption key from 128 bit encryption key. The
 *   decryption key is used for some cipher modes when decrypting.
 *
 * @deprecated
 *   This function is present to preserve backwards compatibility. Use
 *   @ref CRYPTO_AES_DecryptKey128 instead.
 ******************************************************************************/
__STATIC_INLINE void AES_DecryptKey128(uint8_t * out, const uint8_t * in)
{
  CRYPTO_AES_DecryptKey128(CRYPTO, out, in);
}

/***************************************************************************//**
 * @brief
 *   Generate 256 bit AES decryption key from 256 bit encryption key. The
 *   decryption key is used for some cipher modes when decrypting.
 *
 * @deprecated
 *   This function is present to preserve backwards compatibility. Use
 *   @ref CRYPTO_AES_DecryptKey256 instead.
 ******************************************************************************/
__STATIC_INLINE void AES_DecryptKey256(uint8_t * out, const uint8_t * in)
{
  CRYPTO_AES_DecryptKey256(CRYPTO, out, in);
}

/***************************************************************************//**
 * @brief
 *   AES Electronic Codebook (ECB) cipher mode encryption/decryption,
 *   128 bit key.
 *
 * @deprecated
 *   This function is present to preserve backwards compatibility. Use
 *   @ref CRYPTO_AES_ECB128 instead.
 ******************************************************************************/
__STATIC_INLINE void AES_ECB128(uint8_t * out,
                                const uint8_t * in,
                                unsigned int len,
                                const uint8_t * key,
                                bool encrypt)
{
  CRYPTO_AES_ECB128(CRYPTO, out, in, len, key, encrypt);
}

/***************************************************************************//**
 * @brief
 *   AES Electronic Codebook (ECB) cipher mode encryption/decryption,
 *   256 bit key.
 *
 * @deprecated
 *   This function is present to preserve backwards compatibility. Use
 *   @ref CRYPTO_AES_ECB256 instead.
 ******************************************************************************/
__STATIC_INLINE void AES_ECB256(uint8_t * out,
                                const uint8_t * in,
                                unsigned int len,
                                const uint8_t * key,
                                bool encrypt)
{
  CRYPTO_AES_ECB256(CRYPTO, out, in, len, key, encrypt);
}

/***************************************************************************//**
 * @brief
 *   AES Output feedback (OFB) cipher mode encryption/decryption, 128 bit key.
 *
 * @deprecated
 *   This function is present to preserve backwards compatibility. Use
 *   @ref CRYPTO_AES_OFB128 instead.
 ******************************************************************************/
__STATIC_INLINE void AES_OFB128(uint8_t * out,
                                const uint8_t * in,
                                unsigned int len,
                                const uint8_t * key,
                                const uint8_t * iv)
{
  CRYPTO_AES_OFB128(CRYPTO, out, in, len, key, iv);
}

/***************************************************************************//**
 * @brief
 *   AES Output feedback (OFB) cipher mode encryption/decryption, 256 bit key.
 *
 * @deprecated
 *   This function is present to preserve backwards compatibility. Use
 *   @ref CRYPTO_AES_OFB256 instead.
 ******************************************************************************/
__STATIC_INLINE void AES_OFB256(uint8_t * out,
                                const uint8_t * in,
                                unsigned int len,
                                const uint8_t * key,
                                const uint8_t * iv)
{
  CRYPTO_AES_OFB256(CRYPTO, out, in, len, key, iv);
}

#ifdef __cplusplus
}
#endif

/** @} (end addtogroup CRYPTO) */
/** @} (end addtogroup emlib) */

#endif /* defined(CRYPTO_COUNT) && (CRYPTO_COUNT > 0) */

#endif /* EM_CRYPTO_H */
