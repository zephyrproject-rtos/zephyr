/*
 * Copyright (c) 2004, Swedish Institute of Computer Science.
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
 * Author: Adam Dunkels <adam@sics.se>
 *
 */

/**
 * \file
 *          Header for the Contiki/uIP interface.
 * \author  Adam Dunkels <adam@sics.se>
 * \author  Mathilde Durvy <mdurvy@cisco.com> (IPv6 related code)
 * \author  Julien Abeille <jabeille@cisco.com> (IPv6 related code)
 */

/**
 * \addtogroup uip
 * @{
 */

/**
 * \defgroup tcpip The Contiki/uIP interface
 * @{
 *
 * TCP/IP support in Contiki is implemented using the uIP TCP/IP
 * stack. For sending and receiving data, Contiki uses the functions
 * provided by the uIP module, but Contiki adds a set of functions for
 * connection management. The connection management functions make
 * sure that the uIP TCP/IP connections are connected to the correct
 * process.
 *
 * Contiki also includes an optional protosocket library that provides
 * an API similar to the BSD socket API.
 *
 * \sa \ref uip "The uIP TCP/IP stack"
 * \sa \ref psock "Protosockets library"
 *
 */

#ifndef TCPIP_H_
#define TCPIP_H_

#include "contiki.h"
#include "contiki/ip/uipaddr.h"

struct uip_conn;

struct tcpip_uipstate {
  struct process *p;
  void *state;
};

#define UIP_APPCALL(buf) tcpip_uipcall(buf)
#define UIP_UDP_APPCALL(buf) tcpip_uipcall(buf)
#define UIP_ICMP6_APPCALL tcpip_icmp6_call

/*#define UIP_APPSTATE_SIZE sizeof(struct tcpip_uipstate)*/

typedef struct tcpip_uipstate uip_udp_appstate_t;
typedef struct tcpip_uipstate uip_tcp_appstate_t;
typedef struct tcpip_uipstate uip_icmp6_appstate_t;
#include "contiki/ip/uip.h"
void tcpip_uipcall(struct net_buf *buf);

/**
 * \name TCP functions
 * @{
 */

/**
 * Attach a TCP connection to the current process
 *
 * This function attaches the current process to a TCP
 * connection. Each TCP connection must be attached to a process in
 * order for the process to be able to receive and send
 * data. Additionally, this function can add a pointer with connection
 * state to the connection.
 *
 * \param conn A pointer to the TCP connection.
 *
 * \param appstate An opaque pointer that will be passed to the
 * process whenever an event occurs on the connection.
 *
 */
CCIF void tcp_attach(struct uip_conn *conn,
		     void *appstate);
#define tcp_markconn(conn, appstate) tcp_attach(conn, appstate)

/**
 * Open a TCP port.
 *
 * This function opens a TCP port for listening. When a TCP connection
 * request occurs for the port, the process will be sent a tcpip_event
 * with the new connection request.
 *
 * \note Port numbers must always be given in network byte order. The
 * functions UIP_HTONS() and uip_htons() can be used to convert port numbers
 * from host byte order to network byte order.
 *
 * \param port The port number in network byte order.
 *
 */
CCIF void tcp_listen(uint16_t port, struct process *handler);

/**
 * Close a listening TCP port.
 *
 * This function closes a listening TCP port.
 *
 * \note Port numbers must always be given in network byte order. The
 * functions UIP_HTONS() and uip_htons() can be used to convert port numbers
 * from host byte order to network byte order.
 *
 * \param port The port number in network byte order.
 *
 */
CCIF void tcp_unlisten(uint16_t port, struct process *handler);

/**
 * Open a TCP connection to the specified IP address and port.
 *
 * This function opens a TCP connection to the specified port at the
 * host specified with an IP address. Additionally, an opaque pointer
 * can be attached to the connection. This pointer will be sent
 * together with uIP events to the process.
 *
 * \note The port number must be provided in network byte order so a
 * conversion with UIP_HTONS() usually is necessary.
 *
 * \note This function will only create the connection. The connection
 * is not opened directly. uIP will try to open the connection the
 * next time the uIP stack is scheduled by Contiki.
 *
 * \param ripaddr Pointer to the IP address of the remote host.
 * \param port Port number in network byte order.
 * \param appstate Pointer to application defined data.
 *
 * \return A pointer to the newly created connection, or NULL if
 * memory could not be allocated for the connection.
 *
 */
CCIF struct uip_conn *tcp_connect(const uip_ipaddr_t *ripaddr, uint16_t port,
				  void *appstate, struct process *process,
				  struct net_buf *buf);

/**
 * Cause a specified TCP connection to be polled.
 *
 * This function causes uIP to poll the specified TCP connection. The
 * function is used when the application has data that is to be sent
 * immediately and do not wish to wait for the periodic uIP polling
 * mechanism.
 *
 * \param conn A pointer to the TCP connection that should be polled.
 *
 */
void tcpip_poll_tcp(struct uip_conn *conn, struct net_buf *data_buf);

void tcpip_resend_syn(struct uip_conn *conn, struct net_buf *buf);

/** @} */

/**
 * \name UDP functions
 * @{
 */

struct uip_udp_conn;
/**
 * Attach the current process to a UDP connection
 *
 * This function attaches the current process to a UDP
 * connection. Each UDP connection must have a process attached to it
 * in order for the process to be able to receive and send data over
 * the connection. Additionally, this function can add a pointer with
 * connection state to the connection.
 *
 * \param conn A pointer to the UDP connection.
 *
 * \param appstate An opaque pointer that will be passed to the
 * process whenever an event occurs on the connection.
 *
 */
void udp_attach(struct uip_udp_conn *conn,
		void *appstate);
#define udp_markconn(conn, appstate) udp_attach(conn, appstate)

/**
 * Create a new UDP connection.
 *
 * This function creates a new UDP connection with the specified
 * remote endpoint.
 *
 * \note The port number must be provided in network byte order so a
 * conversion with UIP_HTONS() usually is necessary.
 *
 * \sa udp_bind()
 *
 * \param ripaddr Pointer to the IP address of the remote host.
 * \param port Port number in network byte order.
 * \param appstate Pointer to application defined data.
 *
 * \return A pointer to the newly created connection, or NULL if
 * memory could not be allocated for the connection.
 */
CCIF struct uip_udp_conn *udp_new(const uip_ipaddr_t *ripaddr, uint16_t port,
				  void *appstate);

/**
 * Create a new UDP broadcast connection.
 *
 * This function creates a new (link-local) broadcast UDP connection
 * to a specified port.
 *
 * \param port Port number in network byte order.
 * \param appstate Pointer to application defined data.
 *
 * \return A pointer to the newly created connection, or NULL if
 * memory could not be allocated for the connection.
 */
struct uip_udp_conn *udp_broadcast_new(uint16_t port, void *appstate);

/**
 * Bind a UDP connection to a local port.
 *
 * This function binds a UDP connection to a specified local port.
 *
 * When a connection is created with udp_new(), it gets a local port
 * number assigned automatically. If the application needs to bind the
 * connection to a specified local port, this function should be used.
 *
 * \note The port number must be provided in network byte order so a
 * conversion with UIP_HTONS() usually is necessary.
 *
 * \param conn A pointer to the UDP connection that is to be bound.
 * \param port The port number in network byte order to which to bind
 * the connection.
 */
#define udp_bind(conn, port) uip_udp_bind(conn, port)

/**
 * Unbind a UDP connection.
 *
 * This function unbinds a UDP connection.
 *
 * When a connection is created with udp_new(), it gets a local port
 * number assigned automatically. If the application needs to unbind the
 * connection from local port, this function should be used.
 *
 * \param conn A pointer to the UDP connection that is to be bound.
 */
#define udp_unbind(conn) uip_udp_remove(conn)

/**
 * Cause a specified UDP connection to be polled.
 *
 * This function causes uIP to poll the specified UDP connection. The
 * function is used when the application has data that is to be sent
 * immediately and do not wish to wait for the periodic uIP polling
 * mechanism.
 *
 * \param conn A pointer to the UDP connection that should be polled.
 *
 */
CCIF void tcpip_poll_udp(struct uip_udp_conn *conn);

/** @} */
 
/**
 * \name ICMPv6 functions
 * @{
 */

#if UIP_CONF_ICMP6

/**
 * The ICMP6 event.
 *
 * This event is posted to a process whenever a uIP ICMP event has occurred.
 */
CCIF extern process_event_t tcpip_icmp6_event;

/**
 * \brief register an ICMPv6 callback
 * \return 0 if success, 1 if failure (one application already registered)
 *
 * This function just registers a process to be polled when
 * an ICMPv6 message is received.
 * If no application registers, some ICMPv6 packets will be
 * processed by the "kernel" as usual (NS, NA, RS, RA, Echo request),
 * others will be dropped.
 * If an application registers here, it will be polled with a
 * process_post_synch every time an ICMPv6 packet is received.
 */
uint8_t icmp6_new(void *appstate);

/**
 * This function is called at reception of an ICMPv6 packet
 * If an application registered as an ICMPv6 listener (with
 * icmp6_new), it will be called through a process_post_synch()
 */
void tcpip_icmp6_call(uint8_t type);
#endif /*UIP_CONF_ICMP6*/

/** @} */
/**
 * The uIP event.
 *
 * This event is posted to a process whenever a uIP event has occurred.
 */
CCIF extern process_event_t tcpip_event;

/**
 * \name TCP/IP packet processing
 * @{
 */

/**
 * \brief      Deliver an incoming packet to the TCP/IP stack
 *
 *             This function is called by network device drivers to
 *             deliver an incoming packet to the TCP/IP stack. The
 *             incoming packet must be present in the uip_buf buffer,
 *             and the length of the packet must be in the global
 *             uip_len variable. If 0 is returned, then there was
 *             an error in the packet and it is discarded. The caller
 *             can then release the net_buf
 */
CCIF uint8_t tcpip_input(struct net_buf *buf);

/**
 * \brief Output packet to layer 2
 * The eventual parameter is the MAC address of the destination.
 */
uint8_t tcpip_output(struct net_buf *buf, const uip_lladdr_t *);
void tcpip_set_outputfunc(uint8_t (* f)(struct net_buf *buf, const uip_lladdr_t *));

/**
 * \brief This function does address resolution and then calls tcpip_output
 */
#if NETSTACK_CONF_WITH_IPV6
uint8_t tcpip_ipv6_output(struct net_buf *buf);
#endif

/**
 * \brief Is forwarding generally enabled?
 */
extern unsigned char tcpip_do_forwarding;

/*
 * Are we at the moment forwarding the contents of uip_buf[]?
 */
extern unsigned char tcpip_is_forwarding;


#define tcpip_set_forwarding(forwarding) tcpip_do_forwarding = (forwarding)

/** @} */

PROCESS_NAME(tcpip_process);

#endif /* TCPIP_H_ */

/** @} */
/** @} */
