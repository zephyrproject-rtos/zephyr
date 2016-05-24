/*
 * Copyright (c) 2001-2003, Adam Dunkels.
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
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * This file is part of the uIP TCP/IP stack.
 *
 *
 */

/**
 * \addtogroup uip
 * @{
 */

/**
 * \file
 * Header file for the uIP TCP/IP stack.
 * \author  Adam Dunkels <adam@dunkels.com>
 * \author  Julien Abeille <jabeille@cisco.com> (IPv6 related code)
 * \author  Mathilde Durvy <mdurvy@cisco.com> (IPv6 related code)
 *
 * The uIP TCP/IP stack header file contains definitions for a number
 * of C macros that are used by uIP programs as well as internal uIP
 * structures, TCP/IP header structures and function declarations.
 *
 */

#include <stdint.h>
#include <string.h>

#ifndef UIP_H_
#define UIP_H_

struct net_buf;

#include "contiki/ip/uipopt.h"
#include "contiki/ip/uipaddr.h"
#include "contiki/ip/tcpip.h"

/* Header sizes. */
#if NETSTACK_CONF_WITH_IPV6
#define UIP_IPH_LEN    40
#define UIP_FRAGH_LEN  8
#else /* NETSTACK_CONF_WITH_IPV6 */
#define UIP_IPH_LEN    20    /* Size of IP header */
#endif /* NETSTACK_CONF_WITH_IPV6 */

#define UIP_UDPH_LEN    8    /* Size of UDP header */
#define UIP_TCPH_LEN   20    /* Size of TCP header */
#define UIP_ICMPH_LEN   4    /* Size of ICMP header */

#define UIP_IPUDPH_LEN (UIP_UDPH_LEN + UIP_IPH_LEN)    /* Size of IP +
                        * UDP
							   * header */
#define UIP_IPTCPH_LEN (UIP_TCPH_LEN + UIP_IPH_LEN)    /* Size of IP +
							   * TCP
							   * header */
#define UIP_TCPIP_HLEN UIP_IPTCPH_LEN
#define UIP_IPICMPH_LEN (UIP_IPH_LEN + UIP_ICMPH_LEN) /* size of ICMP
                                                         + IP header */
#define UIP_LLIPH_LEN (UIP_LLH_LEN + UIP_IPH_LEN)    /* size of L2
                                                        + IP header */
#if NETSTACK_CONF_WITH_IPV6
/**
 * The sums below are quite used in ND. When used for uip_buf, we
 * include link layer length when used for uip_len, we do not, hence
 * we need values with and without LLH_LEN we do not use capital
 * letters as these values are variable
 */
#define uip_l2_l3_hdr_len(buf) (UIP_LLH_LEN + UIP_IPH_LEN + uip_ext_len(buf))
#define uip_l2_l3_icmp_hdr_len(buf) (UIP_LLH_LEN + UIP_IPH_LEN + uip_ext_len(buf) + UIP_ICMPH_LEN)
#define uip_l3_hdr_len(buf) (UIP_IPH_LEN + uip_ext_len(buf))
#define uip_l3_icmp_hdr_len(buf) (UIP_IPH_LEN + uip_ext_len(buf) + UIP_ICMPH_LEN)
#endif /*NETSTACK_CONF_WITH_IPV6*/


/*---------------------------------------------------------------------------*/
/* First, the functions that should be called from the
 * system. Initialization, the periodic timer, and incoming packets are
 * handled by the following three functions.
 */
/**
 * \defgroup uipconffunc uIP configuration functions
 * @{
 *
 * The uIP configuration functions are used for setting run-time
 * parameters in uIP such as IP addresses.
 */

/**
 * Set the IP address of this host.
 *
 * The IP address is represented as a 4-byte array where the first
 * octet of the IP address is put in the first member of the 4-byte
 * array.
 *
 * Example:
 \code

 uip_ipaddr_t addr;

 uip_ipaddr(&addr, 192,168,1,2);
 uip_sethostaddr(&addr);

 \endcode
 * \param addr A pointer to an IP address of type uip_ipaddr_t;
 *
 * \sa uip_ipaddr()
 *
 * \hideinitializer
 */
#define uip_sethostaddr(addr) uip_ipaddr_copy(&uip_hostaddr, (addr))

/**
 * Get the IP address of this host.
 *
 * The IP address is represented as a 4-byte array where the first
 * octet of the IP address is put in the first member of the 4-byte
 * array.
 *
 * Example:
 \code
 uip_ipaddr_t hostaddr;

 uip_gethostaddr(&hostaddr);
 \endcode
 * \param addr A pointer to a uip_ipaddr_t variable that will be
 * filled in with the currently configured IP address.
 *
 * \hideinitializer
 */
#define uip_gethostaddr(addr) uip_ipaddr_copy((addr), &uip_hostaddr)

/**
 * Set the default router's IP address.
 *
 * \param addr A pointer to a uip_ipaddr_t variable containing the IP
 * address of the default router.
 *
 * \sa uip_ipaddr()
 *
 * \hideinitializer
 */
#define uip_setdraddr(addr) uip_ipaddr_copy(&uip_draddr, (addr))

/**
 * Set the netmask.
 *
 * \param addr A pointer to a uip_ipaddr_t variable containing the IP
 * address of the netmask.
 *
 * \sa uip_ipaddr()
 *
 * \hideinitializer
 */
#define uip_setnetmask(addr) uip_ipaddr_copy(&uip_netmask, (addr))


/**
 * Get the default router's IP address.
 *
 * \param addr A pointer to a uip_ipaddr_t variable that will be
 * filled in with the IP address of the default router.
 *
 * \hideinitializer
 */
#define uip_getdraddr(addr) uip_ipaddr_copy((addr), &uip_draddr)

/**
 * Get the netmask.
 *
 * \param addr A pointer to a uip_ipaddr_t variable that will be
 * filled in with the value of the netmask.
 *
 * \hideinitializer
 */
#define uip_getnetmask(addr) uip_ipaddr_copy((addr), &uip_netmask)

/** @} */

/**
 * \defgroup uipinit uIP initialization functions
 * @{
 *
 * The uIP initialization functions are used for booting uIP.
 */

/**
 * uIP initialization function.
 *
 * This function should be called at boot up to initilize the uIP
 * TCP/IP stack.
 */
void uip_init(void);

/**
 * uIP initialization function.
 *
 * This function may be used at boot time to set the initial ip_id.
 */
void uip_setipid(uint16_t id);

/** @} */

/**
 * \defgroup uipdevfunc uIP device driver functions
 * @{
 *
 * These functions are used by a network device driver for interacting
 * with uIP.
 */

/**
 * Process an incoming packet.
 *
 * This function should be called when the device driver has received
 * a packet from the network. The packet from the device driver must
 * be present in the uip_buf buffer, and the length of the packet
 * should be placed in the uip_len variable.
 *
 * When the function returns, there may be an outbound packet placed
 * in the uip_buf packet buffer. If so, the uip_len variable is set to
 * the length of the packet. If no packet is to be sent out, the
 * uip_len variable is set to 0.
 *
 * The usual way of calling the function is presented by the source
 * code below.
 \code
 uip_len = devicedriver_poll();
 if(uip_len > 0) {
 uip_input();
 if(uip_len > 0) {
 devicedriver_send();
 }
 }
 \endcode
 *
 * \note If you are writing a uIP device driver that needs ARP
 * (Address Resolution Protocol), e.g., when running uIP over
 * Ethernet, you will need to call the uIP ARP code before calling
 * this function:
 \code
 #define BUF ((struct uip_eth_hdr *)&uip_buf[0])
 uip_len = ethernet_devicedrver_poll();
 if(uip_len > 0) {
 if(BUF->type == UIP_HTONS(UIP_ETHTYPE_IP)) {
 uip_arp_ipin();
 uip_input();
 if(uip_len > 0) {
 uip_arp_out();
 ethernet_devicedriver_send();
 }
 } else if(BUF->type == UIP_HTONS(UIP_ETHTYPE_ARP)) {
 uip_arp_arpin();
 if(uip_len > 0) {
 ethernet_devicedriver_send();
 }
 }
 \endcode
 *
 * \hideinitializer
 */
#define uip_input(buf)        uip_process(&buf, UIP_DATA)

/**
 * Periodic processing for a connection identified by its number.
 *
 * This function does the necessary periodic processing (timers,
 * polling) for a uIP TCP connection, and should be called when the
 * periodic uIP timer goes off. It should be called for every
 * connection, regardless of whether they are open of closed.
 *
 * When the function returns, it may have an outbound packet waiting
 * for service in the uIP packet buffer, and if so the uip_len
 * variable is set to a value larger than zero. The device driver
 * should be called to send out the packet.
 *
 * The usual way of calling the function is through a for() loop like
 * this:
 \code
 for(i = 0; i < UIP_CONNS; ++i) {
 uip_periodic(i);
 if(uip_len > 0) {
 devicedriver_send();
 }
 }
 \endcode
 *
 * \note If you are writing a uIP device driver that needs ARP
 * (Address Resolution Protocol), e.g., when running uIP over
 * Ethernet, you will need to call the uip_arp_out() function before
 * calling the device driver:
 \code
 for(i = 0; i < UIP_CONNS; ++i) {
 uip_periodic(i);
 if(uip_len > 0) {
 uip_arp_out();
 ethernet_devicedriver_send();
 }
 }
 \endcode
 *
 * \param conn The number of the connection which is to be periodically polled.
 *
 * \hideinitializer
 */
#if UIP_TCP
#define uip_periodic(buf, conn) do { uip_set_conn(buf) = &uip_conns[conn]; \
    uip_process(&buf, UIP_TIMER); } while (0)

/**
 * Macro to determine whether a specific uIP connection is active
 *
 * \param conn The connection's number
 * \retval 0 Connection closed
 */
#define uip_conn_active(conn) (uip_conns[conn].tcpstateflags != UIP_CLOSED)

/**
 * Perform periodic processing for a connection identified by a pointer
 * to its structure.
 *
 * Same as uip_periodic() but takes a pointer to the actual uip_conn
 * struct instead of an integer as its argument. This function can be
 * used to force periodic processing of a specific connection.
 *
 * \param conn A pointer to the uip_conn struct for the connection to
 * be processed.
 *
 * \hideinitializer
 */
#define uip_periodic_conn(buf, conn) do { uip_set_conn(buf) = conn;	\
    uip_process(&buf, UIP_TIMER); } while (0)

/**
 * Request that a particular connection should be polled.
 *
 * Similar to uip_periodic_conn() but does not perform any timer
 * processing. The application is polled for new data.
 *
 * \param conn A pointer to the uip_conn struct for the connection to
 * be processed.
 *
 * \hideinitializer
 */
#define uip_poll_conn(buf, conn) do { uip_set_conn(buf) = conn;	\
    uip_process(&buf, UIP_POLL_REQUEST); } while (0)

#endif /* UIP_TCP */

#if UIP_UDP
/**
 * Periodic processing for a UDP connection identified by its number.
 *
 * This function is essentially the same as uip_periodic(), but for
 * UDP connections. It is called in a similar fashion as the
 * uip_periodic() function:
 \code
 for(i = 0; i < UIP_UDP_CONNS; i++) {
 uip_udp_periodic(i);
 if(uip_len > 0) {
 devicedriver_send();
 }
 }
 \endcode
 *
 * \note As for the uip_periodic() function, special care has to be
 * taken when using uIP together with ARP and Ethernet:
 \code
 for(i = 0; i < UIP_UDP_CONNS; i++) {
 uip_udp_periodic(i);
 if(uip_len > 0) {
 uip_arp_out();
 ethernet_devicedriver_send();
 }
 }
 \endcode
 *
 * \param conn The number of the UDP connection to be processed.
 *
 * \hideinitializer
 */
#define uip_udp_periodic(conn) do { uip_udp_conn = &uip_udp_conns[conn]; \
    uip_process(UIP_UDP_TIMER); } while(0)

/**
 * Periodic processing for a UDP connection identified by a pointer to
 * its structure.
 *
 * Same as uip_udp_periodic() but takes a pointer to the actual
 * uip_conn struct instead of an integer as its argument. This
 * function can be used to force periodic processing of a specific
 * connection.
 *
 * \param conn A pointer to the uip_udp_conn struct for the connection
 * to be processed.
 *
 * \hideinitializer
 */
#define uip_udp_periodic_conn(buf, conn) do { uip_set_udp_conn(buf) = conn; \
		uip_process(&buf, UIP_UDP_TIMER); } while(0)
#endif /* UIP_UDP */

/** \brief Abandon the reassembly of the current packet */
void uip_reass_over(void);

/**
 * The uIP packet buffer.
 *
 * The uip_aligned_buf array is used to hold incoming and outgoing
 * packets. The device driver should place incoming data into this
 * buffer. When sending data, the device driver should read the link
 * level headers and the TCP/IP headers from this buffer. The size of
 * the link level headers is configured by the UIP_LLH_LEN define.
 *
 * \note The application data need not be placed in this buffer, so
 * the device driver must read it from the place pointed to by the
 * uip_appdata pointer as illustrated by the following example:
 \code
 void
 devicedriver_send(void)
 {
 hwsend(&uip_buf[0], UIP_LLH_LEN);
 if(uip_len <= UIP_LLH_LEN + UIP_TCPIP_HLEN) {
 hwsend(&uip_buf[UIP_LLH_LEN], uip_len - UIP_LLH_LEN);
 } else {
 hwsend(&uip_buf[UIP_LLH_LEN], UIP_TCPIP_HLEN);
 hwsend(uip_appdata, uip_len - UIP_TCPIP_HLEN - UIP_LLH_LEN);
 }
 }
 \endcode
 */

typedef union {
  uint32_t u32[(UIP_BUFSIZE + 3) / 4];
  uint8_t u8[UIP_BUFSIZE];
} uip_buf_t;

#if 0
/* Moved to net_buf */
CCIF extern uip_buf_t uip_aligned_buf;

/** Macro to access uip_aligned_buf as an array of bytes */
#define uip_buf (uip_aligned_buf.u8)
#endif

/** @} */

/*---------------------------------------------------------------------------*/
/* Functions that are used by the uIP application program. Opening and
 * closing connections, sending and receiving data, etc. is all
 * handled by the functions below.
 */
/**
 * \defgroup uipappfunc uIP application functions
 * @{
 *
 * Functions used by an application running on top of uIP.
 */

/**
 * Start listening to the specified port.
 *
 * \note Since this function expects the port number in network byte
 * order, a conversion using UIP_HTONS() or uip_htons() is necessary.
 *
 \code
 uip_listen(UIP_HTONS(80));
 \endcode
 *
 * \param port A 16-bit port number in network byte order.
 */
void uip_listen(uint16_t port);

/**
 * Stop listening to the specified port.
 *
 * \note Since this function expects the port number in network byte
 * order, a conversion using UIP_HTONS() or uip_htons() is necessary.
 *
 \code
 uip_unlisten(UIP_HTONS(80));
 \endcode
 *
 * \param port A 16-bit port number in network byte order.
 */
void uip_unlisten(uint16_t port);

/**
 * Connect to a remote host using TCP.
 *
 * This function is used to start a new connection to the specified
 * port on the specified host. It allocates a new connection identifier,
 * sets the connection to the SYN_SENT state and sets the
 * retransmission timer to 0. This will cause a TCP SYN segment to be
 * sent out the next time this connection is periodically processed,
 * which usually is done within 0.5 seconds after the call to
 * uip_connect().
 *
 * \note This function is available only if support for active open
 * has been configured by defining UIP_ACTIVE_OPEN to 1 in uipopt.h.
 *
 * \note Since this function requires the port number to be in network
 * byte order, a conversion using UIP_HTONS() or uip_htons() is necessary.
 *
 \code
 uip_ipaddr_t ipaddr;

 uip_ipaddr(&ipaddr, 192,168,1,2);
 uip_connect(&ipaddr, UIP_HTONS(80));
 \endcode
 *
 * \param ripaddr The IP address of the remote host.
 *
 * \param port A 16-bit port number in network byte order.
 *
 * \return A pointer to the uIP connection identifier for the new connection,
 * or NULL if no connection could be allocated.
 *
 */
struct uip_conn *uip_connect(const uip_ipaddr_t *ripaddr, uint16_t port);



/**
 * \internal
 *
 * Check if a connection has outstanding (i.e., unacknowledged) data.
 *
 * \param conn A pointer to the uip_conn structure for the connection.
 *
 * \hideinitializer
 */
#define uip_outstanding(conn) ((conn)->len)

/**
 * Send data on the current connection.
 *
 * This function is used to send out a single segment of TCP
 * data. Only applications that have been invoked by uIP for event
 * processing can send data.
 *
 * The amount of data that actually is sent out after a call to this
 * function is determined by the maximum amount of data TCP allows. uIP
 * will automatically crop the data so that only the appropriate
 * amount of data is sent. The function uip_mss() can be used to query
 * uIP for the amount of data that actually will be sent.
 *
 * \note This function does not guarantee that the sent data will
 * arrive at the destination. If the data is lost in the network, the
 * application will be invoked with the uip_rexmit() event being
 * set. The application will then have to resend the data using this
 * function.
 *
 * \param data A pointer to the data which is to be sent.
 *
 * \param len The maximum amount of data bytes to be sent.
 *
 * \hideinitializer
 */
#if UIP_TCP
CCIF void uip_send(struct net_buf *buf, const void *data, int len);
#endif

#if UIP_UDP
CCIF void uip_send_udp(struct net_buf *buf, const void *data, int len);
#endif

/**
 * The length of any incoming data that is currently available (if available)
 * in the uip_appdata buffer.
 *
 * The test function uip_data() must first be used to check if there
 * is any data available at all.
 *
 * \hideinitializer
 */
#define uip_datalen(buf)       uip_len(buf)

/**
 * The length of any out-of-band data (urgent data) that has arrived
 * on the connection.
 *
 * \note The configuration parameter UIP_URGDATA must be set for this
 * function to be enabled.
 *
 * \hideinitializer
 */
#define uip_urgdatalen()    uip_urglen

/**
 * Close the current connection.
 *
 * This function will close the current connection in a nice way.
 *
 * \hideinitializer
 */
#define uip_close()         (uip_flags = UIP_CLOSE)

/**
 * Abort the current connection.
 *
 * This function will abort (reset) the current connection, and is
 * usually used when an error has occurred that prevents using the
 * uip_close() function.
 *
 * \hideinitializer
 */
#define uip_abort()         (uip_flags = UIP_ABORT)

/**
 * Tell the sending host to stop sending data.
 *
 * This function will close our receiver's window so that we stop
 * receiving data for the current connection.
 *
 * \hideinitializer
 */
#define uip_stop()          (uip_conn->tcpstateflags |= UIP_STOPPED)

/**
 * Find out if the current connection has been previously stopped with
 * uip_stop().
 *
 * \hideinitializer
 */
#define uip_stopped(conn)   ((conn)->tcpstateflags & UIP_STOPPED)

/**
 * Restart the current connection, if is has previously been stopped
 * with uip_stop().
 *
 * This function will open the receiver's window again so that we
 * start receiving data for the current connection.
 *
 * \hideinitializer
 */
#define uip_restart(buf)    do { uip_flags(buf) |= UIP_NEWDATA;	\
    uip_conn->tcpstateflags &= ~UIP_STOPPED;                    \
  } while(0)


/* uIP tests that can be made to determine in what state the current
   connection is, and what the application function should do. */

/**
 * Is the current connection a UDP connection?
 *
 * This function checks whether the current connection is a UDP connection.
 *
 * \hideinitializer
 *
 */
#define uip_udpconnection() (uip_conn == NULL)

/**
 * Is new incoming data available?
 *
 * Will reduce to non-zero if there is new data for the application
 * present at the uip_appdata pointer. The size of the data is
 * available through the uip_len variable.
 *
 * \hideinitializer
 */
#define uip_newdata(buf)   (uip_flags(buf) & UIP_NEWDATA)

/**
 * Has previously sent data been acknowledged?
 *
 * Will reduce to non-zero if the previously sent data has been
 * acknowledged by the remote host. This means that the application
 * can send new data.
 *
 * \hideinitializer
 */
#define uip_acked(buf)   (uip_flags(buf) & UIP_ACKDATA)

/**
 * Has the connection just been connected?
 *
 * Reduces to non-zero if the current connection has been connected to
 * a remote host. This will happen both if the connection has been
 * actively opened (with uip_connect()) or passively opened (with
 * uip_listen()).
 *
 * \hideinitializer
 */
#define uip_connected(buf) (uip_flags(buf) & UIP_CONNECTED)

/**
 * Has the connection been closed by the other end?
 *
 * Is non-zero if the connection has been closed by the remote
 * host. The application may then do the necessary clean-ups.
 *
 * \hideinitializer
 */
#define uip_closed(buf)    (uip_flags(buf) & UIP_CLOSE)

/**
 * Has the connection been aborted by the other end?
 *
 * Non-zero if the current connection has been aborted (reset) by the
 * remote host.
 *
 * \hideinitializer
 */
#define uip_aborted(buf)    (uip_flags(buf) & UIP_ABORT)

/**
 * Has the connection timed out?
 *
 * Non-zero if the current connection has been aborted due to too many
 * retransmissions.
 *
 * \hideinitializer
 */
#define uip_timedout(buf)    (uip_flags(buf) & UIP_TIMEDOUT)

/**
 * Do we need to retransmit previously data?
 *
 * Reduces to non-zero if the previously sent data has been lost in
 * the network, and the application should retransmit it. The
 * application should send the exact same data as it did the last
 * time, using the uip_send() function.
 *
 * \hideinitializer
 */
#define uip_rexmit(buf)     (uip_flags(buf) & UIP_REXMIT)

/**
 * Is the connection being polled by uIP?
 *
 * Is non-zero if the reason the application is invoked is that the
 * current connection has been idle for a while and should be
 * polled.
 *
 * The polling event can be used for sending data without having to
 * wait for the remote host to send data.
 *
 * \hideinitializer
 */
#define uip_poll(buf)       (uip_flags(buf) & UIP_POLL)

/**
 * Get the initial maximum segment size (MSS) of the current
 * connection.
 *
 * \hideinitializer
 */
#define uip_initialmss(buf)             (uip_conn(buf)->initialmss)

/**
 * Get the current maximum segment size that can be sent on the current
 * connection.
 *
 * The current maximum segment size that can be sent on the
 * connection is computed from the receiver's window and the MSS of
 * the connection (which also is available by calling
 * uip_initialmss()).
 *
 * \hideinitializer
 */
#define uip_mss(buf)             (uip_conn(buf)->mss)

/**
 * Set up a new UDP connection.
 *
 * This function sets up a new UDP connection. The function will
 * automatically allocate an unused local port for the new
 * connection. However, another port can be chosen by using the
 * uip_udp_bind() call, after the uip_udp_new() function has been
 * called.
 *
 * Example:
 \code
 uip_ipaddr_t addr;
 struct uip_udp_conn *c;

 uip_ipaddr(&addr, 192,168,2,1);
 c = uip_udp_new(&addr, UIP_HTONS(12345));
 if(c != NULL) {
 uip_udp_bind(c, UIP_HTONS(12344));
 }
 \endcode
 * \param ripaddr The IP address of the remote host.
 *
 * \param rport The remote port number in network byte order.
 *
 * \return The uip_udp_conn structure for the new connection, or NULL
 * if no connection could be allocated.
 */
struct uip_udp_conn *uip_udp_new(const uip_ipaddr_t *ripaddr, uint16_t rport);

/**
 * Remove a UDP connection.
 *
 * \param conn A pointer to the uip_udp_conn structure for the connection.
 *
 * \hideinitializer
 */
#define uip_udp_remove(conn) (conn)->lport = 0

/**
 * Bind a UDP connection to a local port.
 *
 * \param conn A pointer to the uip_udp_conn structure for the
 * connection.
 *
 * \param port The local port number, in network byte order.
 *
 * \hideinitializer
 */
#define uip_udp_bind(conn, port) (conn)->lport = port

/**
 * Send a UDP datagram of length len on the current connection.
 *
 * This function can only be called in response to a UDP event (poll
 * or newdata). The data must be present in the uip_buf buffer, at the
 * place pointed to by the uip_appdata pointer.
 *
 * \param len The length of the data in the uip_buf buffer.
 *
 * \hideinitializer
 */
#define uip_udp_send(buf, len) uip_send((char *)uip_appdata(buf), len)

/** @} */

/* uIP convenience and converting functions. */

/**
 * \defgroup uipconvfunc uIP conversion functions
 * @{
 *
 * These functions can be used for converting between different data
 * formats used by uIP.
 */

/**
 * Convert an IP address to four bytes separated by commas.
 *
 * Example:
 \code
 uip_ipaddr_t ipaddr;
 printf("ipaddr=%d.%d.%d.%d\n", uip_ipaddr_to_quad(&ipaddr));
 \endcode
 *
 * \param a A pointer to a uip_ipaddr_t.
 * \hideinitializer
 */
#define uip_ipaddr_to_quad(a) (a)->u8[0],(a)->u8[1],(a)->u8[2],(a)->u8[3]

/**
 * Construct an IP address from four bytes.
 *
 * This function constructs an IP address of the type that uIP handles
 * internally from four bytes. The function is handy for specifying IP
 * addresses to use with e.g. the uip_connect() function.
 *
 * Example:
 \code
 uip_ipaddr_t ipaddr;
 struct uip_conn *c;

 uip_ipaddr(&ipaddr, 192,168,1,2);
 c = uip_connect(&ipaddr, UIP_HTONS(80));
 \endcode
 *
 * \param addr A pointer to a uip_ipaddr_t variable that will be
 * filled in with the IP address.
 *
 * \param addr0 The first octet of the IP address.
 * \param addr1 The second octet of the IP address.
 * \param addr2 The third octet of the IP address.
 * \param addr3 The forth octet of the IP address.
 *
 * \hideinitializer
 */
#define uip_ipaddr(addr, addr0,addr1,addr2,addr3) do {  \
    (addr)->u8[0] = addr0;                              \
    (addr)->u8[1] = addr1;                              \
    (addr)->u8[2] = addr2;                              \
    (addr)->u8[3] = addr3;                              \
  } while(0)

/**
 * Construct an IPv6 address from eight 16-bit words.
 *
 * This function constructs an IPv6 address.
 *
 * \hideinitializer
 */
#define uip_ip6addr(addr, addr0,addr1,addr2,addr3,addr4,addr5,addr6,addr7) do { \
    (addr)->u16[0] = UIP_HTONS(addr0);                                      \
    (addr)->u16[1] = UIP_HTONS(addr1);                                      \
    (addr)->u16[2] = UIP_HTONS(addr2);                                      \
    (addr)->u16[3] = UIP_HTONS(addr3);                                      \
    (addr)->u16[4] = UIP_HTONS(addr4);                                      \
    (addr)->u16[5] = UIP_HTONS(addr5);                                      \
    (addr)->u16[6] = UIP_HTONS(addr6);                                      \
    (addr)->u16[7] = UIP_HTONS(addr7);                                      \
  } while(0)

/**
 * Construct an IPv6 address from sixteen 8-bit words.
 *
 * This function constructs an IPv6 address.
 *
 * \hideinitializer
 */
#define uip_ip6addr_u8(addr, addr0,addr1,addr2,addr3,addr4,addr5,addr6,addr7,addr8,addr9,addr10,addr11,addr12,addr13,addr14,addr15) do { \
    (addr)->u8[0] = addr0;                                       \
    (addr)->u8[1] = addr1;                                       \
    (addr)->u8[2] = addr2;                                       \
    (addr)->u8[3] = addr3;                                       \
    (addr)->u8[4] = addr4;                                       \
    (addr)->u8[5] = addr5;                                       \
    (addr)->u8[6] = addr6;                                       \
    (addr)->u8[7] = addr7;                                       \
    (addr)->u8[8] = addr8;                                       \
    (addr)->u8[9] = addr9;                                       \
    (addr)->u8[10] = addr10;                                     \
    (addr)->u8[11] = addr11;                                     \
    (addr)->u8[12] = addr12;                                     \
    (addr)->u8[13] = addr13;                                     \
    (addr)->u8[14] = addr14;                                     \
    (addr)->u8[15] = addr15;                                     \
  } while(0)


/**
 * Copy an IP address from one place to another.
 *
 * Copies an IP address from one place to another.
 *
 * Example:
 \code
 uip_ipaddr_t ipaddr1, ipaddr2;

 uip_ipaddr(&ipaddr1, 192,16,1,2);
 uip_ipaddr_copy(&ipaddr2, &ipaddr1);
 \endcode
 *
 * \param dest The destination for the copy.
 * \param src The source from where to copy.
 *
 * \hideinitializer
 */
#ifndef uip_ipaddr_copy
#define uip_ipaddr_copy(dest, src) (*(dest) = *(src))
#endif
#ifndef uip_ip4addr_copy
#define uip_ip4addr_copy(dest, src) (*((uip_ip4addr_t *)dest) = *((uip_ip4addr_t *)src))
#endif
#ifndef uip_ip6addr_copy
#define uip_ip6addr_copy(dest, src) (*((uip_ip6addr_t *)dest) = *((uip_ip6addr_t *)src))
#endif

/**
 * Compare two IP addresses
 *
 * Compares two IP addresses.
 *
 * Example:
 \code
 uip_ipaddr_t ipaddr1, ipaddr2;

 uip_ipaddr(&ipaddr1, 192,16,1,2);
 if(uip_ipaddr_cmp(&ipaddr2, &ipaddr1)) {
  printf("They are the same");
 }
 \endcode
 *
 * \param addr1 The first IP address.
 * \param addr2 The second IP address.
 *
 * \hideinitializer
 */
#define uip_ip4addr_cmp(addr1, addr2) ((addr1)->u16[0] == (addr2)->u16[0] && \
				      (addr1)->u16[1] == (addr2)->u16[1])
#define uip_ip6addr_cmp(addr1, addr2) (memcmp(addr1, addr2, sizeof(uip_ip6addr_t)) == 0)

#if NETSTACK_CONF_WITH_IPV6
#define uip_ipaddr_cmp(addr1, addr2) uip_ip6addr_cmp(addr1, addr2)
#else /* NETSTACK_CONF_WITH_IPV6 */
#define uip_ipaddr_cmp(addr1, addr2) uip_ip4addr_cmp(addr1, addr2)
#endif /* NETSTACK_CONF_WITH_IPV6 */

/**
 * Compare two IP addresses with netmasks
 *
 * Compares two IP addresses with netmasks. The masks are used to mask
 * out the bits that are to be compared.
 *
 * Example:
 \code
 uip_ipaddr_t ipaddr1, ipaddr2, mask;

 uip_ipaddr(&mask, 255,255,255,0);
 uip_ipaddr(&ipaddr1, 192,16,1,2);
 uip_ipaddr(&ipaddr2, 192,16,1,3);
 if(uip_ipaddr_maskcmp(&ipaddr1, &ipaddr2, &mask)) {
 printf("They are the same");
 }
 \endcode
 *
 * \param addr1 The first IP address.
 * \param addr2 The second IP address.
 * \param mask The netmask.
 *
 * \hideinitializer
 */

#define uip_ipaddr_maskcmp(addr1, addr2, mask)          \
  (((((uint16_t *)addr1)[0] & ((uint16_t *)mask)[0]) ==       \
    (((uint16_t *)addr2)[0] & ((uint16_t *)mask)[0])) &&      \
   ((((uint16_t *)addr1)[1] & ((uint16_t *)mask)[1]) ==       \
    (((uint16_t *)addr2)[1] & ((uint16_t *)mask)[1])))

#define uip_ipaddr_prefixcmp(addr1, addr2, length) \
	uip_ipaddr_prefixcmp2((uint8_t *)(addr1), (uint8_t *)(addr2), length)

static inline bool uip_ipaddr_prefixcmp2(uint8_t *addr1,
					 uint8_t *addr2,
					 uint8_t length)
{
	uint8_t bits = 128 - length;
	uint8_t bytes = bits / 8;
	uint8_t remain = bits % 8;

	if (length > 128) {
		return false;
	}

	if (memcmp(addr1, addr2, 16 - bytes)) {
		return false;
	}

	return ((addr1[16 - bytes] & ((8 - remain) << 8))
		==
		(addr2[16 - bytes] & ((8 - remain) << 8)));
}

/*
 * Check if an address is a broadcast address for a network.
 *
 * Checks if an address is the broadcast address for a network. The
 * network is defined by an IP address that is on the network and the
 * network's netmask.
 *
 * \param addr The IP address.
 * \param netaddr The network's IP address.
 * \param netmask The network's netmask.
 *
 * \hideinitializer
 */
/*#define uip_ipaddr_isbroadcast(addr, netaddr, netmask)
  ((uip_ipaddr_t *)(addr)).u16 & ((uip_ipaddr_t *)(addr)).u16*/



/**
 * Mask out the network part of an IP address.
 *
 * Masks out the network part of an IP address, given the address and
 * the netmask.
 *
 * Example:
 \code
 uip_ipaddr_t ipaddr1, ipaddr2, netmask;

 uip_ipaddr(&ipaddr1, 192,16,1,2);
 uip_ipaddr(&netmask, 255,255,255,0);
 uip_ipaddr_mask(&ipaddr2, &ipaddr1, &netmask);
 \endcode
 *
 * In the example above, the variable "ipaddr2" will contain the IP
 * address 192.168.1.0.
 *
 * \param dest Where the result is to be placed.
 * \param src The IP address.
 * \param mask The netmask.
 *
 * \hideinitializer
 */
#define uip_ipaddr_mask(dest, src, mask) do {                           \
    ((uint16_t *)dest)[0] = ((uint16_t *)src)[0] & ((uint16_t *)mask)[0];        \
    ((uint16_t *)dest)[1] = ((uint16_t *)src)[1] & ((uint16_t *)mask)[1];        \
  } while(0)

/**
 * Pick the first octet of an IP address.
 *
 * Picks out the first octet of an IP address.
 *
 * Example:
 \code
 uip_ipaddr_t ipaddr;
 uint8_t octet;

 uip_ipaddr(&ipaddr, 1,2,3,4);
 octet = uip_ipaddr1(&ipaddr);
 \endcode
 *
 * In the example above, the variable "octet" will contain the value 1.
 *
 * \hideinitializer
 */
#define uip_ipaddr1(addr) ((addr)->u8[0])

/**
 * Pick the second octet of an IP address.
 *
 * Picks out the second octet of an IP address.
 *
 * Example:
 \code
 uip_ipaddr_t ipaddr;
 uint8_t octet;

 uip_ipaddr(&ipaddr, 1,2,3,4);
 octet = uip_ipaddr2(&ipaddr);
 \endcode
 *
 * In the example above, the variable "octet" will contain the value 2.
 *
 * \hideinitializer
 */
#define uip_ipaddr2(addr) ((addr)->u8[1])

/**
 * Pick the third octet of an IP address.
 *
 * Picks out the third octet of an IP address.
 *
 * Example:
 \code
 uip_ipaddr_t ipaddr;
 uint8_t octet;

 uip_ipaddr(&ipaddr, 1,2,3,4);
 octet = uip_ipaddr3(&ipaddr);
 \endcode
 *
 * In the example above, the variable "octet" will contain the value 3.
 *
 * \hideinitializer
 */
#define uip_ipaddr3(addr) ((addr)->u8[2])

/**
 * Pick the fourth octet of an IP address.
 *
 * Picks out the fourth octet of an IP address.
 *
 * Example:
 \code
 uip_ipaddr_t ipaddr;
 uint8_t octet;

 uip_ipaddr(&ipaddr, 1,2,3,4);
 octet = uip_ipaddr4(&ipaddr);
 \endcode
 *
 * In the example above, the variable "octet" will contain the value 4.
 *
 * \hideinitializer
 */
#define uip_ipaddr4(addr) ((addr)->u8[3])

/**
 * Convert 16-bit quantity from host byte order to network byte order.
 *
 * This macro is primarily used for converting constants from host
 * byte order to network byte order. For converting variables to
 * network byte order, use the uip_htons() function instead.
 *
 * \hideinitializer
 */
#ifndef UIP_HTONS
#   if UIP_BYTE_ORDER == UIP_BIG_ENDIAN
#      define UIP_HTONS(n) (n)
#      define UIP_HTONL(n) (n)
#   else /* UIP_BYTE_ORDER == UIP_BIG_ENDIAN */
#      define UIP_HTONS(n) (uint16_t)((((uint16_t) (n)) << 8) | (((uint16_t) (n)) >> 8))
#      define UIP_HTONL(n) (((uint32_t)UIP_HTONS(n) << 16) | UIP_HTONS((uint32_t)(n) >> 16))
#   endif /* UIP_BYTE_ORDER == UIP_BIG_ENDIAN */
#else
#error "UIP_HTONS already defined!"
#endif /* UIP_HTONS */

/**
 * Convert a 16-bit quantity from host byte order to network byte order.
 *
 * This function is primarily used for converting variables from host
 * byte order to network byte order. For converting constants to
 * network byte order, use the UIP_HTONS() macro instead.
 */
#ifndef uip_htons
CCIF uint16_t uip_htons(uint16_t val);
#endif /* uip_htons */
#ifndef uip_ntohs
#define uip_ntohs uip_htons
#endif

#ifndef uip_htonl
CCIF uint32_t uip_htonl(uint32_t val);
#endif /* uip_htonl */
#ifndef uip_ntohl
#define uip_ntohl uip_htonl
#endif

/** @} */

/**
 * Pointer to the application data in the packet buffer.
 *
 * This pointer points to the application data when the application is
 * called. If the application wishes to send data, the application may
 * use this space to write the data into before calling uip_send().
 */
#if 0
/* part of net_buf */
CCIF extern void *uip_appdata;
#endif

#if UIP_URGDATA > 0
/* uint8_t *uip_urgdata:
 *
 * This pointer points to any urgent data that has been received. Only
 * present if compiled with support for urgent data (UIP_URGDATA).
 */
extern void *uip_urgdata;
#endif /* UIP_URGDATA > 0 */


/**
 * \defgroup uipdrivervars Variables used in uIP device drivers
 * @{
 *
 * uIP has a few global variables that are used in device drivers for
 * uIP.
 */

/**
 * The length of the packet in the uip_buf buffer.
 *
 * The global variable uip_len holds the length of the packet in the
 * uip_buf buffer.
 *
 * When the network device driver calls the uIP input function,
 * uip_len should be set to the length of the packet in the uip_buf
 * buffer.
 *
 * When sending packets, the device driver should use the contents of
 * the uip_len variable to determine the length of the outgoing
 * packet.
 *
 */
#if 0
/* No longer used as len is part of net_buf */
CCIF extern uint16_t uip_len;
#endif

/**
 * The length of the extension headers
 */
#if 0
/* No longer used as len is part of net_buf */
extern uint8_t uip_ext_len;
#endif
/** @} */

#if UIP_URGDATA > 0
extern uint16_t uip_urglen, uip_surglen;
#endif /* UIP_URGDATA > 0 */

/**
 * Representation of a uIP TCP connection.
 *
 * The uip_conn structure is used for identifying a connection. All
 * but one field in the structure are to be considered read-only by an
 * application. The only exception is the appstate field whose purpose
 * is to let the application store application-specific state (e.g.,
 * file pointers) for the connection. The type of this field is
 * configured in the "uipopt.h" header file.
 */
struct uip_conn {
  uip_ipaddr_t ripaddr;   /**< The IP address of the remote host. */

  uint16_t lport;        /**< The local TCP port, in network byte order. */
  uint16_t rport;        /**< The local remote TCP port, in network byte
			 order. */

  uint8_t rcv_nxt[4];    /**< The sequence number that we expect to
			 receive next. */
  uint8_t snd_nxt[4];    /**< The sequence number that was last sent by
                         us. */
  uint16_t len;          /**< Length of the data that was previously sent. */
  uint16_t mss;          /**< Current maximum segment size for the
			 connection. */
  uint16_t initialmss;   /**< Initial maximum segment size for the
			 connection. */
  uint8_t sa;            /**< Retransmission time-out calculation state
			 variable. */
  uint8_t sv;            /**< Retransmission time-out calculation state
			 variable. */
  uint8_t rto;           /**< Retransmission time-out. */
  uint8_t tcpstateflags; /**< TCP state and flags. */
  uint8_t timer;         /**< The retransmission timer. */
  uint8_t nrtx;          /**< The number of retransmissions for the last
			 segment sent. */

  /** The application state. */
  uip_tcp_appstate_t appstate;

  /* buffer holding the data to this connection */
  struct net_buf *buf;

#if UIP_ACTIVE_OPEN
  /* re-send SYN in active open connection */
  struct ctimer retransmit_timer;
#endif
};


/**
 * Pointer to the current TCP connection.
 *
 * The uip_conn pointer can be used to access the current TCP
 * connection.
 */
#if 0
/* Moved to net_buf */
CCIF extern struct uip_conn *uip_conn;
#endif
#if UIP_TCP
/* The array containing all uIP connections. */
CCIF extern struct uip_conn uip_conns[UIP_CONNS];
#endif

/**
 * \addtogroup uiparch
 * @{
 */

/**
 * 4-byte array used for the 32-bit sequence number calculations.
 */
extern uint8_t uip_acc32[4];
/** @} */

/**
 * Representation of a uIP UDP connection.
 */
struct uip_udp_conn {
  uip_ipaddr_t ripaddr;   /**< The IP address of the remote peer. */
  uint16_t lport;        /**< The local port number in network byte order. */
  uint16_t rport;        /**< The remote port number in network byte order. */
  uint8_t  ttl;          /**< Default time-to-live. */

  /** The application state. */
  uip_udp_appstate_t appstate;

  /* buffer holding the data to this connection */
  struct net_buf *buf;
};

/**
 * The current UDP connection.
 */
#if 0
/* Moved to net_buf */
extern struct uip_udp_conn *uip_udp_conn;
#endif
extern struct uip_udp_conn uip_udp_conns[UIP_UDP_CONNS];

struct uip_fallback_interface {
  void (*init)(void);
  void (*output)(void);
};

#if UIP_CONF_ICMP6
struct uip_icmp6_conn {
  uip_icmp6_appstate_t appstate;

  /* buffer holding the data to this connection */
  struct net_buf *buf;
};
extern struct uip_icmp6_conn uip_icmp6_conns;
#endif /*UIP_CONF_ICMP6*/

/**
 * The uIP TCP/IP statistics.
 *
 * This is the variable in which the uIP TCP/IP statistics are gathered.
 */
#if UIP_STATISTICS == 1
extern struct uip_stats uip_stat;
#define UIP_STAT(s) s
#else
#define UIP_STAT(s)
#endif /* UIP_STATISTICS == 1 */

/**
 * The structure holding the TCP/IP statistics that are gathered if
 * UIP_STATISTICS is set to 1.
 *
 */
struct uip_stats {
  struct {
    uip_stats_t recv;     /**< Number of received packets at the IP
			     layer. */
    uip_stats_t sent;     /**< Number of sent packets at the IP
			     layer. */
    uip_stats_t forwarded;/**< Number of forwarded packets at the IP
			     layer. */
    uip_stats_t drop;     /**< Number of dropped packets at the IP
			     layer. */
    uip_stats_t vhlerr;   /**< Number of packets dropped due to wrong
			     IP version or header length. */
    uip_stats_t hblenerr; /**< Number of packets dropped due to wrong
			     IP length, high byte. */
    uip_stats_t lblenerr; /**< Number of packets dropped due to wrong
			     IP length, low byte. */
    uip_stats_t fragerr;  /**< Number of packets dropped because they
			     were IP fragments. */
    uip_stats_t chkerr;   /**< Number of packets dropped due to IP
			     checksum errors. */
    uip_stats_t protoerr; /**< Number of packets dropped because they
			     were neither ICMP, UDP nor TCP. */
  } ip;                   /**< IP statistics. */
  struct {
    uip_stats_t recv;     /**< Number of received ICMP packets. */
    uip_stats_t sent;     /**< Number of sent ICMP packets. */
    uip_stats_t drop;     /**< Number of dropped ICMP packets. */
    uip_stats_t typeerr;  /**< Number of ICMP packets with a wrong
			     type. */
    uip_stats_t chkerr;   /**< Number of ICMP packets with a bad
			     checksum. */
  } icmp;                 /**< ICMP statistics. */
#if UIP_TCP
  struct {
    uip_stats_t recv;     /**< Number of recived TCP segments. */
    uip_stats_t sent;     /**< Number of sent TCP segments. */
    uip_stats_t drop;     /**< Number of dropped TCP segments. */
    uip_stats_t chkerr;   /**< Number of TCP segments with a bad
			     checksum. */
    uip_stats_t ackerr;   /**< Number of TCP segments with a bad ACK
			     number. */
    uip_stats_t rst;      /**< Number of received TCP RST (reset) segments. */
    uip_stats_t rexmit;   /**< Number of retransmitted TCP segments. */
    uip_stats_t syndrop;  /**< Number of dropped SYNs because too few
			     connections were available. */
    uip_stats_t synrst;   /**< Number of SYNs for closed ports,
			     triggering a RST. */
  } tcp;                  /**< TCP statistics. */
#endif
#if UIP_UDP
  struct {
    uip_stats_t drop;     /**< Number of dropped UDP segments. */
    uip_stats_t recv;     /**< Number of recived UDP segments. */
    uip_stats_t sent;     /**< Number of sent UDP segments. */
    uip_stats_t chkerr;   /**< Number of UDP segments with a bad
			     checksum. */
  } udp;                  /**< UDP statistics. */
#endif /* UIP_UDP */
#if NETSTACK_CONF_WITH_IPV6
  struct {
    uip_stats_t drop;     /**< Number of dropped ND6 packets. */
    uip_stats_t recv;     /**< Number of recived ND6 packets */
    uip_stats_t sent;     /**< Number of sent ND6 packets */
  } nd6;
#endif /*NETSTACK_CONF_WITH_IPV6*/
};


/*---------------------------------------------------------------------------*/
/* All the stuff below this point is internal to uIP and should not be
 * used directly by an application or by a device driver.
 */
/*---------------------------------------------------------------------------*/



/* uint8_t uip_flags:
 *
 * When the application is called, uip_flags will contain the flags
 * that are defined in this file. Please read below for more
 * information.
 */
#if 0
/* Moved to net_buf */
CCIF extern uint8_t uip_flags;
#endif

/* The following flags may be set in the global variable uip_flags
   before calling the application callback. The UIP_ACKDATA,
   UIP_NEWDATA, and UIP_CLOSE flags may both be set at the same time,
   whereas the others are mutually exclusive. Note that these flags
   should *NOT* be accessed directly, but only through the uIP
   functions/macros. */

#define UIP_ACKDATA   1     /* Signifies that the outstanding data was
			       acked and the application should send
			       out new data instead of retransmitting
			       the last data. */
#define UIP_NEWDATA   2     /* Flags the fact that the peer has sent
			       us new data. */
#define UIP_REXMIT    4     /* Tells the application to retransmit the
			       data that was last sent. */
#define UIP_POLL      8     /* Used for polling the application, to
			       check if the application has data that
			       it wants to send. */
#define UIP_CLOSE     16    /* The remote host has closed the
			       connection, thus the connection has
			       gone away. Or the application signals
			       that it wants to close the
			       connection. */
#define UIP_ABORT     32    /* The remote host has aborted the
			       connection, thus the connection has
			       gone away. Or the application signals
			       that it wants to abort the
			       connection. */
#define UIP_CONNECTED 64    /* We have got a connection from a remote
                               host and have set up a new connection
                               for it, or an active connection has
                               been successfully established. */

#define UIP_TIMEDOUT  128   /* The connection has been aborted due to
			       too many retransmissions. */


/**
 * \brief process the options within a hop by hop or destination option header
 * \retval 0: nothing to send,
 * \retval 1: drop pkt
 * \retval 2: ICMP error message to send
 */
/*static uint8_t
uip_ext_hdr_options_process(); */

/* uip_process(flag):
 *
 * The actual uIP function which does all the work.
 */
uint8_t uip_process(struct net_buf **buf, uint8_t flag);

  /* The following flags are passed as an argument to the uip_process()
   function. They are used to distinguish between the two cases where
   uip_process() is called. It can be called either because we have
   incoming data that should be processed, or because the periodic
   timer has fired. These values are never used directly, but only in
   the macros defined in this file. */

#define UIP_DATA          1     /* Tells uIP that there is incoming
				   data in the uip_buf buffer. The
				   length of the data is stored in the
				   global variable uip_len. */
#define UIP_TIMER         2     /* Tells uIP that the periodic timer
				   has fired. */
#define UIP_POLL_REQUEST  3     /* Tells uIP that a connection should
				   be polled. */
#define UIP_UDP_SEND_CONN 4     /* Tells uIP that a UDP datagram
				   should be constructed in the
				   uip_buf buffer. */
#if UIP_UDP
#define UIP_UDP_TIMER     5
#endif /* UIP_UDP */

#if UIP_TCP
#define UIP_TCP_SEND_CONN 6     /* Tells uIP that a TCP segment
				   should be constructed in the
				   uip_buf buffer. */
#endif

/* The TCP states used in the uip_conn->tcpstateflags. */
#define UIP_CLOSED      0
#define UIP_SYN_RCVD    1
#define UIP_SYN_SENT    2
#define UIP_ESTABLISHED 3
#define UIP_FIN_WAIT_1  4
#define UIP_FIN_WAIT_2  5
#define UIP_CLOSING     6
#define UIP_TIME_WAIT   7
#define UIP_LAST_ACK    8
#define UIP_TS_MASK     15

#define UIP_STOPPED      16

/* The TCP and IP headers. */
struct uip_tcpip_hdr {
#if NETSTACK_CONF_WITH_IPV6
  /* IPv6 header. */
  uint8_t vtc,
    tcflow;
  uint16_t flow;
  uint8_t len[2];
  uint8_t proto, ttl;
  uip_ip6addr_t srcipaddr, destipaddr;
#else /* NETSTACK_CONF_WITH_IPV6 */
  /* IPv4 header. */
  uint8_t vhl,
    tos,
    len[2],
    ipid[2],
    ipoffset[2],
    ttl,
    proto;
  uint16_t ipchksum;
  uip_ipaddr_t srcipaddr, destipaddr;
#endif /* NETSTACK_CONF_WITH_IPV6 */

  /* TCP header. */
  uint16_t srcport,
    destport;
  uint8_t seqno[4],
    ackno[4],
    tcpoffset,
    flags,
    wnd[2];
  uint16_t tcpchksum;
  uint8_t urgp[2];
  uint8_t optdata[4];
} PACK_ALIAS_STRUCT;

/* The ICMP and IP headers. */
struct uip_icmpip_hdr {
#if NETSTACK_CONF_WITH_IPV6
  /* IPv6 header. */
  uint8_t vtc,
    tcf;
  uint16_t flow;
  uint8_t len[2];
  uint8_t proto, ttl;
  uip_ip6addr_t srcipaddr, destipaddr;
#else /* NETSTACK_CONF_WITH_IPV6 */
  /* IPv4 header. */
  uint8_t vhl,
    tos,
    len[2],
    ipid[2],
    ipoffset[2],
    ttl,
    proto;
  uint16_t ipchksum;
  uip_ipaddr_t srcipaddr, destipaddr;
#endif /* NETSTACK_CONF_WITH_IPV6 */

  /* ICMP header. */
  uint8_t type, icode;
  uint16_t icmpchksum;
#if !NETSTACK_CONF_WITH_IPV6
  uint16_t id, seqno;
  uint8_t payload[1];
#endif /* !NETSTACK_CONF_WITH_IPV6 */
} PACK_ALIAS_STRUCT;


/* The UDP and IP headers. */
struct uip_udpip_hdr {
#if NETSTACK_CONF_WITH_IPV6
  /* IPv6 header. */
  uint8_t vtc,
    tcf;
  uint16_t flow;
  uint8_t len[2];
  uint8_t proto, ttl;
  uip_ip6addr_t srcipaddr, destipaddr;
#else /* NETSTACK_CONF_WITH_IPV6 */
  /* IP header. */
  uint8_t vhl,
    tos,
    len[2],
    ipid[2],
    ipoffset[2],
    ttl,
    proto;
  uint16_t ipchksum;
  uip_ipaddr_t srcipaddr, destipaddr;
#endif /* NETSTACK_CONF_WITH_IPV6 */

  /* UDP header. */
  uint16_t srcport,
    destport;
  uint16_t udplen;
  uint16_t udpchksum;
} PACK_ALIAS_STRUCT;

/*
 * In IPv6 the length of the L3 headers before the transport header is
 * not fixed, due to the possibility to include extension option headers
 * after the IP header. hence we split here L3 and L4 headers
 */
/* The IP header */
struct uip_ip_hdr {
#if NETSTACK_CONF_WITH_IPV6
  /* IPV6 header */
  uint8_t vtc;
  uint8_t tcflow;
  uint16_t flow;
  uint8_t len[2];
  uint8_t proto, ttl;
  uip_ip6addr_t srcipaddr, destipaddr;
#else /* NETSTACK_CONF_WITH_IPV6 */
  /* IPV4 header */
  uint8_t vhl,
    tos,
    len[2],
    ipid[2],
    ipoffset[2],
    ttl,
    proto;
  uint16_t ipchksum;
  uip_ipaddr_t srcipaddr, destipaddr;
#endif /* NETSTACK_CONF_WITH_IPV6 */
} PACK_ALIAS_STRUCT;


/*
 * IPv6 extension option headers: we are able to process
 * the 4 extension headers defined in RFC2460 (IPv6):
 * - Hop by hop option header, destination option header:
 *   These two are not used by any core IPv6 protocol, hence
 *   we just read them and go to the next. They convey options,
 *   the options defined in RFC2460 are Pad1 and PadN, which do
 *   some padding, and that we do not need to read (the length
 *   field in the header is enough)
 * - Routing header: this one is most notably used by MIPv6,
 *   which we do not implement, hence we just read it and go
 *   to the next
 * - Fragmentation header: we read this header and are able to
 *   reassemble packets
 *
 * We do not offer any means to send packets with extension headers
 *
 * We do not implement Authentication and ESP headers, which are
 * used in IPSec and defined in RFC4302,4303,4305,4385
 */
/* common header part */
typedef struct uip_ext_hdr {
  uint8_t next;
  uint8_t len;
} PACK_ALIAS_STRUCT uip_ext_hdr;

/* Hop by Hop option header */
typedef struct uip_hbho_hdr {
  uint8_t next;
  uint8_t len;
} PACK_ALIAS_STRUCT uip_hbho_hdr;

/* destination option header */
typedef struct uip_desto_hdr {
  uint8_t next;
  uint8_t len;
} uip_desto_hdr;

/* We do not define structures for PAD1 and PADN options */

/*
 * routing header
 * the routing header as 4 common bytes, then routing header type
 * specific data there are several types of routing header. Type 0 was
 * deprecated as per RFC5095 most notable other type is 2, used in
 * RFC3775 (MIPv6) here we do not implement MIPv6, so we just need to
 * parse the 4 first bytes
 */
typedef struct uip_routing_hdr {
  uint8_t next;
  uint8_t len;
  uint8_t routing_type;
  uint8_t seg_left;
} PACK_ALIAS_STRUCT uip_routing_hdr;

/* fragmentation header */
typedef struct uip_frag_hdr {
  uint8_t next;
  uint8_t res;
  uint16_t offsetresmore;
  uint32_t id;
} uip_frag_hdr;

/*
 * an option within the destination or hop by hop option headers
 * it contains type an length, which is true for all options but PAD1
 */
typedef struct uip_ext_hdr_opt {
  uint8_t type;
  uint8_t len;
} PACK_ALIAS_STRUCT uip_ext_hdr_opt;

/* PADN option */
typedef struct uip_ext_hdr_opt_padn {
  uint8_t opt_type;
  uint8_t opt_len;
} PACK_ALIAS_STRUCT uip_ext_hdr_opt_padn;

/* RPL option */
typedef struct uip_ext_hdr_opt_rpl {
  uint8_t opt_type;
  uint8_t opt_len;
  uint8_t flags;
  uint8_t instance;
  uint16_t senderrank;
} PACK_ALIAS_STRUCT uip_ext_hdr_opt_rpl;

/* TCP header */
struct uip_tcp_hdr {
  uint16_t srcport;
  uint16_t destport;
  uint8_t seqno[4];
  uint8_t ackno[4];
  uint8_t tcpoffset;
  uint8_t flags;
  uint8_t  wnd[2];
  uint16_t tcpchksum;
  uint8_t urgp[2];
  uint8_t optdata[4];
} PACK_ALIAS_STRUCT;

/* The ICMP headers. */
struct uip_icmp_hdr {
  uint8_t type, icode;
  uint16_t icmpchksum;
#if !NETSTACK_CONF_WITH_IPV6
  uint16_t id, seqno;
#endif /* !NETSTACK_CONF_WITH_IPV6 */
} PACK_ALIAS_STRUCT;


/* The UDP headers. */
struct uip_udp_hdr {
  uint16_t srcport;
  uint16_t destport;
  uint16_t udplen;
  uint16_t udpchksum;
} PACK_ALIAS_STRUCT;


/**
 * The buffer size available for user data in the \ref uip_buf buffer.
 *
 * This macro holds the available size for user data in the \ref
 * uip_buf buffer. The macro is intended to be used for checking
 * bounds of available user data.
 *
 * Example:
 \code
 snprintf(uip_appdata, UIP_APPDATA_SIZE, "%u\n", i);
 \endcode
 *
 * \hideinitializer
 */
#define UIP_APPDATA_SIZE (UIP_BUFSIZE - UIP_LLH_LEN - UIP_TCPIP_HLEN)
#define UIP_APPDATA_PTR(buf) (void *)&uip_buf(buf)[UIP_LLH_LEN + UIP_TCPIP_HLEN]

#define UIP_PROTO_ICMP  1
#define UIP_PROTO_TCP   6
#define UIP_PROTO_UDP   17
#define UIP_PROTO_ICMP6 58


#if NETSTACK_CONF_WITH_IPV6
/** @{ */
/** \brief  extension headers types */
#define UIP_PROTO_HBHO        0
#define UIP_PROTO_DESTO       60
#define UIP_PROTO_ROUTING     43
#define UIP_PROTO_FRAG        44
#define UIP_PROTO_NONE        59
/** @} */

/** @{ */
/** \brief  Destination and Hop By Hop extension headers option types */
#define UIP_EXT_HDR_OPT_PAD1  0
#define UIP_EXT_HDR_OPT_PADN  1
#define UIP_EXT_HDR_OPT_RPL   0x63

/** @} */

/** @{ */
/**
 * \brief Bitmaps for extension header processing
 *
 * When processing extension headers, we should record somehow which one we
 * see, because you cannot have twice the same header, except for destination
 * We store all this in one uint8_t bitmap one bit for each header expected. The
 * order in the bitmap is the order recommended in RFC2460
 */
#define UIP_EXT_HDR_BITMAP_HBHO 0x01
#define UIP_EXT_HDR_BITMAP_DESTO1 0x02
#define UIP_EXT_HDR_BITMAP_ROUTING 0x04
#define UIP_EXT_HDR_BITMAP_FRAG 0x08
#define UIP_EXT_HDR_BITMAP_AH 0x10
#define UIP_EXT_HDR_BITMAP_ESP 0x20
#define UIP_EXT_HDR_BITMAP_DESTO2 0x40
/** @} */


#endif /* NETSTACK_CONF_WITH_IPV6 */


#if UIP_FIXEDADDR
CCIF extern const uip_ipaddr_t uip_hostaddr, uip_netmask, uip_draddr;
#else /* UIP_FIXEDADDR */
CCIF extern uip_ipaddr_t uip_hostaddr, uip_netmask, uip_draddr;
#endif /* UIP_FIXEDADDR */
CCIF extern const uip_ipaddr_t uip_broadcast_addr;
CCIF extern const uip_ipaddr_t uip_all_zeroes_addr;

#if UIP_FIXEDETHADDR
CCIF extern const uip_lladdr_t uip_lladdr;
#else
CCIF extern uip_lladdr_t uip_lladdr;
#endif




#if NETSTACK_CONF_WITH_IPV6
/** Length of the link local prefix */
#define UIP_LLPREF_LEN     10

/**
 * \brief Is IPv6 address a the unspecified address
 * a is of type uip_ipaddr_t
 */
#define uip_is_addr_loopback(a)                  \
  ((((a)->u16[0]) == 0) &&                       \
   (((a)->u16[1]) == 0) &&                       \
   (((a)->u16[2]) == 0) &&                       \
   (((a)->u16[3]) == 0) &&                       \
   (((a)->u16[4]) == 0) &&                       \
   (((a)->u16[5]) == 0) &&                       \
   (((a)->u16[6]) == 0) &&                       \
   (((a)->u8[14]) == 0) &&                       \
   (((a)->u8[15]) == 0x01))
/**
 * \brief Is IPv6 address a the unspecified address
 * a is of type uip_ipaddr_t
 */
#define uip_is_addr_unspecified(a)               \
  ((((a)->u16[0]) == 0) &&                       \
   (((a)->u16[1]) == 0) &&                       \
   (((a)->u16[2]) == 0) &&                       \
   (((a)->u16[3]) == 0) &&                       \
   (((a)->u16[4]) == 0) &&                       \
   (((a)->u16[5]) == 0) &&                       \
   (((a)->u16[6]) == 0) &&                       \
   (((a)->u16[7]) == 0))

/** \brief Is IPv6 address a the link local all-nodes multicast address */
#define uip_is_addr_linklocal_allnodes_mcast(a)     \
  ((((a)->u8[0]) == 0xff) &&                        \
   (((a)->u8[1]) == 0x02) &&                        \
   (((a)->u16[1]) == 0) &&                          \
   (((a)->u16[2]) == 0) &&                          \
   (((a)->u16[3]) == 0) &&                          \
   (((a)->u16[4]) == 0) &&                          \
   (((a)->u16[5]) == 0) &&                          \
   (((a)->u16[6]) == 0) &&                          \
   (((a)->u8[14]) == 0) &&                          \
   (((a)->u8[15]) == 0x01))

/** \brief Is IPv6 address a the link local all-routers multicast address */
#define uip_is_addr_linklocal_allrouters_mcast(a)     \
  ((((a)->u8[0]) == 0xff) &&                        \
   (((a)->u8[1]) == 0x02) &&                        \
   (((a)->u16[1]) == 0) &&                          \
   (((a)->u16[2]) == 0) &&                          \
   (((a)->u16[3]) == 0) &&                          \
   (((a)->u16[4]) == 0) &&                          \
   (((a)->u16[5]) == 0) &&                          \
   (((a)->u16[6]) == 0) &&                          \
   (((a)->u8[14]) == 0) &&                          \
   (((a)->u8[15]) == 0x02))

/**
 * \brief Checks whether the address a is link local.
 * a is of type uip_ipaddr_t
 */
#define uip_is_addr_linklocal(a)                 \
  ((a)->u8[0] == 0xfe &&                         \
   (a)->u8[1] == 0x80)

/** \brief set IP address a to unspecified */
#define uip_create_unspecified(a) uip_ip6addr(a, 0, 0, 0, 0, 0, 0, 0, 0)

/** \brief set IP address a to the link local all-nodes multicast address */
#define uip_create_linklocal_allnodes_mcast(a) uip_ip6addr(a, 0xff02, 0, 0, 0, 0, 0, 0, 0x0001)

/** \brief set IP address a to the link local all-routers multicast address */
#define uip_create_linklocal_allrouters_mcast(a) uip_ip6addr(a, 0xff02, 0, 0, 0, 0, 0, 0, 0x0002)
#define uip_create_linklocal_prefix(addr) do { \
    (addr)->u16[0] = UIP_HTONS(0xfe80);            \
    (addr)->u16[1] = 0;                        \
    (addr)->u16[2] = 0;                        \
    (addr)->u16[3] = 0;                        \
  } while(0)

/**
 * \brief  is addr (a) a solicited node multicast address, see RFC3513
 *  a is of type uip_ipaddr_t*
 */
#define uip_is_addr_solicited_node(a)          \
  ((((a)->u8[0])  == 0xFF) &&                  \
   (((a)->u8[1])  == 0x02) &&                  \
   (((a)->u16[1]) == 0x00) &&                  \
   (((a)->u16[2]) == 0x00) &&                  \
   (((a)->u16[3]) == 0x00) &&                  \
   (((a)->u16[4]) == 0x00) &&                  \
   (((a)->u8[10]) == 0x00) &&                  \
   (((a)->u8[11]) == 0x01) &&                  \
   (((a)->u8[12]) == 0xFF))

/**
 * \brief put in b the solicited node address corresponding to address a
 * both a and b are of type uip_ipaddr_t*
 * */
#define uip_create_solicited_node(a, b)    \
  (((b)->u8[0]) = 0xFF);                        \
  (((b)->u8[1]) = 0x02);                        \
  (((b)->u16[1]) = 0);                          \
  (((b)->u16[2]) = 0);                          \
  (((b)->u16[3]) = 0);                          \
  (((b)->u16[4]) = 0);                          \
  (((b)->u8[10]) = 0);                          \
  (((b)->u8[11]) = 0x01);                       \
  (((b)->u8[12]) = 0xFF);                       \
  (((b)->u8[13]) = ((a)->u8[13]));              \
  (((b)->u16[7]) = ((a)->u16[7]))

/**
 * \brief is addr (a) a link local unicast address, see RFC3513
 *  i.e. is (a) on prefix FE80::/10
 *  a is of type uip_ipaddr_t*
 */
#define uip_is_addr_link_local(a) \
  ((((a)->u8[0]) == 0xFE) && \
  (((a)->u8[1]) == 0x80))

/**
 * \brief was addr (a) forged based on the mac address m
 * a type is uip_ipaddr_t
 * m type is uiplladdr_t
 */
#if UIP_CONF_LL_802154
#define uip_is_addr_mac_addr_based(a, m) \
  ((((a)->u8[8])  == (((m)->addr[0]) ^ 0x02)) &&   \
   (((a)->u8[9])  == (m)->addr[1]) &&            \
   (((a)->u8[10]) == (m)->addr[2]) &&            \
   (((a)->u8[11]) == (m)->addr[3]) &&            \
   (((a)->u8[12]) == (m)->addr[4]) &&            \
   (((a)->u8[13]) == (m)->addr[5]) &&            \
   (((a)->u8[14]) == (m)->addr[6]) &&            \
   (((a)->u8[15]) == (m)->addr[7]))
#else

#define uip_is_addr_mac_addr_based(a, m) \
  ((((a)->u8[8])  == (((m)->addr[0]) | 0x02)) &&   \
   (((a)->u8[9])  == (m)->addr[1]) &&            \
   (((a)->u8[10]) == (m)->addr[2]) &&            \
   (((a)->u8[11]) == 0xff) &&            \
   (((a)->u8[12]) == 0xfe) &&            \
   (((a)->u8[13]) == (m)->addr[3]) &&            \
   (((a)->u8[14]) == (m)->addr[4]) &&            \
   (((a)->u8[15]) == (m)->addr[5]))

#endif /*UIP_CONF_LL_802154*/

/**
 * \brief is address a multicast address, see RFC 3513
 * a is of type uip_ipaddr_t*
 * */
#define uip_is_addr_mcast(a)                    \
  (((a)->u8[0]) == 0xFF)

/**
 * \brief is address a global multicast address (FFxE::/16),
 * a is of type uip_ip6addr_t*
 * */
#define uip_is_addr_mcast_global(a) \
  ((((a)->u8[0]) == 0xFF) && \
   (((a)->u8[1] & 0x0F) == 0x0E))

/**
 * \brief is address a non-routable multicast address.
 * Scopes 1 (interface-local) and 2 (link-local) are non-routable
 * See RFC4291 and draft-ietf-6man-multicast-scopes
 * a is of type uip_ip6addr_t*
 * */
#define uip_is_addr_mcast_non_routable(a) \
  ((((a)->u8[0]) == 0xFF) && \
   (((a)->u8[1] & 0x0F) <= 0x02))

/**
 * \brief is address a routable multicast address.
 * Scope 3 (Realm-Local) or higher are routable
 * Realm-Local scope is defined in draft-ietf-6man-multicast-scopes
 * See RFC4291 and draft-ietf-6man-multicast-scopes
 * a is of type uip_ip6addr_t*
 * */
#define uip_is_addr_mcast_routable(a) \
  ((((a)->u8[0]) == 0xFF) && \
   (((a)->u8[1] & 0x0F) > 0x02))

/**
 * \brief is group-id of multicast address a
 * the all nodes group-id
 */
#define uip_is_mcast_group_id_all_nodes(a) \
  ((((a)->u16[1])  == 0) &&                 \
   (((a)->u16[2])  == 0) &&                 \
   (((a)->u16[3])  == 0) &&                 \
   (((a)->u16[4])  == 0) &&                 \
   (((a)->u16[5])  == 0) &&                 \
   (((a)->u16[6])  == 0) &&                 \
   (((a)->u8[14])  == 0) &&                 \
   (((a)->u8[15])  == 1))

/**
 * \brief is group-id of multicast address a
 * the all routers group-id
 */
#define uip_is_mcast_group_id_all_routers(a) \
  ((((a)->u16[1])  == 0) &&                 \
   (((a)->u16[2])  == 0) &&                 \
   (((a)->u16[3])  == 0) &&                 \
   (((a)->u16[4])  == 0) &&                 \
   (((a)->u16[5])  == 0) &&                 \
   (((a)->u16[6])  == 0) &&                 \
   (((a)->u8[14])  == 0) &&                 \
   (((a)->u8[15])  == 2))


/**
 * \brief are last three bytes of both addresses equal?
 * This is used to compare solicited node multicast addresses
 */
#define uip_are_solicited_bytes_equal(a, b)             \
  ((((a)->u8[13])  == ((b)->u8[13])) &&                 \
   (((a)->u8[14])  == ((b)->u8[14])) &&                 \
   (((a)->u8[15])  == ((b)->u8[15])))

#endif /*NETSTACK_CONF_WITH_IPV6*/

/**
 * Calculate the Internet checksum over a buffer.
 *
 * The Internet checksum is the one's complement of the one's
 * complement sum of all 16-bit words in the buffer.
 *
 * See RFC1071.
 *
 * \param data A pointer to the buffer over which the checksum is to be
 * computed.
 *
 * \param len The length of the buffer over which the checksum is to
 * be computed.
 *
 * \return The Internet checksum of the buffer.
 */
uint16_t uip_chksum(uint16_t *data, uint16_t len);

/**
 * Calculate the IP header checksum of the packet header in uip_buf.
 *
 * The IP header checksum is the Internet checksum of the 20 bytes of
 * the IP header.
 *
 * \return The IP header checksum of the IP header in the uip_buf
 * buffer.
 */
uint16_t uip_ipchksum(struct net_buf *buf);

/**
 * Calculate the TCP checksum of the packet in uip_buf and uip_appdata.
 *
 * The TCP checksum is the Internet checksum of data contents of the
 * TCP segment, and a pseudo-header as defined in RFC793.
 *
 * \return The TCP checksum of the TCP segment in uip_buf and pointed
 * to by uip_appdata.
 */
uint16_t uip_tcpchksum(struct net_buf *buf);

/**
 * Calculate the UDP checksum of the packet in uip_buf and uip_appdata.
 *
 * The UDP checksum is the Internet checksum of data contents of the
 * UDP segment, and a pseudo-header as defined in RFC768.
 *
 * \return The UDP checksum of the UDP segment in uip_buf and pointed
 * to by uip_appdata.
 */
uint16_t uip_udpchksum(struct net_buf *buf);

/**
 * Calculate the ICMP checksum of the packet in uip_buf.
 *
 * \return The ICMP checksum of the ICMP packet in uip_buf
 */
uint16_t uip_icmp6chksum(struct net_buf *buf);


#endif /* UIP_H_ */


/** @} */
