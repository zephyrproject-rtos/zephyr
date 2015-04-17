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

#ifndef UIP_NAMESERVER_H_
#define UIP_NAMESERVER_H_

/**
 * \name General
 * @{
 */
/** \brief Number of Nameservers to keep */
#ifndef UIP_CONF_NAMESERVER_POOL_SIZE
#define UIP_NAMESERVER_POOL_SIZE 1
#else /* UIP_CONF_NAMESERVER_POOL_SIZE */
#define UIP_NAMESERVER_POOL_SIZE UIP_CONF_NAMESERVER_POOL_SIZE
#endif /* UIP_CONF_NAMESERVER_POOL_SIZE */
/** \brief Infinite Lifetime indicator */
#define UIP_NAMESERVER_INFINITE_LIFETIME 0xFFFFFFFF
/** @} */

/**
 * \name Nameserver maintenance
 * @{
 */
/**
 * \brief Insert or update a nameserver into/from the pool
 *
 * The list is kept according to the RFC6106, which indicates that new entries
 * will replace old ones (with lower lifetime) and existing entries will update
 * their lifetimes.
 *
 * \param nameserver   Pointer to the nameserver ip address
 * \param lifetime     Life time of the given address. Minimum is 0, which is
 *                     considered to remove an entry. Maximum is 0xFFFFFFFF which
 *                     is considered infinite.
 */
void uip_nameserver_update(uip_ipaddr_t *nameserver, uint32_t lifetime);

/**
 * \brief Get a Nameserver ip address given in RA
 *
 * \param num   The number of the nameserver to obtain, starting at 0 and going
 *              up to the pool size.
 */
uip_ipaddr_t *uip_nameserver_get(uint8_t num);

/**
 * \brief Get next expiration time
 *
 * The least expiration time is returned
 */
uint32_t uip_nameserver_next_expiration(void);

/**
 * \brief Get the number of recorded name servers
 */
uint16_t uip_nameserver_count(void);
/** @} */

#endif /* UIP_NAMESERVER_H_ */
/** @} */
