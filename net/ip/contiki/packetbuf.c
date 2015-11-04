/*
 * Copyright (c) 2006, Swedish Institute of Computer Science.
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
 *         Rime buffer (packetbuf) management
 * \author
 *         Adam Dunkels <adam@sics.se>
 */

/**
 * \addtogroup packetbuf
 * @{
 */

#include <string.h>

#include <net/l2_buf.h>

#include "contiki-net.h"
#include "contiki/packetbuf.h"

#if NETSTACK_CONF_WITH_RIME
#include "net/rime/rime.h"
#endif

#if 0
/* Moved to l2_buf.h */
struct packetbuf_attr packetbuf_attrs[PACKETBUF_NUM_ATTRS];
struct packetbuf_addr packetbuf_addrs[PACKETBUF_NUM_ADDRS];


static uint16_t buflen, bufptr;
static uint8_t hdrptr;

/* The declarations below ensure that the packet buffer is aligned on
   an even 32-bit boundary. On some platforms (most notably the
   msp430 or OpenRISC), having a potentially misaligned packet buffer may lead to
   problems when accessing words. */
static uint32_t packetbuf_aligned[(PACKETBUF_SIZE + PACKETBUF_HDR_SIZE + 3) / 4];
static uint8_t *packetbuf = (uint8_t *)packetbuf_aligned;

static uint8_t *packetbufptr;
#endif

#define DEBUG 0
#define DEBUG_LEVEL DEBUG
//#define DEBUG DEBUG_NONE
#include "contiki/ip/uip-debug.h"

#if UIP_LOGGING
#include <stdio.h>
void uip_log(char *msg);
#define UIP_LOG(m) uip_log(m)
#else
#define UIP_LOG(m)
#endif

/*---------------------------------------------------------------------------*/
void
packetbuf_clear(struct net_buf *buf)
{
  uip_pkt_buflen(buf) = uip_pkt_bufptr(buf) = 0;
  uip_pkt_hdrptr(buf) = PACKETBUF_HDR_SIZE;

  uip_pkt_packetbufptr(buf) = &uip_pkt_packetbuf(buf)[PACKETBUF_HDR_SIZE];
  packetbuf_attr_clear(buf);
}
/*---------------------------------------------------------------------------*/
void
packetbuf_clear_hdr(struct net_buf *buf)
{
  uip_pkt_hdrptr(buf) = PACKETBUF_HDR_SIZE;
}
/*---------------------------------------------------------------------------*/
int
packetbuf_copyfrom(struct net_buf *buf, const void *from, uint16_t len)
{
  uint16_t l;

  packetbuf_clear(buf);
  l = len > PACKETBUF_SIZE? PACKETBUF_SIZE: len;
  memcpy(uip_pkt_packetbufptr(buf), from, l);
  uip_pkt_buflen(buf) = l;
  return l;
}
/*---------------------------------------------------------------------------*/
void
packetbuf_compact(struct net_buf *buf)
{
  int i, len;

  if(packetbuf_is_reference(buf)) {
    memcpy(&uip_pkt_packetbuf(buf)[PACKETBUF_HDR_SIZE], packetbuf_reference_ptr(buf),
	   packetbuf_datalen(buf));
  } else if(uip_pkt_bufptr(buf) > 0) {
    len = packetbuf_datalen(buf) + PACKETBUF_HDR_SIZE;
    for(i = PACKETBUF_HDR_SIZE; i < len; i++) {
      uip_pkt_packetbuf(buf)[i] = uip_pkt_packetbuf(buf)[uip_pkt_bufptr(buf) + i];
    }

    uip_pkt_bufptr(buf) = 0;
  }
}
/*---------------------------------------------------------------------------*/
int
packetbuf_copyto_hdr(struct net_buf *buf, uint8_t *to)
{
#if DEBUG_LEVEL > 0
  {
    int i;
    PRINTF("packetbuf %p\n", uip_pkt_packetbuf(buf));
    PRINTF("packetbuf_write_hdr: header (from %p, %d bytes):\n", uip_pkt_hdrptr(buf), PACKETBUF_HDR_SIZE);
    for(i = uip_pkt_hdrptr(buf); i < PACKETBUF_HDR_SIZE; ++i) {
      PRINTF("0x%02x, ", uip_pkt_packetbuf(buf)[i]);
    }
    PRINTF("\n");
  }
#endif /* DEBUG_LEVEL */
  memcpy(to, uip_pkt_packetbuf(buf) + uip_pkt_hdrptr(buf), PACKETBUF_HDR_SIZE - uip_pkt_hdrptr(buf));
  return PACKETBUF_HDR_SIZE - uip_pkt_hdrptr(buf);
}
/*---------------------------------------------------------------------------*/
int
packetbuf_copyto(struct net_buf *buf, void *to)
{
#if DEBUG_LEVEL > 0
  {
    int i;
    char buffer[1000];
    char *bufferptr = buffer;
    
    bufferptr[0] = 0;
    for(i = uip_pkt_hdrptr(buf); i < PACKETBUF_HDR_SIZE; ++i) {
      bufferptr += sprintf(bufferptr, "0x%02x, ", uip_pkt_packetbuf(buf)[i]);
    }
    PRINTF("packetbuf_write: header: %s\n", buffer);
    bufferptr = buffer;
    bufferptr[0] = 0;
    for(i = uip_pkt_bufptr(buf); i < uip_pkt_buflen(buf) + uip_pkt_bufptr(buf); ++i) {
      bufferptr += sprintf(bufferptr, "0x%02x, ", uip_pkt_packetbufptr(buf)[i]);
    }
    PRINTF("packetbuf_write: data: %s\n", buffer);
  }
#endif /* DEBUG_LEVEL */
  if(PACKETBUF_HDR_SIZE - uip_pkt_hdrptr(buf) + uip_pkt_buflen(buf) > PACKETBUF_SIZE) {
    /* Too large packet */
    return 0;
  }
  memcpy(to, uip_pkt_packetbuf(buf) + uip_pkt_hdrptr(buf), PACKETBUF_HDR_SIZE - uip_pkt_hdrptr(buf));
  memcpy((uint8_t *)to + PACKETBUF_HDR_SIZE - uip_pkt_hdrptr(buf), uip_pkt_packetbufptr(buf) + uip_pkt_bufptr(buf),
	 uip_pkt_buflen(buf));
  return PACKETBUF_HDR_SIZE - uip_pkt_hdrptr(buf) + uip_pkt_buflen(buf);
}
/*---------------------------------------------------------------------------*/
int
packetbuf_hdralloc(struct net_buf *buf, int size)
{
  if(uip_pkt_hdrptr(buf) >= size && packetbuf_totlen(buf) + size <= PACKETBUF_SIZE) {
    uip_pkt_hdrptr(buf) -= size;
    return 1;
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
void
packetbuf_hdr_remove(struct net_buf *buf, int size)
{
  uip_pkt_hdrptr(buf) += size;
}
/*---------------------------------------------------------------------------*/
int
packetbuf_hdrreduce(struct net_buf *buf, int size)
{
  if(uip_pkt_buflen(buf) < size) {
    return 0;
  }

  uip_pkt_bufptr(buf) += size;
  uip_pkt_buflen(buf) -= size;
  return 1;
}
/*---------------------------------------------------------------------------*/
void
packetbuf_set_datalen(struct net_buf *buf, uint16_t len)
{
  PRINTF("packetbuf_set_len: len %d\n", len);
  uip_pkt_buflen(buf) = len;
}
/*---------------------------------------------------------------------------*/
void *
packetbuf_dataptr(struct net_buf *buf)
{
  return (void *)(&uip_pkt_packetbuf(buf)[uip_pkt_bufptr(buf) + PACKETBUF_HDR_SIZE]);
}
/*---------------------------------------------------------------------------*/
void *
packetbuf_hdrptr(struct net_buf *buf)
{
  return (void *)(&uip_pkt_packetbuf(buf)[uip_pkt_hdrptr(buf)]);
}
/*---------------------------------------------------------------------------*/
void
packetbuf_reference(struct net_buf *buf, void *ptr, uint16_t len)
{
  packetbuf_clear(buf);
  uip_pkt_packetbufptr(buf) = ptr;
  uip_pkt_buflen(buf) = len;
}
/*---------------------------------------------------------------------------*/
int
packetbuf_is_reference(struct net_buf *buf)
{
  return uip_pkt_packetbufptr(buf) != &uip_pkt_packetbuf(buf)[PACKETBUF_HDR_SIZE];
}
/*---------------------------------------------------------------------------*/
void *
packetbuf_reference_ptr(struct net_buf *buf)
{
  return uip_pkt_packetbufptr(buf);
}
/*---------------------------------------------------------------------------*/
uint16_t
packetbuf_datalen(struct net_buf *buf)
{
  return uip_pkt_buflen(buf);
}
/*---------------------------------------------------------------------------*/
uint8_t
packetbuf_hdrlen(struct net_buf *buf)
{
  uint8_t hdrlen;
  
  hdrlen = PACKETBUF_HDR_SIZE - uip_pkt_hdrptr(buf);
  if(hdrlen) {
    /* outbound packet */
    return hdrlen;
  } else {
    /* inbound packet */
    return uip_pkt_bufptr(buf);
  }
}
/*---------------------------------------------------------------------------*/
uint16_t
packetbuf_totlen(struct net_buf *buf)
{
  return packetbuf_hdrlen(buf) + packetbuf_datalen(buf);
}
/*---------------------------------------------------------------------------*/
void
packetbuf_attr_clear(struct net_buf *buf)
{
  int i;
  for(i = 0; i < PACKETBUF_NUM_ATTRS; ++i) {
    uip_pkt_packetbuf_attrs(buf)[i].val = 0;
  }
  for(i = 0; i < PACKETBUF_NUM_ADDRS; ++i) {
    linkaddr_copy(&uip_pkt_packetbuf_addrs(buf)[i].addr, &linkaddr_null);
  }
}
/*---------------------------------------------------------------------------*/
void
packetbuf_attr_copyto(struct net_buf *buf, struct packetbuf_attr *attrs,
		    struct packetbuf_addr *addrs)
{
  memcpy(attrs, uip_pkt_packetbuf_attrs(buf), sizeof(uip_pkt_packetbuf_attrs(buf)));
  memcpy(addrs, uip_pkt_packetbuf_addrs(buf), sizeof(uip_pkt_packetbuf_addrs(buf)));
}
/*---------------------------------------------------------------------------*/
void
packetbuf_attr_copyfrom(struct net_buf *buf, struct packetbuf_attr *attrs,
		      struct packetbuf_addr *addrs)
{
  memcpy(uip_pkt_packetbuf_attrs(buf), attrs, sizeof(uip_pkt_packetbuf_attrs(buf)));
  memcpy(uip_pkt_packetbuf_addrs(buf), addrs, sizeof(uip_pkt_packetbuf_addrs(buf)));
}
/*---------------------------------------------------------------------------*/
#if !PACKETBUF_CONF_ATTRS_INLINE
int
packetbuf_set_attr(struct net_buf *buf, uint8_t type, const packetbuf_attr_t val)
{
/*   uip_pkt_packetbuf_attrs(buf)[type].type = type; */
  uip_pkt_packetbuf_attrs(buf)[type].val = val;
  return 1;
}
/*---------------------------------------------------------------------------*/
packetbuf_attr_t
packetbuf_attr(struct net_buf *buf, uint8_t type)
{
  return uip_pkt_packetbuf_attrs(buf)[type].val;
}
/*---------------------------------------------------------------------------*/
int
packetbuf_set_addr(struct net_buf *buf, uint8_t type, const linkaddr_t *addr)
{
/*   uip_pkt_packetbuf_addrs(buf)[type - PACKETBUF_ADDR_FIRST].type = type; */
  linkaddr_copy(&uip_pkt_packetbuf_addrs(buf)[type - PACKETBUF_ADDR_FIRST].addr, addr);
  return 1;
}
/*---------------------------------------------------------------------------*/
const linkaddr_t *
packetbuf_addr(struct net_buf *buf, uint8_t type)
{
  return &uip_pkt_packetbuf_addrs(buf)[type - PACKETBUF_ADDR_FIRST].addr;
}
/*---------------------------------------------------------------------------*/
#endif /* PACKETBUF_CONF_ATTRS_INLINE */
int
packetbuf_holds_broadcast(struct net_buf *buf)
{
  return linkaddr_cmp(&uip_pkt_packetbuf_addrs(buf)[PACKETBUF_ADDR_RECEIVER - PACKETBUF_ADDR_FIRST].addr, &linkaddr_null);
}
/*---------------------------------------------------------------------------*/

/** @} */
