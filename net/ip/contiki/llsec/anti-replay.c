/*
 * Copyright (c) 2014, Hasso-Plattner-Institut.
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
 *         Protects against replay attacks by comparing with the last
 *         unicast or broadcast frame counter of the sender.
 * \author
 *         Konrad Krentz <konrad.krentz@gmail.com>
 */

/**
 * \addtogroup llsec802154
 * @{
 */

#include "net/llsec/anti-replay.h"
#include "net/packetbuf.h"

/* This node's current frame counter value */
static uint32_t counter;

/*---------------------------------------------------------------------------*/
void
anti_replay_set_counter(void)
{
  frame802154_frame_counter_t reordered_counter;
  
  reordered_counter.u32 = LLSEC802154_HTONL(++counter);
  
  packetbuf_set_attr(PACKETBUF_ATTR_FRAME_COUNTER_BYTES_0_1, reordered_counter.u16[0]);
  packetbuf_set_attr(PACKETBUF_ATTR_FRAME_COUNTER_BYTES_2_3, reordered_counter.u16[1]);
}
/*---------------------------------------------------------------------------*/
uint32_t
anti_replay_get_counter(void)
{
  frame802154_frame_counter_t disordered_counter;
  
  disordered_counter.u16[0] = packetbuf_attr(PACKETBUF_ATTR_FRAME_COUNTER_BYTES_0_1);
  disordered_counter.u16[1] = packetbuf_attr(PACKETBUF_ATTR_FRAME_COUNTER_BYTES_2_3);
  
  return LLSEC802154_HTONL(disordered_counter.u32); 
}
/*---------------------------------------------------------------------------*/
void
anti_replay_init_info(struct anti_replay_info *info)
{
  info->last_broadcast_counter
      = info->last_unicast_counter
      = anti_replay_get_counter();
}
/*---------------------------------------------------------------------------*/
int
anti_replay_was_replayed(struct anti_replay_info *info)
{
  uint32_t received_counter;
  
  received_counter = anti_replay_get_counter();
  
  if(packetbuf_holds_broadcast()) {
    /* broadcast */
    if(received_counter <= info->last_broadcast_counter) {
      return 1;
    } else {
      info->last_broadcast_counter = received_counter;
      return 0;
    }
  } else {
    /* unicast */
    if(received_counter <= info->last_unicast_counter) {
      return 1;
    } else {
      info->last_unicast_counter = received_counter;
      return 0;
    }
  }
}
/*---------------------------------------------------------------------------*/

/** @} */
