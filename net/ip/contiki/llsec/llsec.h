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
 *         Link layer security header file.
 * \author
 *         Konrad Krentz <konrad.krentz@gmail.com>
 */

/**
 * \ingroup net
 * \defgroup llsec Link Layer Security
 * 
 * Layer for implementing link layer security.
 *         
 * NETSTACK_LLSEC sits in between NETSTACK_MAC and NETSTACK_NETWORK
 * protocols. All NETSTACK_MAC protocols invoke NETSTACK_LLSEC.input()
 * for incoming packets. Likewise, all NETSTACK_NETWORK protocols
 * invoke NETSTACK_LLSEC.send(...) for outgoing packets.
 * 
 * The bootstrap function of llsec_drivers can be used to defer the start
 * of upper layers so as to bootstrap pairwise keys. Only contiki-sky-main.c
 * supports this at the moment.
 * 
 * @{
 */

#ifndef LLSEC_H_
#define LLSEC_H_

#include "net/mac/mac.h"

typedef void (* llsec_on_bootstrapped_t)(void);

/**
 * The structure of a link layer security driver.
 */
struct llsec_driver {
  char *name;
  
  /** Bootstraps link layer security and thereafter starts upper layers. */
  void (* bootstrap)(llsec_on_bootstrapped_t on_bootstrapped);
  
  /** Secures outgoing frames before passing them to NETSTACK_MAC. */
  void (* send)(mac_callback_t sent_callback, void *ptr);
  
  /**
   * Once the NETSTACK_FRAMER wrote the headers, the LLSEC driver
   * can generate a MIC over the entire frame.
   * \return Returns != 0 <-> success
   */
  int (* on_frame_created)(void);
  
  /**
   * Decrypts incoming frames;
   * filters out injected or replayed frames.
   */
  void (* input)(void);
  
  /** Returns the security-related overhead per frame in bytes */
  uint8_t (* get_overhead)(void);
};

#endif /* LLSEC_H_ */

/** @} */
