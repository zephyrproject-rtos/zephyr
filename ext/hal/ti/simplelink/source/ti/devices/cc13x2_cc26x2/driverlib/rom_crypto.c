/*******************************************************************************
*  Filename:       rom_crypto.c
*  Revised:        2018-09-17 08:57:21 +0200 (Mon, 17 Sep 2018)
*  Revision:       52619
*
*  Description:    This is the implementation for the API to the ECC functions
*                  built into ROM on the CC13x2/CC26x2.
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
*******************************************************************************/

#include <stdint.h>
#include "rom_crypto.h"


////////////////////////////////////* ECC *////////////////////////////////////
#ifdef ECC_PRIME_NIST256_CURVE
//#define TEST_NIST256
//#define PARAM_P NIST256_p;
#define PARAM_P 0x100257d4;

//#define PARAM_R NIST256_r;
#define PARAM_R 0x100257f8;

//#define PARAM_A NIST256_a;
#define PARAM_A 0x1002581c;

//#define PARAM_B NIST256_b;
#define PARAM_B 0x10025840;

//#define PARAM_GX NIST256_Gx;
#define PARAM_GX 0x10025864;

//#define PARAM_GY NIST256_Gy;
#define PARAM_GY 0x10025888;

#endif


//*****************************************************************************
// ECC_initialize
//*****************************************************************************
void
ECC_initialize(uint32_t *pWorkzone)
{
  // Initialize curve parameters
  //data_p  = (uint32_t *)PARAM_P;
  *((uint32_t **)0x20000138) = (uint32_t *)PARAM_P;

  //data_r  = (uint32_t *)PARAM_R;
  *((uint32_t **)0x2000013c) = (uint32_t *)PARAM_R;

  //data_a  = (uint32_t *)PARAM_A;
  *((uint32_t **)0x20000140) = (uint32_t *)PARAM_A;

  //data_b  = (uint32_t *)PARAM_B;
  *((uint32_t **)0x20000144) = (uint32_t *)PARAM_B;

  //data_Gx = (uint32_t *)PARAM_GX;
  *((uint32_t **)0x2000012c) = (uint32_t *)PARAM_GX;

  //data_Gy = (uint32_t *)PARAM_GY;
  *((uint32_t **)0x20000130) = (uint32_t *)PARAM_GY;

  // Initialize window size
  //win = (uint8_t) ECC_WINDOW_SIZE;
  *((uint8_t *)0x20000148) = (uint8_t) ECC_WINDOW_SIZE;

  // Initialize work zone
  //workzone = (uint32_t *) pWorkzone;
  *((uint32_t **)0x20000134) = (uint32_t *) pWorkzone;
}

typedef uint8_t(*ecc_keygen_t)(uint32_t *, uint32_t *,uint32_t *, uint32_t *);
ecc_keygen_t ecc_generatekey = (ecc_keygen_t)(0x1001f94d);

typedef uint8_t(*ecdsa_sign_t)(uint32_t *, uint32_t *,uint32_t *, uint32_t *, uint32_t *);
ecdsa_sign_t ecc_ecdsa_sign = (ecdsa_sign_t)(0x10010381);

typedef uint8_t(*ecdsa_verify_t)(uint32_t *, uint32_t *,uint32_t *, uint32_t *, uint32_t *);
ecdsa_verify_t ecc_ecdsa_verify = (ecdsa_verify_t)(0x1000c805);

typedef uint8_t(*ecdh_computeSharedSecret_t)(uint32_t *, uint32_t *,uint32_t *, uint32_t *, uint32_t *);
ecdh_computeSharedSecret_t ecdh_computeSharedSecret = (ecdh_computeSharedSecret_t)(0x10023485);

//*****************************************************************************
// ECC_generateKey
//*****************************************************************************
uint8_t
ECC_generateKey(uint32_t *randString, uint32_t *privateKey,
                uint32_t *publicKey_x, uint32_t *publicKey_y)
{
  return (uint8_t)ecc_generatekey((uint32_t*)randString, (uint32_t*)privateKey,
                                  (uint32_t*)publicKey_x, (uint32_t*)publicKey_y);

}

//*****************************************************************************
// ECC_ECDSA_sign
//*****************************************************************************
uint8_t
ECC_ECDSA_sign(uint32_t *secretKey, uint32_t *text, uint32_t *randString,
               uint32_t *sign1, uint32_t *sign2)
{
  return (uint8_t)ecc_ecdsa_sign((uint32_t*)secretKey, (uint32_t*)text, (uint32_t*)randString,
                             (uint32_t*)sign1, (uint32_t*)sign2);
}

//*****************************************************************************
// ECC_ECDSA_verify
//*****************************************************************************
uint8_t
ECC_ECDSA_verify(uint32_t *publicKey_x, uint32_t *publicKey_y,
                 uint32_t *text, uint32_t *sign1, uint32_t *sign2)
{
  return (uint8_t)ecc_ecdsa_verify((uint32_t*)publicKey_x, (uint32_t*)publicKey_y, (uint32_t*)text,
                              (uint32_t*)sign1, (uint32_t*)sign2);
}

//*****************************************************************************
// ECC_ECDH_computeSharedSecret
//*****************************************************************************
uint8_t
ECC_ECDH_computeSharedSecret(uint32_t *privateKey, uint32_t *publicKey_x,
                             uint32_t *publicKey_y, uint32_t *sharedSecret_x,
                             uint32_t *sharedSecret_y)
{
  return (uint8_t)ecdh_computeSharedSecret((uint32_t*)privateKey, (uint32_t*)publicKey_x,
                                 (uint32_t*)publicKey_y, (uint32_t*)sharedSecret_x,
                                 (uint32_t*)sharedSecret_y);
}
