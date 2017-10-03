/*
 * Copyright (c) 2017, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/** ============================================================================
 *  @file       ECCParams.h
 *
 *  This file contains a common definition for eliptic curve structures used
 *  throughout the ECC based drivers.
 *
 *  Elliptic curves may be expressed in multiple ways. The most common ones are
 *  the short Weierstrass form y^3 = x^2 + a*x + b mod p, where a and b are integer
 *  curve parameters and p is the prime of the curve. Another commonly found form
 *  is the Montgomery form By^2 = x^3 + Ax^2 + x mod p, where B and A are integer
 *  curve parameters and p is the prime of the curve. The last common form is the
 *  Edwards form x^2 + y^2 = 1 + dx^2y^2 mod p, where d is an integer curve parameter
 *  and p is the prime of the curve.
 *
 *  This API expects curves in short Weierstrass form. The other curve expressions
 *  may be converted to short Weierstrass form using substitution.
 */

#ifndef ti_drivers_cryptoutils_ecc_ECCParams__include
#define ti_drivers_cryptoutils_ecc_ECCParams__include

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/*!
 *
 *  @brief A structure containing the parameters of an elliptic curve in short Weierstrass form.
 *
 *  Elliptical Curve Cryptography (ECC) prime curve.
 *
 *  The equation used to define the curve is expressed in the short Weierstrass
 *  form y^3 = x^2 + a*x + b
 *
 */
typedef struct ECCParams_CurveParams_ {
    const size_t    length;         //!< Length of the curve in bytes. All other buffers have this length.
    const uint8_t   *prime;         //!< The prime that defines the field of the curve.
    const uint8_t   *order;         //!< Order of the curve.
    const uint8_t   *a;             //!< Coefficient a of the equation.
    const uint8_t   *b;             //!< Coefficient b of the equation.
    const uint8_t   *generatorX;    //!< X coordinate of the generator point of the curve.
    const uint8_t   *generatorY;    //!< Y coordinate of the generator point of the curve.
}
ECCParams_CurveParams;

/*!
 *
 *  @brief The NISTP256 curve in short Weierstrass form.
 *
 */
extern const ECCParams_CurveParams ECCParams_NISTP256;

#ifdef __cplusplus
}
#endif

#endif /* ti_drivers_cryptoutils_ecc_ECCParams__include */
