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
 *         AES_128-based CCM* implementation.
 * \author
 *         Konrad Krentz <konrad.krentz@gmail.com>
 */

/**
 * \addtogroup llsec802154
 * @{
 */

#include <net/l2_buf.h>

#include "contiki/llsec/ccm-star.h"
#include "contiki/llsec/llsec802154.h"
#include "contiki/packetbuf.h"
#include "lib/aes-128.h"
#include <string.h>

/*---------------------------------------------------------------------------*/
static void
set_nonce(struct net_buf *buf, uint8_t *nonce,
    uint8_t flags,
    const uint8_t *extended_source_address,
    uint8_t counter)
{
  /* 1 byte||          8 bytes        ||    4 bytes    || 1 byte  || 2 bytes */
  /* flags || extended_source_address || frame_counter || sec_lvl || counter */
  
  nonce[0] = flags;
  memcpy(nonce + 1, extended_source_address, 8);
  nonce[9] = packetbuf_attr(buf, PACKETBUF_ATTR_FRAME_COUNTER_BYTES_2_3) >> 8;
  nonce[10] = packetbuf_attr(buf, PACKETBUF_ATTR_FRAME_COUNTER_BYTES_2_3) & 0xff;
  nonce[11] = packetbuf_attr(buf, PACKETBUF_ATTR_FRAME_COUNTER_BYTES_0_1) >> 8;
  nonce[12] = packetbuf_attr(buf, PACKETBUF_ATTR_FRAME_COUNTER_BYTES_0_1) & 0xff;
  nonce[13] = packetbuf_attr(buf, PACKETBUF_ATTR_SECURITY_LEVEL);
  nonce[14] = 0;
  nonce[15] = counter;
}
/*---------------------------------------------------------------------------*/
/* XORs the block m[pos] ... m[pos + 15] with K_{counter} */
static void
ctr_step(struct net_buf *buf, const uint8_t *extended_source_address,
    uint8_t pos,
    uint8_t *m_and_result,
    uint8_t m_len,
    uint8_t counter)
{
  uint8_t a[AES_128_BLOCK_SIZE];
  uint8_t i;
  
  set_nonce(buf, a, CCM_STAR_ENCRYPTION_FLAGS, extended_source_address, counter);
  AES_128.encrypt(a);
  
  for(i = 0; (pos + i < m_len) && (i < AES_128_BLOCK_SIZE); i++) {
    m_and_result[pos + i] ^= a[i];
  }
}
/*---------------------------------------------------------------------------*/
static void
mic(struct net_buf *buf, const uint8_t *extended_source_address,
    uint8_t *result,
    uint8_t mic_len)
{
  uint8_t x[AES_128_BLOCK_SIZE];
  uint8_t pos;
  uint8_t i;
  uint8_t a_len;
  uint8_t *a;
#if LLSEC802154_USES_ENCRYPTION
  uint8_t shall_encrypt;
  uint8_t m_len;
  uint8_t *m;
  
  shall_encrypt = packetbuf_attr(buf, PACKETBUF_ATTR_SECURITY_LEVEL) & (1 << 2);
  if(shall_encrypt) {
    a_len = packetbuf_hdrlen(buf);
    m_len = packetbuf_datalen(buf);
  } else {
    a_len = packetbuf_totlen(buf);
    m_len = 0;
  }
  set_nonce(x,
      CCM_STAR_AUTH_FLAGS(a_len, mic_len),
      extended_source_address,
      m_len);
#else /* LLSEC802154_USES_ENCRYPTION */
  a_len = packetbuf_totlen(buf);
  set_nonce(buf, x,
      CCM_STAR_AUTH_FLAGS(a_len, mic_len),
      extended_source_address,
      0);
#endif /* LLSEC802154_USES_ENCRYPTION */
  AES_128.encrypt(x);
  
  a = packetbuf_hdrptr(buf);
  if(a_len) {
    x[1] = x[1] ^ a_len;
    for(i = 2; (i - 2 < a_len) && (i < AES_128_BLOCK_SIZE); i++) {
      x[i] ^= a[i - 2];
    }
    
    AES_128.encrypt(x);
    
    pos = 14;
    while(pos < a_len) {
      for(i = 0; (pos + i < a_len) && (i < AES_128_BLOCK_SIZE); i++) {
        x[i] ^= a[pos + i];
      }
      pos += AES_128_BLOCK_SIZE;
      AES_128.encrypt(x);
    }
  }
  
#if LLSEC802154_USES_ENCRYPTION
  if(shall_encrypt) {
    m = a + a_len;
    pos = 0;
    while(pos < m_len) {
      for(i = 0; (pos + i < m_len) && (i < AES_128_BLOCK_SIZE); i++) {
        x[i] ^= m[pos + i];
      }
      pos += AES_128_BLOCK_SIZE;
      AES_128.encrypt(x);
    }
  }
#endif /* LLSEC802154_USES_ENCRYPTION */
  
  ctr_step(buf, extended_source_address, 0, x, AES_128_BLOCK_SIZE, 0);
  
  memcpy(result, x, mic_len);
}
/*---------------------------------------------------------------------------*/
static void
ctr(struct net_buf *buf, const uint8_t *extended_source_address)
{
  uint8_t m_len;
  uint8_t *m;
  uint8_t pos;
  uint8_t counter;
  
  m_len = packetbuf_datalen(buf);
  m = (uint8_t *) packetbuf_dataptr(buf);
  
  pos = 0;
  counter = 1;
  while(pos < m_len) {
    ctr_step(buf, extended_source_address, pos, m, m_len, counter++);
    pos += AES_128_BLOCK_SIZE;
  }
}
/*---------------------------------------------------------------------------*/
const struct ccm_star_driver ccm_star_driver = {
  mic,
  ctr
};
/*---------------------------------------------------------------------------*/

/** @} */
