/*
 * Copyright (c) 2012 - 2018, Nordic Semiconductor ASA
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

//lint -e438

#include <nrfx.h>
#include "nrf_ecb.h"
#include <string.h>

static uint8_t  ecb_data[48];   ///< ECB data structure for RNG peripheral to access.
static uint8_t* ecb_key;        ///< Key:        Starts at ecb_data
static uint8_t* ecb_cleartext;  ///< Cleartext:  Starts at ecb_data + 16 bytes.
static uint8_t* ecb_ciphertext; ///< Ciphertext: Starts at ecb_data + 32 bytes.

bool nrf_ecb_init(void)
{
  ecb_key = ecb_data;
  ecb_cleartext  = ecb_data + 16;
  ecb_ciphertext = ecb_data + 32;

  NRF_ECB->ECBDATAPTR = (uint32_t)ecb_data;
  return true;
}


bool nrf_ecb_crypt(uint8_t * dest_buf, const uint8_t * src_buf)
{
   uint32_t counter = 0x1000000;
   if (src_buf != ecb_cleartext)
   {
     memcpy(ecb_cleartext,src_buf,16);
   }
   NRF_ECB->EVENTS_ENDECB = 0;
   NRF_ECB->TASKS_STARTECB = 1;
   while (NRF_ECB->EVENTS_ENDECB == 0)
   {
    counter--;
    if (counter == 0)
    {
      return false;
    }
   }
   NRF_ECB->EVENTS_ENDECB = 0;
   if (dest_buf != ecb_ciphertext)
   {
     memcpy(dest_buf,ecb_ciphertext,16);
   }
   return true;
}

void nrf_ecb_set_key(const uint8_t * key)
{
  memcpy(ecb_key,key,16);
}


