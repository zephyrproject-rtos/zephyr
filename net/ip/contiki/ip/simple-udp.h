/*
 * Copyright (c) 2011, Swedish Institute of Computer Science.
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
 */

/** 
 * \file
 *         Header file for the simple-udp module.
 * \author
 *         Adam Dunkels <adam@sics.se>
 *
 */

/**
 * \addtogroup uip
 * @{
 */


/**
 * \defgroup simple-udp A simple UDP API
 *
 * The default Contiki UDP API is difficult to use. The simple-udp
 * module provides a significantly simpler API.
 *
 * @{
 */

#ifndef SIMPLE_UDP_H
#define SIMPLE_UDP_H

#include "net/ip/uip.h"

struct simple_udp_connection;

/** Simple UDP Callback function type. */
typedef void (* simple_udp_callback)(struct simple_udp_connection *c,
                                     const uip_ipaddr_t *source_addr,
                                     uint16_t source_port,
                                     const uip_ipaddr_t *dest_addr,
                                     uint16_t dest_port,
                                     const uint8_t *data, uint16_t datalen);

/** Simple UDP connection */
struct simple_udp_connection {
  struct simple_udp_connection *next;
  uip_ipaddr_t remote_addr;
  uint16_t remote_port, local_port;
  simple_udp_callback receive_callback;
  struct uip_udp_conn *udp_conn;
  struct process *client_process;
};

/**
 * \brief      Register a UDP connection
 * \param c    A pointer to a struct simple_udp_connection
 * \param local_port The local UDP port in host byte order
 * \param remote_addr The remote IP address
 * \param remote_port The remote UDP port in host byte order
 * \param receive_callback A pointer to a function of to be called for incoming packets
 * \retval 0   If no UDP connection could be allocated
 * \retval 1   If the connection was successfully allocated
 *
 *     This function registers a UDP connection and attaches a
 *     callback function to it. The callback function will be
 *     called for incoming packets. The local UDP port can be
 *     set to 0 to indicate that an ephemeral UDP port should
 *     be allocated. The remote IP address can be NULL, to
 *     indicate that packets from any IP address should be
 *     accepted.
 *
 */
int simple_udp_register(struct simple_udp_connection *c,
                        uint16_t local_port,
                        uip_ipaddr_t *remote_addr,
                        uint16_t remote_port,
                        simple_udp_callback receive_callback);

/**
 * \brief      Send a UDP packet
 * \param c    A pointer to a struct simple_udp_connection
 * \param data A pointer to the data to be sent
 * \param datalen The length of the data
 *
 *     This function sends a UDP packet. The packet will be
 *     sent to the IP address and with the UDP ports that were
 *     specified when the connection was registered with
 *     simple_udp_register().
 *
 * \sa simple_udp_sendto()
 */
int simple_udp_send(struct simple_udp_connection *c,
                    const void *data, uint16_t datalen);

/**
 * \brief      Send a UDP packet to a specified IP address
 * \param c    A pointer to a struct simple_udp_connection
 * \param data A pointer to the data to be sent
 * \param datalen The length of the data
 * \param to   The IP address of the receiver
 *
 *     This function sends a UDP packet to a specified IP
 *     address. The packet will be sent with the UDP ports
 *     that were specified when the connection was registered
 *     with simple_udp_register().
 *
 * \sa simple_udp_send()
 */
int simple_udp_sendto(struct simple_udp_connection *c,
                      const void *data, uint16_t datalen,
                      const uip_ipaddr_t *to);

/**
 * \brief      Send a UDP packet to a specified IP address and UDP port
 * \param c    A pointer to a struct simple_udp_connection
 * \param data A pointer to the data to be sent
 * \param datalen The length of the data
 * \param to   The IP address of the receiver
 * \param to_port   The UDP port of the receiver, in host byte order
 *
 *     This function sends a UDP packet to a specified IP
 *     address and UDP port. The packet will be sent with the
 *     UDP ports that were specified when the connection was
 *     registered with simple_udp_register().
 *
 * \sa simple_udp_sendto()
 */
int simple_udp_sendto_port(struct simple_udp_connection *c,
			   const void *data, uint16_t datalen,
			   const uip_ipaddr_t *to, uint16_t to_port);

void simple_udp_init(void);

#endif /* SIMPLE_UDP_H */

/** @} */
/** @} */
