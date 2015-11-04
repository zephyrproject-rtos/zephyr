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
 *         CCM* header file.
 * \author
 *         Konrad Krentz <konrad.krentz@gmail.com>
 */

/**
 * \addtogroup llsec802154
 * @{
 */

#ifndef CCM_STAR_H_
#define CCM_STAR_H_

#include <net/l2_buf.h>

#include "contiki.h"
#include "contiki/mac/frame802154.h"

/* see RFC 3610 */
#define CCM_STAR_AUTH_FLAGS(Adata, M) ((Adata ? (1 << 6) : 0) | (((M - 2) >> 1) << 3) | 1)
#define CCM_STAR_ENCRYPTION_FLAGS     1

#ifdef CCM_STAR_CONF
#define CCM_STAR CCM_STAR_CONF
#else /* CCM_STAR_CONF */
#define CCM_STAR ccm_star_driver
#endif /* CCM_STAR_CONF */

/**
 * Structure of CCM* drivers.
 */
struct ccm_star_driver {
  
   /**
    * \brief         Generates a MIC over the frame in the packetbuf.
    * \param result  The generated MIC will be put here
    * \param mic_len  <= 16; set to LLSEC802154_MIC_LENGTH to be compliant
    */
  void (* mic)(struct net_buf *buf, const uint8_t *extended_source_address,
      uint8_t *result,
      uint8_t mic_len);
  
  /**
   * \brief XORs the frame in the packetbuf with the key stream.
   */
  void (* ctr)(struct net_buf *buf, const uint8_t *extended_source_address);
};

extern const struct ccm_star_driver CCM_STAR;

#endif /* CCM_STAR_H_ */

/** @} */
