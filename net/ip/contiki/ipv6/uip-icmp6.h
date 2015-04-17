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
 * \addtogroup uip6
 * @{
 */

/**
 * \file
 *    Header file for ICMPv6 message and error handing (RFC 4443)
 * \author Julien Abeille <jabeille@cisco.com> 
 * \author Mathilde Durvy <mdurvy@cisco.com>
 */

#ifndef ICMP6_H_
#define ICMP6_H_

#include "net/ip/uip.h"


/** \name ICMPv6 message types */
/** @{ */
#define ICMP6_DST_UNREACH                 1	/**< dest unreachable */
#define ICMP6_PACKET_TOO_BIG	            2	/**< packet too big */
#define ICMP6_TIME_EXCEEDED	            3	/**< time exceeded */
#define ICMP6_PARAM_PROB	               4	/**< ip6 header bad */
#define ICMP6_ECHO_REQUEST              128  /**< Echo request */
#define ICMP6_ECHO_REPLY                129  /**< Echo reply */

#define ICMP6_RS                        133  /**< Router Solicitation */
#define ICMP6_RA                        134  /**< Router Advertisement */
#define ICMP6_NS                        135  /**< Neighbor Solicitation */
#define ICMP6_NA                        136  /**< Neighbor advertisement */
#define ICMP6_REDIRECT                  137  /**< Redirect */

#define ICMP6_RPL                       155  /**< RPL */
#define ICMP6_PRIV_EXP_100              100  /**< Private Experimentation */
#define ICMP6_PRIV_EXP_101              101  /**< Private Experimentation */
#define ICMP6_PRIV_EXP_200              200  /**< Private Experimentation */
#define ICMP6_PRIV_EXP_201              201  /**< Private Experimentation */
#define ICMP6_ROLL_TM    ICMP6_PRIV_EXP_200  /**< ROLL Trickle Multicast */
/** @} */


/** \name ICMPv6 Destination Unreachable message codes*/
/** @{ */
#define ICMP6_DST_UNREACH_NOROUTE         0 /**< no route to destination */
#define ICMP6_DST_UNREACH_ADMIN	         1 /**< administratively prohibited */
#define ICMP6_DST_UNREACH_NOTNEIGHBOR     2 /**< not a neighbor(obsolete) */
#define ICMP6_DST_UNREACH_BEYONDSCOPE     2 /**< beyond scope of source address */
#define ICMP6_DST_UNREACH_ADDR	         3 /**< address unreachable */
#define ICMP6_DST_UNREACH_NOPORT          4 /**< port unreachable */
/** @} */

/** \name ICMPv6 Time Exceeded message codes*/
/** @{ */
#define ICMP6_TIME_EXCEED_TRANSIT         0 /**< ttl==0 in transit */
#define ICMP6_TIME_EXCEED_REASSEMBLY      1 /**< ttl==0 in reass */
/** @} */

/** \name ICMPv6 Parameter Problem message codes*/
/** @{ */
#define ICMP6_PARAMPROB_HEADER            0 /**< erroneous header field */
#define ICMP6_PARAMPROB_NEXTHEADER        1 /**< unrecognized next header */
#define ICMP6_PARAMPROB_OPTION            2 /**< unrecognized option */
/** @} */

/** \brief Echo Request constant part length */
#define UIP_ICMP6_ECHO_REQUEST_LEN        4

/** \brief ICMPv6 Error message constant part length */
#define UIP_ICMP6_ERROR_LEN               4

/** \brief ICMPv6 Error message constant part */
typedef struct uip_icmp6_error{
  uint32_t param;
} uip_icmp6_error;

/** \name ICMPv6 RFC4443 Message processing and sending */
/** @{ */
/**
 * \brief Send an icmpv6 error message
 * \param type type of the error message
 * \param code of the error message
 * \param param 32 bit parameter of the error message, semantic depends on error
 */
void
uip_icmp6_error_output(uint8_t type, uint8_t code, uint32_t param); 

/**
 * \brief Send an icmpv6 message
 * \param dest destination address of the message
 * \param type type of the message
 * \param code of the message
 * \param payload_len length of the payload
 */
void
uip_icmp6_send(const uip_ipaddr_t *dest, int type, int code, int payload_len);



typedef void (* uip_icmp6_echo_reply_callback_t)(uip_ipaddr_t *source,
                                                 uint8_t ttl,
                                                 uint8_t *data,
                                                 uint16_t datalen);
struct uip_icmp6_echo_reply_notification {
  struct uip_icmp6_echo_reply_notification *next;
  uip_icmp6_echo_reply_callback_t callback;
};

/**
 * \brief      Add a callback function for ping replies
 * \param n    A struct uip_icmp6_echo_reply_notification that must have been allocated by the caller
 * \param c    A pointer to the callback function to be called
 *
 *             This function adds a callback function to the list of
 *             callback functions that are called when an ICMP echo
 *             reply (ping reply) is received. This is used when
 *             implementing a ping protocol: attach a callback
 *             function to the ping reply, then send out a ping packet
 *             with uip_icmp6_send().
 *
 *             The caller must have statically allocated a struct
 *             uip_icmp6_echo_reply_notification to hold the internal
 *             state of the callback function.
 *
 *             When a ping reply packet is received, all registered
 *             callback functions are called. The callback functions
 *             must not modify the contents of the uIP buffer.
 */
void
uip_icmp6_echo_reply_callback_add(struct uip_icmp6_echo_reply_notification *n,
                                  uip_icmp6_echo_reply_callback_t c);

/**
 * \brief      Remove a callback function for ping replies
 * \param n    A struct uip_icmp6_echo_reply_notification that must have been previously added with uip_icmp6_echo_reply_callback_add()
 *
 *             This function removes a callback function from the list of
 *             callback functions that are called when an ICMP echo
 *             reply (ping reply) is received.
 */
void
uip_icmp6_echo_reply_callback_rm(struct uip_icmp6_echo_reply_notification *n);

/* Generic ICMPv6 input handers */
typedef struct uip_icmp6_input_handler {
  struct uip_icmp6_input_handler *next;
  uint8_t type;
  uint8_t icode;
  void (*handler)(void);
} uip_icmp6_input_handler_t;

#define UIP_ICMP6_INPUT_SUCCESS     0
#define UIP_ICMP6_INPUT_ERROR       1

#define UIP_ICMP6_HANDLER_CODE_ANY 0xFF /* Handle all codes for this type */

/*
 * Initialise a variable of type uip_icmp6_input_handler, to be used later as
 * the argument to uip_icmp6_register_input_handler
 *
 * The function pointer stored in this variable will get called and will be
 * expected to handle incoming ICMPv6 datagrams of the specified type/code
 *
 * If code has a value of UIP_ICMP6_HANDLER_CODE_ANY, the same function
 * will handle all codes for this type. In other words, the ICMPv6
 * message's code is "don't care"
 */
#define UIP_ICMP6_HANDLER(name, type, code, func) \
  static uip_icmp6_input_handler_t name = { NULL, type, code, func }

/**
 * \brief Handle an incoming ICMPv6 message
 * \param type The ICMPv6 message type
 * \param icode The ICMPv6 message code
 * \return Success: UIP_ICMP6_INPUT_SUCCESS, Error: UIP_ICMP6_INPUT_ERROR
 *
 * Generic handler for unknown ICMPv6 types. It will lookup for a registered
 * function capable of handing this message type. The function must have first
 * been registered with uip_icmp6_register_input_handler. The function is in
 * charge of setting uip_len to 0, otherwise the uIPv6 core will attempt to
 * send whatever remains in the UIP_IP_BUF.
 *
 * A return value of UIP_ICMP6_INPUT_ERROR means that a handler could not be
 * invoked. This is most likely because the ICMPv6 type does not have a valid
 * handler associated with it.

 * UIP_ICMP6_INPUT_SUCCESS signifies that a handler was found for this ICMPv6
 * type and that it was invoked. It does NOT provide any indication whatsoever
 * regarding whether the handler itself succeeded.
 */
uint8_t uip_icmp6_input(uint8_t type, uint8_t icode);

/**
 * \brief Register a handler which can handle a specific ICMPv6 message type
 * \param handler A pointer to the handler
 */
void uip_icmp6_register_input_handler(uip_icmp6_input_handler_t *handler);


/**
 * \brief Initialise the uIP ICMPv6 core
 */
void uip_icmp6_init(void);

/** @} */

#endif /*ICMP6_H_*/
/** @} */

