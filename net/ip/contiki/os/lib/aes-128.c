/* --COPYRIGHT--,BSD
 * Copyright (c) 2011, Texas Instruments Incorporated
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
 * --/COPYRIGHT--*/
/*
 * TI_aes_128_encr_only.c
 *
 *  Created on: Nov 3, 2011
 *      Author: Eric Peeters
 */

/**
 * \file
 *         Wrapped AES-128 implementation from Texas Instruments. 
 * \author
 *         Konrad Krentz <konrad.krentz@gmail.com>
 */

#include "lib/aes-128.h"
#include <string.h>

static const uint8_t sbox[256] =   { 
0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16 };

static uint8_t round_keys[11][AES_128_KEY_LENGTH];

/*---------------------------------------------------------------------------*/
/* multiplies by 2 in GF(2) */
static uint8_t
galois_mul2(uint8_t value)
{
  if(value >> 7) {
    value = value << 1;
    return value ^ 0x1b;
  } else {
    return value << 1;
  }
}
/*---------------------------------------------------------------------------*/
static void
set_key(uint8_t *key)
{
  uint8_t i;
  uint8_t j;
  uint8_t rcon;
  
  rcon = 0x01;
  memcpy(round_keys[0], key, AES_128_KEY_LENGTH);
  for(i = 1; i <= 10; i++) {
    round_keys[i][0] = sbox[round_keys[i - 1][13]] ^ round_keys[i - 1][0] ^ rcon;
    round_keys[i][1] = sbox[round_keys[i - 1][14]] ^ round_keys[i - 1][1];
    round_keys[i][2] = sbox[round_keys[i - 1][15]] ^ round_keys[i - 1][2];
    round_keys[i][3] = sbox[round_keys[i - 1][12]] ^ round_keys[i - 1][3];
    for(j = 4; j < AES_128_BLOCK_SIZE; j++) {
      round_keys[i][j] = round_keys[i - 1][j] ^ round_keys[i][j - 4];
    }
    rcon = galois_mul2(rcon);
  }
}
/*---------------------------------------------------------------------------*/
static void
encrypt(uint8_t *state)
{
  uint8_t buf1, buf2, buf3, buf4, round, i;
  
  /* round 0 */
  /* AddRoundKey */
  for(i = 0; i < AES_128_BLOCK_SIZE; i++) {
    state[i] = state[i] ^ round_keys[0][i];
  }
  
  for(round = 1; round <= 10; round++) {
    /* ByteSub */
    for(i = 0; i < AES_128_BLOCK_SIZE; i++) {
      state[i] = sbox[state[i]];
    }
      
    /* ShiftRow */
    buf1 = state[1];
    state[1] = state[5];
    state[5] = state[9];
    state[9] = state[13];
    state[13] = buf1;

    buf1 = state[2];
    buf2 = state[6];
    state[2] = state[10];
    state[6] = state[14];
    state[10] = buf1;
    state[14] = buf2;

    buf1 = state[15];
    state[15] = state[11];
    state[11] = state[7];
    state[7] = state[3];
    state[3] = buf1;

    /* last round skips MixColumn */
    if(round < 10) {
      /* MixColumn */
      for(i = 0; i < 4; i++) {
        buf4 = (i << 2);
        buf1 = state[buf4] ^ state[buf4 + 1] ^ state[buf4 + 2] ^ state[buf4 + 3];
        buf2 = state[buf4];
        buf3 = state[buf4] ^ state[buf4 + 1];
        buf3 = galois_mul2(buf3);
        
        state[buf4] = state[buf4] ^ buf3 ^ buf1;
        
        buf3 = state[buf4 + 1] ^ state[buf4 + 2];
        buf3 = galois_mul2(buf3);
        state[buf4 + 1] = state[buf4 + 1] ^ buf3 ^ buf1;
        
        buf3 = state[buf4 + 2] ^ state[buf4 + 3];
        buf3 = galois_mul2(buf3);
        state[buf4 + 2] = state[buf4 + 2] ^ buf3 ^ buf1;
        
        buf3 = state[buf4 + 3] ^ buf2;
        buf3 = galois_mul2(buf3);
        state[buf4 + 3] = state[buf4 + 3] ^ buf3 ^ buf1;
      }
    }
    
    /* AddRoundKey */
    for(i = 0; i < AES_128_BLOCK_SIZE; i++) {
      state[i] = state[i] ^ round_keys[round][i];
    }
  }
}
/*---------------------------------------------------------------------------*/
void
aes_128_padded_encrypt(uint8_t *plaintext_and_result, uint8_t plaintext_len)
{
  uint8_t block[AES_128_BLOCK_SIZE];
  
  memset(block, 0, AES_128_BLOCK_SIZE);
  memcpy(block, plaintext_and_result, plaintext_len);
  AES_128.encrypt(block);
  memcpy(plaintext_and_result, block, plaintext_len);
}
/*---------------------------------------------------------------------------*/
void
aes_128_set_padded_key(uint8_t *key, uint8_t key_len)
{
  uint8_t block[AES_128_BLOCK_SIZE];
  
  memset(block, 0, AES_128_BLOCK_SIZE);
  memcpy(block, key, key_len);
  AES_128.set_key(block);
}
/*---------------------------------------------------------------------------*/
const struct aes_128_driver aes_128_driver = {
  set_key,
  encrypt
};
/*---------------------------------------------------------------------------*/
