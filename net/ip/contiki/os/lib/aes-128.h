/*
 * Copyright (c) 2013, Hasso-Plattner-Institut.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 */

/**
 * \file
 *         AES-128.
 * \author
 *         Konrad Krentz <konrad.krentz@gmail.com>
 */

#ifndef AES_H_
#define AES_H_

#include "contiki.h"

#define AES_128_BLOCK_SIZE 16
#define AES_128_KEY_LENGTH 16

#ifdef AES_128_CONF
#define AES_128            AES_128_CONF
#else /* AES_128_CONF */
#define AES_128            aes_128_driver
#endif /* AES_128_CONF */

/**
 * Structure of AES drivers.
 */
struct aes_128_driver {
  
  /**
   * \brief Sets the current key.
   */
  void (* set_key)(uint8_t *key);
  
  /**
   * \brief Encrypts.
   */
  void (* encrypt)(uint8_t *plaintext_and_result);
};

/**
 * \brief Pads the plaintext with zeroes before calling AES_128.encrypt
 */
void aes_128_padded_encrypt(uint8_t *plaintext_and_result, uint8_t plaintext_len);

/**
 * \brief Pads the key with zeroes before calling AES_128.set_key
 */
void aes_128_set_padded_key(uint8_t *key, uint8_t key_len);

extern const struct aes_128_driver AES_128;

#endif /* AES_H_ */
