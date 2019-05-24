/******************************************************************************
*  Filename:       pka.h
*  Revised:        2018-07-19 15:07:05 +0200 (Thu, 19 Jul 2018)
*  Revision:       52294
*
*  Description:    PKA header file.
*
*  Copyright (c) 2015 - 2017, Texas Instruments Incorporated
*  All rights reserved.
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions are met:
*
*  1) Redistributions of source code must retain the above copyright notice,
*     this list of conditions and the following disclaimer.
*
*  2) Redistributions in binary form must reproduce the above copyright notice,
*     this list of conditions and the following disclaimer in the documentation
*     and/or other materials provided with the distribution.
*
*  3) Neither the name of the ORGANIZATION nor the names of its contributors may
*     be used to endorse or promote products derived from this software without
*     specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
*  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
*  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
*  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
*  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
*  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
*  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
*  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
*  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
*  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
*  POSSIBILITY OF SUCH DAMAGE.
*
******************************************************************************/

//*****************************************************************************
//
//! \addtogroup peripheral_group
//! @{
//! \addtogroup pka_api
//! @{
//
//*****************************************************************************

#ifndef __PKA_H__
#define __PKA_H__

//*****************************************************************************
//
// If building with a C++ compiler, make all of the definitions in this header
// have a C binding.
//
//*****************************************************************************
#ifdef __cplusplus
extern "C"
{
#endif

#include "../inc/hw_types.h"
#include "../inc/hw_memmap.h"
#include "../inc/hw_ints.h"
#include "../inc/hw_pka.h"
#include "../inc/hw_pka_ram.h"
#include "interrupt.h"
#include "sys_ctrl.h"
#include "debug.h"
#include <string.h>

//*****************************************************************************
//
// Support for DriverLib in ROM:
// This section renames all functions that are not "static inline", so that
// calling these functions will default to implementation in flash. At the end
// of this file a second renaming will change the defaults to implementation in
// ROM for available functions.
//
// To force use of the implementation in flash, e.g. for debugging:
// - Globally: Define DRIVERLIB_NOROM at project level
// - Per function: Use prefix "NOROM_" when calling the function
//
//*****************************************************************************
#if !defined(DOXYGEN)
    #define PKAClearPkaRam                  NOROM_PKAClearPkaRam
    #define PKAGetOpsStatus                 NOROM_PKAGetOpsStatus
    #define PKAArrayAllZeros                NOROM_PKAArrayAllZeros
    #define PKAZeroOutArray                 NOROM_PKAZeroOutArray
    #define PKABigNumModStart               NOROM_PKABigNumModStart
    #define PKABigNumModGetResult           NOROM_PKABigNumModGetResult
    #define PKABigNumDivideStart            NOROM_PKABigNumDivideStart
    #define PKABigNumDivideGetQuotient      NOROM_PKABigNumDivideGetQuotient
    #define PKABigNumDivideGetRemainder     NOROM_PKABigNumDivideGetRemainder
    #define PKABigNumCmpStart               NOROM_PKABigNumCmpStart
    #define PKABigNumCmpGetResult           NOROM_PKABigNumCmpGetResult
    #define PKABigNumInvModStart            NOROM_PKABigNumInvModStart
    #define PKABigNumInvModGetResult        NOROM_PKABigNumInvModGetResult
    #define PKABigNumMultiplyStart          NOROM_PKABigNumMultiplyStart
    #define PKABigNumMultGetResult          NOROM_PKABigNumMultGetResult
    #define PKABigNumAddStart               NOROM_PKABigNumAddStart
    #define PKABigNumAddGetResult           NOROM_PKABigNumAddGetResult
    #define PKABigNumSubStart               NOROM_PKABigNumSubStart
    #define PKABigNumSubGetResult           NOROM_PKABigNumSubGetResult
    #define PKAEccMultiplyStart             NOROM_PKAEccMultiplyStart
    #define PKAEccMontgomeryMultiplyStart   NOROM_PKAEccMontgomeryMultiplyStart
    #define PKAEccMultiplyGetResult         NOROM_PKAEccMultiplyGetResult
    #define PKAEccAddStart                  NOROM_PKAEccAddStart
    #define PKAEccAddGetResult              NOROM_PKAEccAddGetResult
    #define PKAEccVerifyPublicKeyWeierstrassStart NOROM_PKAEccVerifyPublicKeyWeierstrassStart
#endif




//*****************************************************************************
//
// Function return values
//
//*****************************************************************************
#define PKA_STATUS_SUCCESS                      0 //!< Success
#define PKA_STATUS_FAILURE                      1 //!< Failure
#define PKA_STATUS_INVALID_PARAM                2 //!< Invalid parameter
#define PKA_STATUS_BUF_UNDERFLOW                3 //!< Buffer underflow
#define PKA_STATUS_RESULT_0                     4 //!< Result is all zeros
#define PKA_STATUS_A_GREATER_THAN_B             5 //!< Big number compare return status if the first big number is greater than the second.
#define PKA_STATUS_A_LESS_THAN_B                6 //!< Big number compare return status if the first big number is less than the second.
#define PKA_STATUS_EQUAL                        7 //!< Big number compare return status if the first big number is equal to the second.
#define PKA_STATUS_OPERATION_BUSY               8 //!< PKA operation is in progress.
#define PKA_STATUS_OPERATION_RDY                9 //!< No PKA operation is in progress.
#define PKA_STATUS_LOCATION_IN_USE              10 //!< Location in PKA RAM is not available
#define PKA_STATUS_X_ZERO                       11 //!< X coordinate of public key is 0
#define PKA_STATUS_Y_ZERO                       12 //!< Y coordinate of public key is 0
#define PKA_STATUS_X_LARGER_THAN_PRIME          13 //!< X coordinate of public key is larger than the curve prime
#define PKA_STATUS_Y_LARGER_THAN_PRIME          14 //!< Y coordinate of public key is larger than the curve prime
#define PKA_STATUS_POINT_NOT_ON_CURVE           15 //!< The public key is not on the specified elliptic curve
#define PKA_STATUS_RESULT_ADDRESS_INCORRECT     16 //!< The address of the result passed into one of the PKA*GetResult functions is incorrect
#define PKA_STATUS_POINT_AT_INFINITY            17 //!< The ECC operation resulted in the point at infinity


//*****************************************************************************
//
// Length in bytes of NISTP224 parameters.
//
//*****************************************************************************
#define NISTP224_PARAM_SIZE_BYTES 28

//*****************************************************************************
//
// Length in bytes of NISTP256 parameters.
//
//*****************************************************************************
#define NISTP256_PARAM_SIZE_BYTES 32

//*****************************************************************************
//
// Length in bytes of NISTP384 parameters.
//
//*****************************************************************************
#define NISTP384_PARAM_SIZE_BYTES 48

//*****************************************************************************
//
// Length in bytes of NISTP521 parameters.
//
//*****************************************************************************
#define NISTP521_PARAM_SIZE_BYTES 66

//*****************************************************************************
//
// Length in bytes of BrainpoolP256R1 parameters.
//
//*****************************************************************************
#define BrainpoolP256R1_PARAM_SIZE_BYTES 32

//*****************************************************************************
//
// Length in bytes of BrainpoolP384R1 parameters.
//
//*****************************************************************************
#define BrainpoolP384R1_PARAM_SIZE_BYTES 48

//*****************************************************************************
//
// Length in bytes of BrainpoolP512R1 parameters.
//
//*****************************************************************************
#define BrainpoolP512R1_PARAM_SIZE_BYTES 64

//*****************************************************************************
//
// Length in bytes of Curve25519 parameters.
//
//*****************************************************************************
#define Curve25519_PARAM_SIZE_BYTES 32

//*****************************************************************************
//
// Union for parameters that forces 32-bit alignment on the byte array.
//
//*****************************************************************************
typedef union {
    uint8_t     byte[28];
    uint32_t    word[28 / sizeof(uint32_t)];
} PKA_EccParam224;

typedef union {
    uint8_t     byte[32];
    uint32_t    word[32 / sizeof(uint32_t)];
} PKA_EccParam256;

typedef union {
    uint8_t     byte[48];
    uint32_t    word[48 / sizeof(uint32_t)];
} PKA_EccParam384;

typedef union {
    uint8_t     byte[64];
    uint32_t    word[64 / sizeof(uint32_t)];
} PKA_EccParam512;

typedef union {
    uint8_t     byte[68];
    uint32_t    word[68 / sizeof(uint32_t)];
} PKA_EccParam521;

//*****************************************************************************
//
// Struct to keep points in that forces adjacency of X and Y coordinates in
// memmory.
//
//*****************************************************************************


typedef struct PKA_EccPoint224_ {
    PKA_EccParam224     x;
    PKA_EccParam224     y;
} PKA_EccPoint224;

typedef struct PKA_EccPoint256_ {
    PKA_EccParam256     x;
    PKA_EccParam256     y;
} PKA_EccPoint256;

typedef struct PKA_EccPoint384_ {
    PKA_EccParam384     x;
    PKA_EccParam384     y;
} PKA_EccPoint384;

typedef struct PKA_EccPoint512_ {
    PKA_EccParam512     x;
    PKA_EccParam512     y;
} PKA_EccPoint512;

typedef struct PKA_EccPoint521_ {
    PKA_EccParam521     x;
    PKA_EccParam521     y;
} PKA_EccPoint521;


//*****************************************************************************
//
//! \brief X coordinate of the generator point of the NISTP224 curve.
//
//*****************************************************************************
extern const PKA_EccPoint224 NISTP224_generator;

//*****************************************************************************
//
//! \brief Prime of the NISTP224 curve.
//
//*****************************************************************************
extern const PKA_EccParam224 NISTP224_prime;


//*****************************************************************************
//
//! \brief a constant of the NISTP224 curve when expressed in short
//! Weierstrass form (y^3 = x^2 + a*x + b).
//
//*****************************************************************************
extern const PKA_EccParam224 NISTP224_a;


//*****************************************************************************
//
//! \brief b constant of the NISTP224 curve when expressed in short
//! Weierstrass form (y^3 = x^2 + a*x + b).
//
//*****************************************************************************
extern const PKA_EccParam224 NISTP224_b;


//*****************************************************************************
//
//! \brief Order of the NISTP224 curve.
//
//*****************************************************************************
extern const PKA_EccParam224 NISTP224_order;




//*****************************************************************************
//
//! \brief X coordinate of the generator point of the NISTP256 curve.
//
//*****************************************************************************
extern const PKA_EccPoint256 NISTP256_generator;

//*****************************************************************************
//
//! \brief Prime of the NISTP256 curve.
//
//*****************************************************************************
extern const PKA_EccParam256 NISTP256_prime;


//*****************************************************************************
//
//! \brief a constant of the NISTP256 curve when expressed in short
//! Weierstrass form (y^3 = x^2 + a*x + b).
//
//*****************************************************************************
extern const PKA_EccParam256 NISTP256_a;


//*****************************************************************************
//
//! \brief b constant of the NISTP256 curve when expressed in short
//! Weierstrass form (y^3 = x^2 + a*x + b).
//
//*****************************************************************************
extern const PKA_EccParam256 NISTP256_b;


//*****************************************************************************
//
//! \brief Order of the NISTP256 curve.
//
//*****************************************************************************
extern const PKA_EccParam256 NISTP256_order;





//*****************************************************************************
//
//! \brief X coordinate of the generator point of the NISTP384 curve.
//
//*****************************************************************************
extern const PKA_EccPoint384 NISTP384_generator;

//*****************************************************************************
//
//! \brief Prime of the NISTP384 curve.
//
//*****************************************************************************
extern const PKA_EccParam384 NISTP384_prime;


//*****************************************************************************
//
//! \brief a constant of the NISTP384 curve when expressed in short
//! Weierstrass form (y^3 = x^2 + a*x + b).
//
//*****************************************************************************
extern const PKA_EccParam384 NISTP384_a;


//*****************************************************************************
//
//! \brief b constant of the NISTP384 curve when expressed in short
//! Weierstrass form (y^3 = x^2 + a*x + b).
//
//*****************************************************************************
extern const PKA_EccParam384 NISTP384_b;


//*****************************************************************************
//
//! \brief Order of the NISTP384 curve.
//
//*****************************************************************************
extern const PKA_EccParam384 NISTP384_order;




//*****************************************************************************
//
//! \brief X coordinate of the generator point of the NISTP521 curve.
//
//*****************************************************************************
extern const PKA_EccPoint521 NISTP521_generator;

//*****************************************************************************
//
//! \brief Prime of the NISTP521 curve.
//
//*****************************************************************************
extern const PKA_EccParam521 NISTP521_prime;


//*****************************************************************************
//
//! \brief a constant of the NISTP521 curve when expressed in short
//! Weierstrass form (y^3 = x^2 + a*x + b).
//
//*****************************************************************************
extern const PKA_EccParam521 NISTP521_a;


//*****************************************************************************
//
//! \brief b constant of the NISTP521 curve when expressed in short
//! Weierstrass form (y^3 = x^2 + a*x + b).
//
//*****************************************************************************
extern const PKA_EccParam521 NISTP521_b;


//*****************************************************************************
//
//! \brief Order of the NISTP521 curve.
//
//*****************************************************************************
extern const PKA_EccParam521 NISTP521_order;




//*****************************************************************************
//
//! \brief X coordinate of the generator point of the BrainpoolP256R1 curve.
//
//*****************************************************************************
extern const PKA_EccPoint256 BrainpoolP256R1_generator;

//*****************************************************************************
//
//! \brief Prime of the BrainpoolP256R1 curve.
//
//*****************************************************************************
extern const PKA_EccParam256 BrainpoolP256R1_prime;


//*****************************************************************************
//
//! \brief a constant of the BrainpoolP256R1 curve when expressed in short
//! Weierstrass form (y^3 = x^2 + a*x + b).
//
//*****************************************************************************
extern const PKA_EccParam256 BrainpoolP256R1_a;


//*****************************************************************************
//
//! \brief b constant of the BrainpoolP256R1 curve when expressed in short
//! Weierstrass form (y^3 = x^2 + a*x + b).
//
//*****************************************************************************
extern const PKA_EccParam256 BrainpoolP256R1_b;


//*****************************************************************************
//
//! \brief Order of the BrainpoolP256R1 curve.
//
//*****************************************************************************
extern const PKA_EccParam256 BrainpoolP256R1_order;




//*****************************************************************************
//
//! \brief X coordinate of the generator point of the BrainpoolP384R1 curve.
//
//*****************************************************************************
extern const PKA_EccPoint384 BrainpoolP384R1_generator;

//*****************************************************************************
//
//! \brief Prime of the BrainpoolP384R1 curve.
//
//*****************************************************************************
extern const PKA_EccParam384 BrainpoolP384R1_prime;


//*****************************************************************************
//
//! \brief a constant of the BrainpoolP384R1 curve when expressed in short
//! Weierstrass form (y^3 = x^2 + a*x + b).
//
//*****************************************************************************
extern const PKA_EccParam384 BrainpoolP384R1_a;


//*****************************************************************************
//
//! \brief b constant of the BrainpoolP384R1 curve when expressed in short
//! Weierstrass form (y^3 = x^2 + a*x + b).
//
//*****************************************************************************
extern const PKA_EccParam384 BrainpoolP384R1_b;


//*****************************************************************************
//
//! \brief Order of the BrainpoolP384R1 curve.
//
//*****************************************************************************
extern const PKA_EccParam384 BrainpoolP384R1_order;



//*****************************************************************************
//
//! \brief X coordinate of the generator point of the BrainpoolP512R1 curve.
//
//*****************************************************************************
extern const PKA_EccPoint512 BrainpoolP512R1_generator;

//*****************************************************************************
//
//! \brief Prime of the BrainpoolP512R1 curve.
//
//*****************************************************************************
extern const PKA_EccParam512 BrainpoolP512R1_prime;


//*****************************************************************************
//
//! \brief a constant of the BrainpoolP512R1 curve when expressed in short
//! Weierstrass form (y^3 = x^2 + a*x + b).
//
//*****************************************************************************
extern const PKA_EccParam512 BrainpoolP512R1_a;


//*****************************************************************************
//
//! \brief b constant of the BrainpoolP512R1 curve when expressed in short
//! Weierstrass form (y^3 = x^2 + a*x + b).
//
//*****************************************************************************
extern const PKA_EccParam512 BrainpoolP512R1_b;


//*****************************************************************************
//
//! \brief Order of the BrainpoolP512R1 curve.
//
//*****************************************************************************
extern const PKA_EccParam512 BrainpoolP512R1_order;



//*****************************************************************************
//
//! \brief X coordinate of the generator point of the Curve25519 curve.
//
//*****************************************************************************
extern const PKA_EccPoint256 Curve25519_generator;

//*****************************************************************************
//
//! \brief Prime of the Curve25519 curve.
//
//*****************************************************************************
extern const PKA_EccParam256 Curve25519_prime;


//*****************************************************************************
//
//! \brief a constant of the Curve25519 curve when expressed in Montgomery
//! form (By^2 = x^3 + a*x^2 + x).
//
//*****************************************************************************
extern const PKA_EccParam256 Curve25519_a;


//*****************************************************************************
//
//! \brief b constant of the Curve25519 curve when expressed in Montgomery
//! form (By^2 = x^3 + a*x^2 + x).
//
//*****************************************************************************
extern const PKA_EccParam256 Curve25519_b;


//*****************************************************************************
//
//! \brief Order of the Curve25519 curve.
//
//*****************************************************************************
extern const PKA_EccParam256 Curve25519_order;

//*****************************************************************************
//
// Prototypes for the APIs.
//
//*****************************************************************************

//*****************************************************************************
//
//! \brief Zeroizes PKA RAM.
//!
//! This function uses the zeroization function in PRCM to clear the PKA RAM.
//
//*****************************************************************************
extern void PKAClearPkaRam(void);

//*****************************************************************************
//
//! \brief Gets the PKA operation status.
//!
//! This function gets information on whether any PKA operation is in
//! progress or not. This function allows to check the PKA operation status
//! before starting any new PKA operation.
//!
//! \return Returns a status code.
//! - \ref PKA_STATUS_OPERATION_BUSY if the PKA operation is in progress.
//! - \ref PKA_STATUS_OPERATION_RDY if the PKA operation is not in progress.
//
//*****************************************************************************
extern uint32_t  PKAGetOpsStatus(void);

//*****************************************************************************
//
//! \brief Checks whether and array only consists of zeros
//!
//! \param [in] array is the array to check.
//!
//! \param [in] arrayLength is the length of the array.
//!
//! \return Returns true if the array contains only zeros and false if one
//! or more bits are set.
//
//*****************************************************************************
extern bool PKAArrayAllZeros(const uint8_t *array, uint32_t arrayLength);

//*****************************************************************************
//
//! \brief Zeros-out an array
//!
//! \param [in] array is the array to zero-out.
//!
//! \param [in] arrayLength is the length of the array.
//
//*****************************************************************************
extern void PKAZeroOutArray(const uint8_t *array, uint32_t arrayLength);

//*****************************************************************************
//
//! \brief Starts a big number modulus operation.
//!
//! This function starts the modulo operation on the big number \c bigNum
//! using the divisor \c modulus. The PKA RAM location where the result
//! will be available is stored in \c resultPKAMemAddr.
//!
//! \param [in] bigNum is the pointer to the big number on which modulo operation
//!        needs to be carried out.
//!
//! \param [in] bigNumLength is the size of the big number \c bigNum in bytes.
//!
//! \param [in] modulus is the pointer to the divisor.
//!
//! \param [in] modulusLength is the size of the divisor \c modulus in bytes.
//!
//! \param [out] resultPKAMemAddr is the pointer to the result vector location
//!        which will be set by this function.
//!
//!\return Returns a status code.
//! - \ref PKA_STATUS_SUCCESS if successful in starting the operation.
//! - \ref PKA_STATUS_OPERATION_BUSY, if the PKA module is busy doing
//!        some other operation.
//!
//! \sa PKABigNumModGetResult()
//
//*****************************************************************************
extern uint32_t  PKABigNumModStart(const uint8_t *bigNum, uint32_t bigNumLength, const uint8_t *modulus, uint32_t modulusLength, uint32_t *resultPKAMemAddr);

//*****************************************************************************
//
//! \brief Gets the result of the big number modulus operation.
//!
//! This function gets the result of the big number modulus operation which was
//! previously started using the function PKABigNumModStart().
//! The function will zero-out \c resultBuf prior to copying in the result of
//! the modulo operation.
//!
//! \param [out] resultBuf is the pointer to buffer where the result needs to
//!        be stored.
//!
//! \param [in] length is the size of the provided buffer in bytes.
//!
//! \param [in] resultPKAMemAddr is the address of the result location which
//!        was provided by the start function PKABigNumModStart().
//!
//! \return Returns a status code.
//! - \ref PKA_STATUS_SUCCESS if successful.
//! - \ref PKA_STATUS_OPERATION_BUSY if the PKA module is busy doing
//!        the operation.
//! - \ref PKA_STATUS_RESULT_0 if the result is all zeros.
//! - \ref PKA_STATUS_BUF_UNDERFLOW if the \c length is less than the length
//!        of the result.
//!
//! \sa PKABigNumModStart()
//
//*****************************************************************************
extern uint32_t  PKABigNumModGetResult(uint8_t *resultBuf, uint32_t length, uint32_t resultPKAMemAddr);

//*****************************************************************************
//
//! \brief Starts a big number divide operation.
//!
//! This function starts the dive operation on the big number \c bigNum
//! using the \c divisor. The PKA RAM location where the result
//! will be available is stored in \c resultPKAMemAddr.
//!
//! \param [in] dividend is the pointer to the big number to be divided.
//!
//! \param [in] dividendLength is the size of the big number \c dividend in bytes.
//!
//! \param [in] divisor is the pointer to the divisor.
//!
//! \param [in] divisorLength is the size of the \c divisor in bytes.
//!
//! \param [out] resultQuotientMemAddr is the pointer to the quotient vector location
//!        which will be set by this function.
//!
//! \param [out] resultRemainderMemAddr is the pointer to the remainder vector location
//!        which will be set by this function.
//!
//!\return Returns a status code.
//! - \ref PKA_STATUS_SUCCESS if successful in starting the operation.
//! - \ref PKA_STATUS_OPERATION_BUSY, if the PKA module is busy doing
//!        some other operation.
//!
//! \sa PKABigNumDivideGetResult()
//
//*****************************************************************************
extern uint32_t PKABigNumDivideStart(const uint8_t *dividend,
                                     uint32_t dividendLength,
                                     const uint8_t *divisor,
                                     uint32_t divisorLength,
                                     uint32_t *resultQuotientMemAddr,
                                     uint32_t *resultRemainderMemAddr);

//*****************************************************************************
//
//! \brief Gets the quotient of the big number divide operation.
//!
//! This function gets the quotient of the big number divide operation which was
//! previously started using the function PKABigNumDivideStart().
//!
//! \param [out] resultBuf is the pointer to buffer where the result needs to
//!        be stored.
//!
//! \param [in] length is the size of the provided buffer in bytes.
//!
//! \param [in] resultQuotientMemAddr is the address of the result location which
//!        was provided by the start function PKABigNumDivideStart().
//!
//! \return Returns a status code.
//! - \ref PKA_STATUS_SUCCESS if successful.
//! - \ref PKA_STATUS_OPERATION_BUSY if the PKA module is busy doing
//!        the operation.
//! - \ref PKA_STATUS_RESULT_0 if the result is all zeros.
//! - \ref PKA_STATUS_BUF_UNDERFLOW if the \c length is less than the length
//!        of the result.
//!
//! \sa PKABigNumDivideStart()
//
//*****************************************************************************
extern uint32_t PKABigNumDivideGetQuotient(uint8_t *resultBuf, uint32_t *length, uint32_t resultQuotientMemAddr);

//*****************************************************************************
//
//! \brief Gets the remainder of the big number divide operation.
//!
//! This function gets the remainder of the big number divide operation which was
//! previously started using the function PKABigNumDivideStart().
//!
//! \param [out] resultBuf is the pointer to buffer where the result needs to
//!        be stored.
//!
//! \param [in] length is the size of the provided buffer in bytes.
//!
//! \param [in] resultRemainderMemAddr is the address of the result location which
//!        was provided by the start function PKABigNumDivideStart().
//!
//! \return Returns a status code.
//! - \ref PKA_STATUS_SUCCESS if successful.
//! - \ref PKA_STATUS_OPERATION_BUSY if the PKA module is busy doing
//!        the operation.
//! - \ref PKA_STATUS_RESULT_0 if the result is all zeros.
//! - \ref PKA_STATUS_BUF_UNDERFLOW if the \c length is less than the length
//!        of the result.
//!
//! \sa PKABigNumDivideStart()
//
//*****************************************************************************
extern uint32_t PKABigNumDivideGetRemainder(uint8_t *resultBuf, uint32_t *length, uint32_t resultRemainderMemAddr);

//*****************************************************************************
//
//! \brief Starts the comparison of two big numbers.
//!
//! This function starts the comparison of two big numbers pointed by
//! \c bigNum1 and \c bigNum2.
//!
//! \note \c bigNum1 and \c bigNum2 must have same size.
//!
//! \param [in] bigNum1 is the pointer to the first big number.
//!
//! \param [in] bigNum2 is the pointer to the second big number.
//!
//! \param [in] length is the size of the big numbers in bytes.
//!
//!\return Returns a status code.
//! - \ref PKA_STATUS_SUCCESS if successful in starting the operation.
//! - \ref PKA_STATUS_OPERATION_BUSY if the PKA module is busy doing
//!        some other operation.
//!
//! \sa PKABigNumCmpGetResult()
//
//*****************************************************************************
extern uint32_t  PKABigNumCmpStart(const uint8_t *bigNum1, const uint8_t *bigNum2, uint32_t length);

//*****************************************************************************
//
//! \brief Gets the result of the comparison operation of two big numbers.
//!
//! This function provides the results of the comparison of two big numbers
//! which was started using the PKABigNumCmpStart().
//!
//! \return Returns a status code.
//! - \ref PKA_STATUS_OPERATION_BUSY if the operation is in progress.
//! - \ref PKA_STATUS_SUCCESS if the two big numbers are equal.
//! - \ref PKA_STATUS_A_GREATER_THAN_B  if the first number is greater than the second.
//! - \ref PKA_STATUS_A_LESS_THAN_B if the first number is less than the second.
//!
//! \sa PKABigNumCmpStart()
//
//*****************************************************************************
extern uint32_t  PKABigNumCmpGetResult(void);

//*****************************************************************************
//
//! \brief Starts a big number inverse modulo operation.
//!
//! This function starts the inverse modulo operation on \c bigNum
//! using the divisor \c modulus.
//!
//! \param [in] bigNum is the pointer to the buffer containing the big number
//!        (dividend).
//!
//! \param [in] bigNumLength is the size of the \c bigNum in bytes.
//!
//! \param [in] modulus is the pointer to the buffer containing the divisor.
//!
//! \param [in] modulusLength is the size of the divisor in bytes.
//!
//! \param [out] resultPKAMemAddr is the pointer to the result vector location
//!        which will be set by this function.
//!
//!
//!\return Returns a status code.
//! - \ref PKA_STATUS_SUCCESS if successful in starting the operation.
//! - \ref PKA_STATUS_OPERATION_BUSY if the PKA module is busy doing
//!        some other operation.
//!
//! \sa PKABigNumInvModGetResult()
//
//*****************************************************************************
extern uint32_t  PKABigNumInvModStart(const uint8_t *bigNum, uint32_t bigNumLength, const uint8_t *modulus, uint32_t modulusLength, uint32_t *resultPKAMemAddr);


//*****************************************************************************
//
//! \brief Gets the result of the big number inverse modulo operation.
//!
//! This function gets the result of the big number inverse modulo operation
//! previously started using the function PKABigNumInvModStart().
//! The function will zero-out \c resultBuf prior to copying in the result of
//! the inverse modulo operation.
//!
//! \param [out] resultBuf is the pointer to buffer where the result needs to be
//!        stored.
//!
//! \param [in] length is the size of the provided buffer in bytes.
//!
//! \param [in] resultPKAMemAddr is the address of the result location which
//!        was provided by the start function PKABigNumInvModStart().
//!
//! \return Returns a status code.
//! - \ref PKA_STATUS_SUCCESS if the operation is successful.
//! - \ref PKA_STATUS_OPERATION_BUSY if the PKA module is busy performing
//!        the operation.
//! - \ref PKA_STATUS_RESULT_0 if the result is all zeros.
//! - \ref PKA_STATUS_BUF_UNDERFLOW if the length of the provided buffer is less
//!        than the result.
//!
//! \sa PKABigNumInvModStart()
//
//*****************************************************************************
extern uint32_t  PKABigNumInvModGetResult(uint8_t *resultBuf, uint32_t length, uint32_t resultPKAMemAddr);


//*****************************************************************************
//
//! \brief Starts the multiplication of two big numbers.
//!
//! \param [in] multiplicand is the pointer to the buffer containing the big
//!        number multiplicand.
//!
//! \param [in] multiplicandLength is the size of the multiplicand in bytes.
//!
//! \param [in] multiplier is the pointer to the buffer containing the big
//!        number multiplier.
//!
//! \param [in] multiplierLength is the size of the multiplier in bytes.
//!
//! \param [out] resultPKAMemAddr is the pointer to the result vector location
//!        which will be set by this function.
//!
//!\return Returns a status code.
//! - \ref PKA_STATUS_SUCCESS if successful in starting the operation.
//! - \ref PKA_STATUS_OPERATION_BUSY if the PKA module is busy doing
//!        some other operation.
//!
//! \sa PKABigNumMultGetResult()
//
//*****************************************************************************
extern uint32_t  PKABigNumMultiplyStart(const uint8_t *multiplicand, uint32_t multiplicandLength, const uint8_t *multiplier, uint32_t multiplierLength, uint32_t *resultPKAMemAddr);


//*****************************************************************************
//
//! \brief Gets the result of the big number multiplication.
//!
//! This function gets the result of the multiplication of two big numbers
//! operation previously started using the function PKABigNumMultiplyStart().
//!
//! \param [out] resultBuf is the pointer to buffer where the result needs to be
//!        stored.
//!
//! \param [in, out] resultLength is the address of the variable containing the length of the
//!        buffer in bytes. After the operation, the actual length of the resultant is stored
//!        at this address.
//!
//! \param [in] resultPKAMemAddr is the address of the result location which
//!        was provided by the start function PKABigNumMultiplyStart().
//!
//! \return Returns a status code.
//! - \ref PKA_STATUS_SUCCESS if the operation is successful.
//! - \ref PKA_STATUS_OPERATION_BUSY if the PKA module is busy performing
//!        the operation.
//! - \ref PKA_STATUS_RESULT_0 if the result is all zeros.
//! - \ref PKA_STATUS_FAILURE if the operation is not successful.
//! - \ref PKA_STATUS_BUF_UNDERFLOW if the length of the provided buffer is less
//!        then the length of the result.
//!
//! \sa PKABigNumMultiplyStart()
//
//*****************************************************************************
extern uint32_t  PKABigNumMultGetResult(uint8_t *resultBuf, uint32_t *resultLength, uint32_t resultPKAMemAddr);

//*****************************************************************************
//
//! \brief Starts the addition of two big numbers.
//!
//! \param [in] bigNum1 is the pointer to the buffer containing the first
//!        big number.
//!
//! \param [in] bigNum1Length is the size of the first big number in bytes.
//!
//! \param [in] bigNum2 is the pointer to the buffer containing the second
//!        big number.
//!
//! \param [in] bigNum2Length is the size of the second big number in bytes.
//!
//! \param [out] resultPKAMemAddr is the pointer to the result vector location
//!        which will be set by this function.
//!
//!\return Returns a status code.
//! - \ref PKA_STATUS_SUCCESS if successful in starting the operation.
//! - \ref PKA_STATUS_OPERATION_BUSY if the PKA module is busy doing
//!        some other operation.
//!
//! \sa PKABigNumAddGetResult()
//
//*****************************************************************************
extern uint32_t  PKABigNumAddStart(const uint8_t *bigNum1, uint32_t bigNum1Length, const uint8_t *bigNum2, uint32_t bigNum2Length, uint32_t *resultPKAMemAddr);

//*****************************************************************************
//
//! \brief Gets the result of the addition operation on two big numbers.
//!
//! \param [out] resultBuf is the pointer to buffer where the result
//!        needs to be stored.
//!
//! \param [in, out] resultLength is the address of the variable containing
//!        the length of the buffer.  After the operation the actual length of the
//!        resultant is stored at this address.
//!
//! \param [in] resultPKAMemAddr is the address of the result location which
//!        was provided by the start function PKABigNumAddStart().
//!
//!
//! \return Returns a status code.
//! - \ref PKA_STATUS_SUCCESS if the operation is successful.
//! - \ref PKA_STATUS_OPERATION_BUSY if the PKA module is busy performing
//!        the operation.
//! - \ref PKA_STATUS_RESULT_0 if the result is all zeros.
//! - \ref PKA_STATUS_FAILURE if the operation is not successful.
//! - \ref PKA_STATUS_BUF_UNDERFLOW if the length of the provided buffer is less
//!        then the length of the result.
//!
//! \sa PKABigNumAddStart()
//
//*****************************************************************************
extern uint32_t  PKABigNumAddGetResult(uint8_t *resultBuf, uint32_t *resultLength, uint32_t resultPKAMemAddr);

//*****************************************************************************
//
//! \brief Starts the subtraction of one big number from another.
//!
//! \param [in] minuend is the pointer to the buffer containing the big number
//!             to be subtracted from.
//!
//! \param [in] minuendLength is the size of the minuend in bytes.
//!
//! \param [in] subtrahend is the pointer to the buffer containing the big
//!             number to subtract from the \c minuend.
//!
//! \param [in] subtrahendLength is the size of the subtrahend in bytes.
//!
//! \param [out] resultPKAMemAddr is the pointer to the result vector location
//!        which will be set by this function.
//!
//!\return Returns a status code.
//! - \ref PKA_STATUS_SUCCESS if successful in starting the operation.
//! - \ref PKA_STATUS_OPERATION_BUSY if the PKA module is busy doing
//!        some other operation.
//!
//! \sa PKABigNumSubGetResult()
//
//*****************************************************************************
extern uint32_t PKABigNumSubStart(const uint8_t *minuend, uint32_t minuendLength, const uint8_t *subtrahend, uint32_t subtrahendLength, uint32_t *resultPKAMemAddr);

//*****************************************************************************
//
//! \brief Gets the result of the subtraction operation on two big numbers.
//!
//! \param [out] resultBuf is the pointer to buffer where the result
//!        needs to be stored.
//!
//! \param [in, out] resultLength is the address of the variable containing
//!        the length of the buffer.  After the operation the actual length of the
//!        resultant is stored at this address.
//!
//! \param [in] resultPKAMemAddr is the address of the result location which
//!        was provided by the start function PKABigNumAddStart().
//!
//!
//! \return Returns a status code.
//! - \ref PKA_STATUS_SUCCESS if the operation is successful.
//! - \ref PKA_STATUS_OPERATION_BUSY if the PKA module is busy performing
//!        the operation.
//! - \ref PKA_STATUS_RESULT_0 if the result is all zeros.
//! - \ref PKA_STATUS_FAILURE if the operation is not successful.
//! - \ref PKA_STATUS_BUF_UNDERFLOW if the length of the provided buffer is less
//!        then the length of the result.
//!
//! \sa PKABigNumSubStart()
//
//*****************************************************************************
extern uint32_t PKABigNumSubGetResult(uint8_t *resultBuf, uint32_t *resultLength, uint32_t resultPKAMemAddr);

//*****************************************************************************
//
//! \brief Starts ECC multiplication.
//!
//! \param [in] scalar is pointer to the buffer containing the scalar
//!        value to be multiplied.
//!
//! \param [in] curvePointX is the pointer to the buffer containing the
//!        X coordinate of the elliptic curve point to be multiplied.
//!        The point must be on the given curve.
//!
//! \param [in] curvePointY is the pointer to the buffer containing the
//!        Y coordinate of the elliptic curve point to be multiplied.
//!        The point must be on the given curve.
//!
//! \param [in] prime is the prime of the curve.
//!
//! \param [in] a is the a constant of the curve when the curve equation is expressed
//!        in short Weierstrass form (y^3 = x^2 + a*x + b).
//!
//! \param [in] b is the b constant of the curve when the curve equation is expressed
//!        in short Weierstrass form (y^3 = x^2 + a*x + b).
//!
//! \param [in] length is the length of the curve parameters in bytes.
//!
//! \param [out] resultPKAMemAddr is the pointer to the result vector location
//!        which will be set by this function.
//!
//!\return Returns a status code.
//! - \ref PKA_STATUS_SUCCESS if successful in starting the operation.
//! - \ref PKA_STATUS_OPERATION_BUSY if the PKA module is busy doing
//!        some other operation.
//!
//! \sa PKAEccMultiplyGetResult()
//
//*****************************************************************************
extern uint32_t  PKAEccMultiplyStart(const uint8_t *scalar,
                                     const uint8_t *curvePointX,
                                     const uint8_t *curvePointY,
                                     const uint8_t *prime,
                                     const uint8_t *a,
                                     const uint8_t *b,
                                     uint32_t length,
                                     uint32_t *resultPKAMemAddr);

//*****************************************************************************
//
//! \brief Starts ECC Montgomery multiplication.
//!
//! \param [in] scalar is pointer to the buffer containing the scalar
//!        value to be multiplied.
//!
//! \param [in] curvePointX is the pointer to the buffer containing the
//!        X coordinate of the elliptic curve point to be multiplied.
//!        The point must be on the given curve.
//!
//! \param [in] prime is the prime of the curve.
//!
//! \param [in] a is the a constant of the curve when the curve equation is expressed
//!        in short Weierstrass form (y^3 = x^2 + a*x + b).
//!
//! \param [in] length is the length of the curve parameters in bytes.
//!
//! \param [out] resultPKAMemAddr is the pointer to the result vector location
//!        which will be set by this function.
//!
//!\return Returns a status code.
//! - \ref PKA_STATUS_SUCCESS if successful in starting the operation.
//! - \ref PKA_STATUS_OPERATION_BUSY if the PKA module is busy doing
//!        some other operation.
//!
//! \sa PKAEccMultiplyGetResult()
//
//*****************************************************************************
extern uint32_t PKAEccMontgomeryMultiplyStart(const uint8_t *scalar,
                                              const uint8_t *curvePointX,
                                              const uint8_t *prime,
                                              const uint8_t *a,
                                              uint32_t length,
                                              uint32_t *resultPKAMemAddr);

//*****************************************************************************
//
//! \brief Gets the result of ECC multiplication
//!
//! This function gets the result of ECC point multiplication operation on the
//! EC point and the scalar value, previously started using the function
//! PKAEccMultiplyStart().
//!
//! \param [out] curvePointX is the pointer to the structure where the X coordinate
//!         of the resultant EC point will be stored.
//!
//! \param [out] curvePointY is the pointer to the structure where the Y coordinate
//!         of the resultant EC point will be stored.
//!
//! \param [in] resultPKAMemAddr is the address of the result location which
//!        was provided by the start function PKAEccMultiplyStart().
//!
//! \param [in] length is the length of the curve parameters in bytes.
//!
//! \return Returns a status code.
//! - \ref PKA_STATUS_SUCCESS if the operation is successful.
//! - \ref PKA_STATUS_OPERATION_BUSY if the PKA module is busy performing
//!        the operation.
//! - \ref PKA_STATUS_RESULT_0 if the result is all zeros.
//! - \ref PKA_STATUS_FAILURE if the operation is not successful.
//!
//! \sa PKAEccMultiplyStart()
//
//*****************************************************************************
extern uint32_t  PKAEccMultiplyGetResult(uint8_t *curvePointX, uint8_t *curvePointY, uint32_t resultPKAMemAddr, uint32_t length);

//*****************************************************************************
//
//! \brief Starts the ECC addition.
//!
//! \param [in] curvePoint1X is the pointer to the buffer containing the
//!        X coordinate of the first elliptic curve point to be added.
//!        The point must be on the given curve.
//!
//! \param [in] curvePoint1Y is the pointer to the buffer containing the
//!        Y coordinate of the first elliptic curve point to be added.
//!        The point must be on the given curve.
//!
//! \param [in] curvePoint2X is the pointer to the buffer containing the
//!        X coordinate of the second elliptic curve point to be added.
//!        The point must be on the given curve.
//!
//! \param [in] curvePoint2Y is the pointer to the buffer containing the
//!        Y coordinate of the second elliptic curve point to be added.
//!        The point must be on the given curve.
//!
//! \param [in] prime is the prime of the curve.
//!
//! \param [in] a is the a constant of the curve when the curve equation is expressed
//!        in short Weierstrass form (y^3 = x^2 + a*x + b).
//!
//! \param [in] length is the length of the curve parameters in bytes.
//!
//! \param [out] resultPKAMemAddr is the pointer to the result vector location
//!        which will be set by this function.
//!
//!\return Returns a status code.
//! - \ref PKA_STATUS_SUCCESS if successful in starting the operation.
//! - \ref PKA_STATUS_OPERATION_BUSY if the PKA module is busy doing
//!        some other operation.
//!
//! \sa PKAEccAddGetResult()
//
//*****************************************************************************
extern uint32_t  PKAEccAddStart(const uint8_t *curvePoint1X,
                                const uint8_t *curvePoint1Y,
                                const uint8_t *curvePoint2X,
                                const uint8_t *curvePoint2Y,
                                const uint8_t *prime,
                                const uint8_t *a,
                                uint32_t length,
                                uint32_t *resultPKAMemAddr);

//*****************************************************************************
//
//! \brief Gets the result of the ECC addition
//!
//! This function gets the result of ECC point addition operation on the
//! on the two given EC points, previously started using the function
//! PKAEccAddStart().
//!
//! \param [out] curvePointX is the pointer to the structure where the X coordinate
//!         of the resultant EC point will be stored.
//!
//! \param [out] curvePointY is the pointer to the structure where the Y coordinate
//!         of the resultant EC point will be stored.
//!
//! \param [in] resultPKAMemAddr is the address of the result location which
//!        was provided by the start function PKAEccAddGetResult().
//!
//! \param [in] length is the length of the curve parameters in bytes.
//!
//! \return Returns a status code.
//! - \ref PKA_STATUS_SUCCESS if the operation is successful.
//! - \ref PKA_STATUS_OPERATION_BUSY if the PKA module is busy performing the operation.
//! - \ref PKA_STATUS_RESULT_0 if the result is all zeros.
//! - \ref PKA_STATUS_FAILURE if the operation is not successful.
//!
//! \sa PKAEccAddStart()
//
//*****************************************************************************
extern uint32_t  PKAEccAddGetResult(uint8_t *curvePointX, uint8_t *curvePointY, uint32_t resultPKAMemAddr, uint32_t length);


//*****************************************************************************
//
//! \brief Begins the validation of a public key against a Short-Weierstrass curve
//!
//! This function validates a public key against a curve.
//! After performing multiple smaller PKA operations in polling mode,
//! it starts an ECC scalar multiplication.
//!
//! The function verifies that:
//!  - X and Y are in the range [1, prime - 1]
//!  - The point is not the point at infinity
//!  - X and Y satisfy the Short-Weierstrass curve equation Y^2 = X^3 + a*X + b mod P
//!  - Multiplying the point by the order of the curve yields the point at infinity
//!
//! \param [in] curvePointX is the pointer to the buffer containing the
//!        X coordinate of the elliptic curve point to verify.
//!
//! \param [in] curvePointY is the pointer to the buffer containing the
//!        Y coordinate of the elliptic curve point to verify.
//!
//! \param [in] prime is the prime of the curve.
//!
//! \param [in] a is the a constant of the curve when the curve equation is expressed
//!        in Short-Weierstrass form (y^3 = x^2 + a*x + b).
//!
//! \param [in] b is the b constant of the curve when the curve equation is expressed
//!        in Short-Weierstrass form (y^3 = x^2 + a*x + b).
//!
//! \param [in] order is the order of the curve.
//!
//! \param [in] length is the length of the curve parameters in bytes.
//!
//! \return Returns a status code.
//! - \ref PKA_STATUS_SUCCESS if the operation is successful.
//! - \ref PKA_STATUS_OPERATION_BUSY if the PKA module is busy performing the operation.
//! - \ref PKA_STATUS_FAILURE if the operation is not successful.
//! - \ref PKA_STATUS_X_ZERO if X is zero.
//! - \ref PKA_STATUS_Y_ZERO if Y is zero.
//! - \ref PKA_STATUS_X_LARGER_THAN_PRIME if X is larger than the curve prime
//! - \ref PKA_STATUS_Y_LARGER_THAN_PRIME if Y is larger than the curve prime
//! - \ref PKA_STATUS_POINT_NOT_ON_CURVE if X and Y do not satisfy the curve equation
//!
//! \sa PKAEccVerifyPublicKeyGetResult()
//
//*****************************************************************************
extern uint32_t PKAEccVerifyPublicKeyWeierstrassStart(const uint8_t *curvePointX,
                                                      const uint8_t *curvePointY,
                                                      const uint8_t *prime,
                                                      const uint8_t *a,
                                                      const uint8_t *b,
                                                      const uint8_t *order,
                                                      uint32_t length);

//*****************************************************************************
//
// Support for DriverLib in ROM:
// Redirect to implementation in ROM when available.
//
//*****************************************************************************
#if !defined(DRIVERLIB_NOROM) && !defined(DOXYGEN)
    #include "../driverlib/rom.h"
    #ifdef ROM_PKAClearPkaRam
        #undef  PKAClearPkaRam
        #define PKAClearPkaRam                  ROM_PKAClearPkaRam
    #endif
    #ifdef ROM_PKAGetOpsStatus
        #undef  PKAGetOpsStatus
        #define PKAGetOpsStatus                 ROM_PKAGetOpsStatus
    #endif
    #ifdef ROM_PKAArrayAllZeros
        #undef  PKAArrayAllZeros
        #define PKAArrayAllZeros                ROM_PKAArrayAllZeros
    #endif
    #ifdef ROM_PKAZeroOutArray
        #undef  PKAZeroOutArray
        #define PKAZeroOutArray                 ROM_PKAZeroOutArray
    #endif
    #ifdef ROM_PKABigNumModStart
        #undef  PKABigNumModStart
        #define PKABigNumModStart               ROM_PKABigNumModStart
    #endif
    #ifdef ROM_PKABigNumModGetResult
        #undef  PKABigNumModGetResult
        #define PKABigNumModGetResult           ROM_PKABigNumModGetResult
    #endif
    #ifdef ROM_PKABigNumDivideStart
        #undef  PKABigNumDivideStart
        #define PKABigNumDivideStart            ROM_PKABigNumDivideStart
    #endif
    #ifdef ROM_PKABigNumDivideGetQuotient
        #undef  PKABigNumDivideGetQuotient
        #define PKABigNumDivideGetQuotient      ROM_PKABigNumDivideGetQuotient
    #endif
    #ifdef ROM_PKABigNumDivideGetRemainder
        #undef  PKABigNumDivideGetRemainder
        #define PKABigNumDivideGetRemainder     ROM_PKABigNumDivideGetRemainder
    #endif
    #ifdef ROM_PKABigNumCmpStart
        #undef  PKABigNumCmpStart
        #define PKABigNumCmpStart               ROM_PKABigNumCmpStart
    #endif
    #ifdef ROM_PKABigNumCmpGetResult
        #undef  PKABigNumCmpGetResult
        #define PKABigNumCmpGetResult           ROM_PKABigNumCmpGetResult
    #endif
    #ifdef ROM_PKABigNumInvModStart
        #undef  PKABigNumInvModStart
        #define PKABigNumInvModStart            ROM_PKABigNumInvModStart
    #endif
    #ifdef ROM_PKABigNumInvModGetResult
        #undef  PKABigNumInvModGetResult
        #define PKABigNumInvModGetResult        ROM_PKABigNumInvModGetResult
    #endif
    #ifdef ROM_PKABigNumMultiplyStart
        #undef  PKABigNumMultiplyStart
        #define PKABigNumMultiplyStart          ROM_PKABigNumMultiplyStart
    #endif
    #ifdef ROM_PKABigNumMultGetResult
        #undef  PKABigNumMultGetResult
        #define PKABigNumMultGetResult          ROM_PKABigNumMultGetResult
    #endif
    #ifdef ROM_PKABigNumAddStart
        #undef  PKABigNumAddStart
        #define PKABigNumAddStart               ROM_PKABigNumAddStart
    #endif
    #ifdef ROM_PKABigNumAddGetResult
        #undef  PKABigNumAddGetResult
        #define PKABigNumAddGetResult           ROM_PKABigNumAddGetResult
    #endif
    #ifdef ROM_PKABigNumSubStart
        #undef  PKABigNumSubStart
        #define PKABigNumSubStart               ROM_PKABigNumSubStart
    #endif
    #ifdef ROM_PKABigNumSubGetResult
        #undef  PKABigNumSubGetResult
        #define PKABigNumSubGetResult           ROM_PKABigNumSubGetResult
    #endif
    #ifdef ROM_PKAEccMultiplyStart
        #undef  PKAEccMultiplyStart
        #define PKAEccMultiplyStart             ROM_PKAEccMultiplyStart
    #endif
    #ifdef ROM_PKAEccMontgomeryMultiplyStart
        #undef  PKAEccMontgomeryMultiplyStart
        #define PKAEccMontgomeryMultiplyStart   ROM_PKAEccMontgomeryMultiplyStart
    #endif
    #ifdef ROM_PKAEccMultiplyGetResult
        #undef  PKAEccMultiplyGetResult
        #define PKAEccMultiplyGetResult         ROM_PKAEccMultiplyGetResult
    #endif
    #ifdef ROM_PKAEccAddStart
        #undef  PKAEccAddStart
        #define PKAEccAddStart                  ROM_PKAEccAddStart
    #endif
    #ifdef ROM_PKAEccAddGetResult
        #undef  PKAEccAddGetResult
        #define PKAEccAddGetResult              ROM_PKAEccAddGetResult
    #endif
    #ifdef ROM_PKAEccVerifyPublicKeyWeierstrassStart
        #undef  PKAEccVerifyPublicKeyWeierstrassStart
        #define PKAEccVerifyPublicKeyWeierstrassStart ROM_PKAEccVerifyPublicKeyWeierstrassStart
    #endif
#endif

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

#endif  // __PKA_H__

//*****************************************************************************
//
//! Close the Doxygen group.
//! @}
//! @}
//
//*****************************************************************************
