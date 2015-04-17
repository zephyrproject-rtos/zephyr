/*
 * Copyright (c) 2012-2014, Thingsquare, http://www.thingsquare.com/.
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
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef UDP_SOCKET_H
#define UDP_SOCKET_H

#include "net/ip/uip.h"

struct udp_socket;

/**
 * \brief      A UDP socket callback function
 * \param c    A pointer to the struct udp_socket that received the data
 * \param ptr  An opaque pointer that was specified when the UDP socket was registered with udp_socket_register()
 * \param source_addr The IP address from which the datagram was sent
 * \param source_port The UDP port number, in host byte order, from which the datagram was sent
 * \param dest_addr The IP address that this datagram was sent to
 * \param dest_port The UDP port number, in host byte order, that the datagram was sent to
 * \param data A pointer to the data contents of the UDP datagram
 * \param datalen The length of the data being pointed to by the data pointer
 *
 *             Each UDP socket has a callback function that is
 *             registered as part of the call to
 *             udp_socket_register(). The callback function gets
 *             called every time a UDP packet is received.
 */
typedef void (* udp_socket_input_callback_t)(struct udp_socket *c,
                                             void *ptr,
                                             const uip_ipaddr_t *source_addr,
                                             uint16_t source_port,
                                             const uip_ipaddr_t *dest_addr,
                                             uint16_t dest_port,
                                             const uint8_t *data,
                                             uint16_t datalen);

struct udp_socket {
  udp_socket_input_callback_t input_callback;
  void *ptr;

  struct process *p;

  struct uip_udp_conn *udp_conn;

};

/**
 * \brief      Register a UDP socket
 * \param c    A pointer to the struct udp_socket that should be registered
 * \param ptr  An opaque pointer that will be passed to callbacks
 * \param receive_callback A function pointer to the callback function that will be called when data arrives
 * \retval -1  The registration failed
 * \retval 1   The registration succeeded
 *
 *             This function registers the UDP socket with the
 *             system. A UDP socket must be registered before any data
 *             can be sent or received over the socket.
 *
 *             The caller must allocate memory for the struct
 *             udp_socket that is to be registered.
 *
 *             A UDP socket can begin to receive data by calling
 *             udp_socket_bind().
 *
 */
int udp_socket_register(struct udp_socket *c,
                        void *ptr,
                        udp_socket_input_callback_t receive_callback);

/**
 * \brief      Bind a UDP socket to a local port
 * \param c    A pointer to the struct udp_socket that should be bound to a local port
 * \param local_port The UDP port number, in host byte order, to bind the UDP socket to
 * \retval -1  Binding the UDP socket to the local port failed
 * \retval 1   Binding the UDP socket to the local port succeeded
 *
 *             This function binds the UDP socket to a local port so
 *             that it will begin to receive data that arrives on the
 *             specified port. A UDP socket will receive data
 *             addressed to the specified port number on any IP
 *             address of the host.
 *
 *             A UDP socket that is bound to a local port will use
 *             this port number as a source port in outgoing UDP
 *             messages.
 *
 */
int udp_socket_bind(struct udp_socket *c,
                    uint16_t local_port);

/**
 * \brief      Bind a UDP socket to a remote address and port
 * \param c    A pointer to the struct udp_socket that should be connected
 * \param remote_addr The IP address of the remote host, or NULL if the UDP socket should only be connected to a specific port
 * \param remote_port The UDP port number, in host byte order, to which the UDP socket should be connected
 * \retval -1  Connecting the UDP socket failed
 * \retval 1   Connecting the UDP socket succeeded
 *
 *             This function connects the UDP socket to a specific
 *             remote port and optional remote IP address. When a UDP
 *             socket is connected to a remote port and address, it
 *             will only receive packets that are sent from the remote
 *             port and address. When sending data over a connected
 *             UDP socket, the data will be sent to the connected
 *             remote address.
 *
 *             A UDP socket can be connected to a remote port, but not
 *             a remote IP address, by providing a NULL parameter as
 *             the remote_addr parameter. This lets the UDP socket
 *             receive data from any IP address on the specified port.
 *
 */
int udp_socket_connect(struct udp_socket *c,
                       uip_ipaddr_t *remote_addr,
                       uint16_t remote_port);
/**
 * \brief      Send data on a UDP socket
 * \param c    A pointer to the struct udp_socket on which the data should be sent
 * \param data A pointer to the data that should be sent
 * \param datalen The length of the data to be sent
 * \return     The number of bytes sent, or -1 if an error occurred
 *
 *             This function sends data over a UDP socket. The UDP
 *             socket must have been connected to a remote address and
 *             port with udp_socket_connect().
 *
 */
int udp_socket_send(struct udp_socket *c,
                    const void *data, uint16_t datalen);

/**
 * \brief      Send data on a UDP socket to a specific address and port
 * \param c    A pointer to the struct udp_socket on which the data should be sent
 * \param data A pointer to the data that should be sent
 * \param datalen The length of the data to be sent
 * \param addr The IP address to which the data should be sent
 * \param port The UDP port number, in host byte order, to which the data should be sent
 * \return     The number of bytes sent, or -1 if an error occurred
 *
 *             This function sends data over a UDP socket to a
 *             specific address and port.
 *
 *             The UDP socket does not have to be connected to use
 *             this function.
 *
 */
int udp_socket_sendto(struct udp_socket *c,
                      const void *data, uint16_t datalen,
                      const uip_ipaddr_t *addr, uint16_t port);

/**
 * \brief      Close a UDP socket
 * \param c    A pointer to the struct udp_socket to be closed
 * \retval -1  If closing the UDP socket failed
 * \retval 1   If closing the UDP socket succeeded
 *
 *             This function closes a UDP socket that has previously
 *             been registered with udp_socket_register(). All
 *             registered UDP sockets must be closed before exiting
 *             the process that registered them, or undefined behavior
 *             may occur.
 *
 */
int udp_socket_close(struct udp_socket *c);

#endif /* UDP_SOCKET_H */
