/*
 * Copyright 2018 NXP
 * All rights reserved.
 *
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _FSL_CASPER_H_
#define _FSL_CASPER_H_

#include "fsl_common.h"

/*! @file */

/*******************************************************************************
 * Definitions
 *******************************************************************************/

/*!
 * @addtogroup casper_driver
 * @{
 */
/*! @name Driver version */
/*@{*/
/*! @brief CASPER driver version. Version 2.0.2.
 *
 * Current version: 2.0.2
 *
 * Change log:
 * - Version 2.0.0
 *   - Initial version
 * - Version 2.0.1
 *   - Bug fix KPSDK-24531 double_scalar_multiplication() result may be all zeroes for some specific input
 * - Version 2.0.2
 *   - Bug fix KPSDK-25015 CASPER_MEMCPY hard-fault on LPC55xx when both source and destination buffers are outside of CASPER_RAM
 */
#define FSL_CASPER_DRIVER_VERSION (MAKE_VERSION(2, 0, 2))
/*@}*/

/*! @brief CASPER operation
 *
 */
typedef enum _casper_operation
{
    kCASPER_OpMul6464NoSum = 0x01, /*! Walking 1 or more of J loop, doing r=a*b using 64x64=128*/
    kCASPER_OpMul6464Sum =
        0x02, /*! Walking 1 or more of J loop, doing c,r=r+a*b using 64x64=128, but assume inner j loop*/
    kCASPER_OpMul6464FullSum =
        0x03, /*! Walking 1 or more of J loop, doing c,r=r+a*b using 64x64=128, but sum all of w. */
    kCASPER_OpMul6464Reduce =
        0x04,               /*! Walking 1 or more of J loop, doing c,r[-1]=r+a*b using 64x64=128, but skip 1st write*/
    kCASPER_OpAdd64 = 0x08, /*! Walking add with off_AB, and in/out off_RES doing c,r=r+a+c using 64+64=65*/
    kCASPER_OpSub64 = 0x09, /*! Walking subtract with off_AB, and in/out off_RES doing r=r-a uding 64-64=64, with last
                               borrow implicit if any*/
    kCASPER_OpDouble64 = 0x0A, /*! Walking add to self with off_RES doing c,r=r+r+c using 64+64=65*/
    kCASPER_OpXor64 = 0x0B,    /*! Walking XOR with off_AB, and in/out off_RES doing r=r^a using 64^64=64*/
    kCASPER_OpShiftLeft32 =
        0x10, /*! Walking shift left doing r1,r=(b*D)|r1, where D is 2^amt and is loaded by app (off_CD not used)*/
    kCASPER_OpShiftRight32 = 0x11, /*! Walking shift right doing r,r1=(b*D)|r1, where D is 2^(32-amt) and is loaded by
                                      app (off_CD not used) and off_RES starts at MSW*/
    kCASPER_OpCopy = 0x14,         /*! Copy from ABoff to resoff, 64b at a time*/
    kCASPER_OpRemask = 0x15,       /*! Copy and mask from ABoff to resoff, 64b at a time*/
    kCASPER_OpCompare = 0x16,      /*! Compare two arrays, running all the way to the end*/
    kCASPER_OpCompareFast = 0x17,  /*! Compare two arrays, stopping on 1st !=*/
} casper_operation_t;

#define CASPER_CP 1
#define CASPER_CP_CTRL0 (0x0 >> 2)
#define CASPER_CP_CTRL1 (0x4 >> 2)
#define CASPER_CP_LOADER (0x8 >> 2)
#define CASPER_CP_STATUS (0xC >> 2)
#define CASPER_CP_INTENSET (0x10 >> 2)
#define CASPER_CP_INTENCLR (0x14 >> 2)
#define CASPER_CP_INTSTAT (0x18 >> 2)
#define CASPER_CP_AREG (0x20 >> 2)
#define CASPER_CP_BREG (0x24 >> 2)
#define CASPER_CP_CREG (0x28 >> 2)
#define CASPER_CP_DREG (0x2C >> 2)
#define CASPER_CP_RES0 (0x30 >> 2)
#define CASPER_CP_RES1 (0x34 >> 2)
#define CASPER_CP_RES2 (0x38 >> 2)
#define CASPER_CP_RES3 (0x3C >> 2)
#define CASPER_CP_MASK (0x60 >> 2)
#define CASPER_CP_REMASK (0x64 >> 2)
#define CASPER_CP_LOCK (0x80 >> 2)
#define CASPER_CP_ID (0xFFC >> 2)
/* mcr (cp,  opc1, value, CRn, CRm, opc2) */
#define CASPER_Wr32b(value, off) __arm_mcr(CASPER_CP, 0, value, ((off >> 4)), (off), 0)
/* mcrr(coproc, opc1, value, CRm) */
#define CASPER_Wr64b(value, off) __arm_mcrr(CASPER_CP, 0, value, off)
/* mrc(coproc, opc1, CRn, CRm, opc2) */
#define CASPER_Rd32b(off) __arm_mrc(CASPER_CP, 0, ((off >> 4)), (off), 0)

/*  The model for this algo is that it can be implemented for a fixed size RSA key */
/*  for max speed. If this is made into a variable (to allow varying size), then */
/*  it will be slower by a bit. */
/*  The file is compiled with N_bitlen passed in as number of bits of the RSA key */
/*  #define N_bitlen 2048 */
#define N_wordlen_max (4096 / 32)

#define CASPER_ECC_P256 1
#define CASPER_ECC_P384 0

#if CASPER_ECC_P256
#define N_bitlen 256
#endif /* CASPER_ECC_P256 */

#if CASPER_ECC_P384
#define N_bitlen 384
#endif /* CASPER_ECC_P256 */

#define NUM_LIMBS (N_bitlen / 32)

enum
{
    kCASPER_RamOffset_Result = 0x0u,
    kCASPER_RamOffset_Base = (N_wordlen_max + 8u),
    kCASPER_RamOffset_TempBase = (2u * N_wordlen_max + 16u),
    kCASPER_RamOffset_Modulus = (kCASPER_RamOffset_TempBase + N_wordlen_max + 4u),
    kCASPER_RamOffset_M64 = 1022,
};

/*! @} */

/*******************************************************************************
 * API
 ******************************************************************************/
#if defined(__cplusplus)
extern "C" {
#endif

/*!
 * @addtogroup casper_driver
 * @{
 */

/*!
 * @brief Enables clock and disables reset for CASPER peripheral.
 *
 * Enable clock and disable reset for CASPER.
 *
 * @param base CASPER base address
 */
void CASPER_Init(CASPER_Type *base);

/*!
 * @brief Disables clock for CASPER peripheral.
 *
 * Disable clock and enable reset.
 *
 * @param base CASPER base address
 */
void CASPER_Deinit(CASPER_Type *base);

/*!
 *@}
 */ /* end of casper_driver */

/*******************************************************************************
 * PKHA API
 ******************************************************************************/

/*!
 * @addtogroup casper_driver_pkha
 * @{
 */

/*!
 * @brief Performs modular exponentiation - (A^E) mod N.
 *
 * This function performs modular exponentiation.
 *
 * @param base CASPER base address
 * @param signature first addend (in little endian format)
 * @param pubN modulus (in little endian format)
 * @param wordLen Size of pubN in bytes
 * @param pubE exponent
 * @param[out] plaintext Output array to store result of operation (in little endian format)
 */
void CASPER_ModExp(CASPER_Type *base,
                   const uint8_t *signature,
                   const uint8_t *pubN,
                   size_t wordLen,
                   uint32_t pubE,
                   uint8_t *plaintext);

void CASPER_ecc_init(void);

/*!
 * @brief Performs ECC secp256r1 point single scalar multiplication
 *
 * This function performs ECC secp256r1 point single scalar multiplication
 * [resX; resY] = scalar * [X; Y]
 * Coordinates are affine in normal form, little endian.
 * Scalars are little endian.
 * All arrays are little endian byte arrays, uint32_t type is used
 * only to enforce the 32-bit alignment (0-mod-4 address).
 *
 * @param base CASPER base address
 * @param[out] resX Output X affine coordinate in normal form, little endian.
 * @param[out] resY Output Y affine coordinate in normal form, little endian.
 * @param X Input X affine coordinate in normal form, little endian.
 * @param Y Input Y affine coordinate in normal form, little endian.
 * @param scalar Input scalar integer, in normal form, little endian.
 */
void CASPER_ECC_SECP256R1_Mul(
    CASPER_Type *base, uint32_t resX[8], uint32_t resY[8], uint32_t X[8], uint32_t Y[8], uint32_t scalar[8]);

/*!
 * @brief Performs ECC secp256r1 point double scalar multiplication
 *
 * This function performs ECC secp256r1 point double scalar multiplication
 * [resX; resY] = scalar1 * [X1; Y1] + scalar2 * [X2; Y2]
 * Coordinates are affine in normal form, little endian.
 * Scalars are little endian.
 * All arrays are little endian byte arrays, uint32_t type is used
 * only to enforce the 32-bit alignment (0-mod-4 address).
 *
 * @param base CASPER base address
 * @param[out] resX Output X affine coordinate.
 * @param[out] resY Output Y affine coordinate.
 * @param X1 Input X1 affine coordinate.
 * @param Y1 Input Y1 affine coordinate.
 * @param scalar1 Input scalar1 integer.
 * @param X2 Input X2 affine coordinate.
 * @param Y2 Input Y2 affine coordinate.
 * @param scalar2 Input scalar2 integer.
 */
void CASPER_ECC_SECP256R1_MulAdd(CASPER_Type *base,
                                 uint32_t resX[8],
                                 uint32_t resY[8],
                                 uint32_t X1[8],
                                 uint32_t Y1[8],
                                 uint32_t scalar1[8],
                                 uint32_t X2[8],
                                 uint32_t Y2[8],
                                 uint32_t scalar2[8]);

/*!
 * @brief Performs ECC secp384r1 point single scalar multiplication
 *
 * This function performs ECC secp384r1 point single scalar multiplication
 * [resX; resY] = scalar * [X; Y]
 * Coordinates are affine in normal form, little endian.
 * Scalars are little endian.
 * All arrays are little endian byte arrays, uint32_t type is used
 * only to enforce the 32-bit alignment (0-mod-4 address).
 *
 * @param base CASPER base address
 * @param[out] resX Output X affine coordinate in normal form, little endian.
 * @param[out] resY Output Y affine coordinate in normal form, little endian.
 * @param X Input X affine coordinate in normal form, little endian.
 * @param Y Input Y affine coordinate in normal form, little endian.
 * @param scalar Input scalar integer, in normal form, little endian.
 */
void CASPER_ECC_SECP384R1_Mul(
    CASPER_Type *base, uint32_t resX[12], uint32_t resY[12], uint32_t X[12], uint32_t Y[12], uint32_t scalar[12]);

/*!
 * @brief Performs ECC secp384r1 point double scalar multiplication
 *
 * This function performs ECC secp384r1 point double scalar multiplication
 * [resX; resY] = scalar1 * [X1; Y1] + scalar2 * [X2; Y2]
 * Coordinates are affine in normal form, little endian.
 * Scalars are little endian.
 * All arrays are little endian byte arrays, uint32_t type is used
 * only to enforce the 32-bit alignment (0-mod-4 address).
 *
 * @param base CASPER base address
 * @param[out] resX Output X affine coordinate.
 * @param[out] resY Output Y affine coordinate.
 * @param X1 Input X1 affine coordinate.
 * @param Y1 Input Y1 affine coordinate.
 * @param scalar1 Input scalar1 integer.
 * @param X2 Input X2 affine coordinate.
 * @param Y2 Input Y2 affine coordinate.
 * @param scalar2 Input scalar2 integer.
 */
void CASPER_ECC_SECP384R1_MulAdd(CASPER_Type *base,
                                 uint32_t resX[12],
                                 uint32_t resY[12],
                                 uint32_t X1[12],
                                 uint32_t Y1[12],
                                 uint32_t scalar1[12],
                                 uint32_t X2[12],
                                 uint32_t Y2[12],
                                 uint32_t scalar2[12]);

void CASPER_ECC_equal(int *res, uint32_t *op1, uint32_t *op2);
void CASPER_ECC_equal_to_zero(int *res, uint32_t *op1);

/*!
 *@}
 */ /* end of casper_driver_pkha */

#if defined(__cplusplus)
}
#endif

#endif /* _FSL_CASPER_H_ */
