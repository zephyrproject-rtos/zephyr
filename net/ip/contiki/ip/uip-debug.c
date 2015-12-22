/*
 * Copyright (c) 2010, Swedish Institute of Computer Science.
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
 */

/**
 * \file
 *         A set of debugging tools
 * \author
 *         Nicolas Tsiftes <nvt@sics.se>
 *         Niclas Finne <nfi@sics.se>
 *         Joakim Eriksson <joakime@sics.se>
 */

#include "contiki/ip/uip-debug.h"

/*---------------------------------------------------------------------------*/
void
uip_debug_ipaddr_print(const uip_ipaddr_t *addr)
{
  if(addr == NULL || addr->u8 == NULL) {
    printf("(NULL IP addr)");
    return;
  }
#if NETSTACK_CONF_WITH_IPV6
  uint16_t a;
  unsigned int i;
  int f;
  for(i = 0, f = 0; i < sizeof(uip_ipaddr_t); i += 2) {
    a = (addr->u8[i] << 8) + addr->u8[i + 1];
    if(a == 0 && f >= 0) {
      if(f++ == 0) {
        PRINTA("::");
      }
    } else {
      if(f > 0) {
        f = -1;
      } else if(i > 0) {
        PRINTA(":");
      }
      PRINTA("%x", a);
    }
  }
#else /* NETSTACK_CONF_WITH_IPV6 */
  PRINTA("%u.%u.%u.%u", addr->u8[0], addr->u8[1], addr->u8[2], addr->u8[3]);
#endif /* NETSTACK_CONF_WITH_IPV6 */
}
/*---------------------------------------------------------------------------*/
void
uip_debug_lladdr_print(const uip_lladdr_t *addr)
{
  unsigned int i;
  for(i = 0; i < sizeof(uip_lladdr_t); i++) {
    if(i > 0) {
      PRINTA(":");
    }
    PRINTA("%x", addr->addr[i]);
  }
}
/*---------------------------------------------------------------------------*/
void
uip_debug_hex_dump(const unsigned char *buffer, int len)
{
  int i = 0;

  while (len--) {
    if(*buffer <= 0xf)
      PRINTA("0%X ", *buffer++);
    else
      PRINTA("%X ", *buffer++);

    i++;
    if (i % 8 == 0) {
      if (i % 16 == 0) {
	PRINTA("\n");
      } else {
	PRINTA(" ");
      }
    }
  }
}
/*---------------------------------------------------------------------------*/
