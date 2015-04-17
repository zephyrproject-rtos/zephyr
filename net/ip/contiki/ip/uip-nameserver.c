/**
 * \addtogroup uip6
 * @{
 */

/**
 * \file
 *         uIP Name Server interface
 * \author Víctor Ariño <victor.arino@tado.com>
 */

/*
 * Copyright (c) 2014, tado° GmbH.
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

#include "contiki.h"
#include "contiki-net.h"

#include "lib/list.h"
#include "lib/memb.h"

#include <string.h>
/** \brief Nameserver record */
typedef struct uip_nameserver_record {
  struct uip_nameserver_record *next;
  uip_ipaddr_t ip;
  uint32_t added;
  uint32_t lifetime;
} uip_nameserver_record;

/** \brief Initialization flag */
static uint8_t initialized = 0;

/** \name List and memory block
 * @{
 */
#if UIP_NAMESERVER_POOL_SIZE > 1
LIST(dns);
MEMB(dnsmemb, uip_nameserver_record, UIP_NAMESERVER_POOL_SIZE);
#else /* UIP_NAMESERVER_POOL_SIZE > 1 */
static uip_ipaddr_t serveraddr;
static uint32_t serverlifetime;
#endif /* UIP_NAMESERVER_POOL_SIZE > 1 */
/** @} */

/** \brief Expiration time in seconds */
#define DNS_EXPIRATION(r) \
  (((UIP_NAMESERVER_INFINITE_LIFETIME - r->added) <= r->lifetime) ? \
   UIP_NAMESERVER_INFINITE_LIFETIME : r->added + r->lifetime)
/*----------------------------------------------------------------------------*/
/**
 * Initialize the module variables
 */
#if UIP_NAMESERVER_POOL_SIZE > 1
static CC_INLINE void
init(void)
{
  list_init(dns);
  memb_init(&dnsmemb);
  initialized = 1;
}
#endif /* UIP_NAMESERVER_POOL_SIZE > 1 */
/*----------------------------------------------------------------------------*/
void
uip_nameserver_update(uip_ipaddr_t *nameserver, uint32_t lifetime)
{
#if UIP_NAMESERVER_POOL_SIZE > 1
  register uip_nameserver_record *e;

  if(initialized == 0) {
    init();
  }

  for(e = list_head(dns); e != NULL; e = list_item_next(e)) {
    if(uip_ipaddr_cmp(&e->ip, nameserver)) {
      break;
      /* RFC6106: In case there's no more space, the new servers should replace
       *          the the eldest ones */
    }
  }
  
  if(e == NULL) {
    if((e = memb_alloc(&dnsmemb)) != NULL) {
      list_add(dns, e);
    } else {
      uip_nameserver_record *p;
      for(e = list_head(dns), p = list_head(dns); p != NULL;
          p = list_item_next(p)) {
        if(DNS_EXPIRATION(p) < DNS_EXPIRATION(e)) {
          e = p;
        }
      }
    }
  }

  /* RFC6106: In case the entry is existing the expiration time must be
   *          updated. Otherwise, new entries are added. */
  if(e != NULL) {
    if(lifetime == 0) {
      memb_free(&dnsmemb, e);
      list_remove(dns, e);
    } else {
      e->added = clock_seconds();
      e->lifetime = lifetime;
      uip_ipaddr_copy(&e->ip, nameserver);
    }
  }
#else /* UIP_NAMESERVER_POOL_SIZE > 1 */
  uip_ipaddr_copy(&serveraddr, nameserver);
  serverlifetime = lifetime;
#endif /* UIP_NAMESERVER_POOL_SIZE > 1 */
}
/*----------------------------------------------------------------------------*/
#if UIP_NAMESERVER_POOL_SIZE > 1
/**
 * Purge expired records
 */
static void
purge(void)
{
  register uip_nameserver_record *e = NULL;
  uint32_t time = clock_seconds();
  for(e = list_head(dns); e != NULL; e = list_item_next(e)) {
    if(DNS_EXPIRATION(e) < time) {
      list_remove(dns, e);
      memb_free(&dnsmemb, e);
      e = list_head(dns);
    }
  }
}
#endif /* UIP_NAMESERVER_POOL_SIZE > 1 */
/*----------------------------------------------------------------------------*/
uip_ipaddr_t *
uip_nameserver_get(uint8_t num)
{
#if UIP_NAMESERVER_POOL_SIZE > 1
  uint8_t i;
  uip_nameserver_record *e = NULL;

  if(initialized == 0) {
    return NULL;
  }
  purge();
  for(i = 1, e = list_head(dns); e != NULL && i <= num;
      i++, e = list_item_next(e)) {
  }

  if(e != NULL) {
    return &e->ip;
  }
  return NULL;
#else /* UIP_NAMESERVER_POOL_SIZE > 1 */
  if(num > 0) {
    return NULL;
  }
  return &serveraddr;
#endif /* UIP_NAMESERVER_POOL_SIZE > 1 */
}
/*----------------------------------------------------------------------------*/
uint32_t
uip_nameserver_next_expiration(void)
{
#if UIP_NAMESERVER_POOL_SIZE > 1
  register uip_nameserver_record *e = NULL;
  uint32_t exp = UIP_NAMESERVER_INFINITE_LIFETIME;
  uint32_t t;

  if(initialized == 0 || list_length(dns) == 0) {
    return 0;
  }
  purge();
  for(e = list_head(dns); e != NULL; e = list_item_next(e)) {
    t = DNS_EXPIRATION(e);
    if(t < exp) {
      exp = t;
    }
  }

  return exp;
#else /* UIP_NAMESERVER_POOL_SIZE > 1 */
  return serverlifetime;
#endif /* UIP_NAMESERVER_POOL_SIZE > 1 */
}
/*----------------------------------------------------------------------------*/
uint16_t
uip_nameserver_count(void)
{
#if UIP_NAMESERVER_POOL_SIZE > 1
  if(initialized == 0) {
    return 0;
  }
  return list_length(dns);
#else /* UIP_NAMESERVER_POOL_SIZE > 1 */
#if NETSTACK_CONF_WITH_IPV6
  if(uip_is_addr_unspecified(&serveraddr)) {
#else /* NETSTACK_CONF_WITH_IPV6 */
  if(uip_ipaddr_cmp(&serveraddr, &uip_all_zeroes_addr)) {
#endif /* NETSTACK_CONF_WITH_IPV6 */
    return 0;
  } else {
    return 1;
  }
#endif /* UIP_NAMESERVER_POOL_SIZE > 1 */
}
/*----------------------------------------------------------------------------*/
/** @} */
