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
 *         Header file for the Rime buffer (packetbuf) management
 * \author
 *         Adam Dunkels <adam@sics.se>
 */

/**
 * \addtogroup rime
 * @{
 */

/**
 * \defgroup packetbuf Rime buffer management
 * @{
 *
 * The packetbuf module does Rime's buffer management.
 */

#include "contiki-conf.h"
#include "contiki/linkaddr.h"
#include "contiki/llsec/llsec802154.h"

#ifndef PACKETBUF_H_
#define PACKETBUF_H_

/**
 * \brief      The size of the packetbuf, in bytes
 */
#ifdef PACKETBUF_CONF_SIZE
#define PACKETBUF_SIZE PACKETBUF_CONF_SIZE
#else
#define PACKETBUF_SIZE 128
#endif

/**
 * \brief      The size of the packetbuf header, in bytes
 */
#ifdef PACKETBUF_CONF_HDR_SIZE
#define PACKETBUF_HDR_SIZE PACKETBUF_CONF_HDR_SIZE
#else
#define PACKETBUF_HDR_SIZE 48
#endif

struct net_buf;

/**
 * \brief      Clear and reset the packetbuf
 *
 *             This function clears the packetbuf and resets all
 *             internal state pointers (header size, header pointer,
 *             external data pointer). It is used before preparing a
 *             packet in the packetbuf.
 *
 */
void packetbuf_clear(struct net_buf *buf);

/**
 * \brief      Clear and reset the header of the packetbuf
 *
 *             This function clears the header of the packetbuf and
 *             resets all the internal state pointers pertaining to
 *             the header (header size, header pointer, but not
 *             external data pointer). It is used before after sending
 *             a packet in the packetbuf, to be able to reuse the
 *             packet buffer for a later retransmission.
 *
 */
void packetbuf_clear_hdr(struct net_buf *buf);

void packetbuf_hdr_remove(struct net_buf *buf, int bytes);

/**
 * \brief      Get a pointer to the data in the packetbuf
 * \return     Pointer to the packetbuf data
 *
 *             This function is used to get a pointer to the data in
 *             the packetbuf. The data is either stored in the packetbuf,
 *             or referenced to an external location.
 *
 *             For outbound packets, the packetbuf consists of two
 *             parts: header and data. The header is accessed with the
 *             packetbuf_hdrptr() function.
 *
 *             For incoming packets, both the packet header and the
 *             packet data is stored in the data portion of the
 *             packetbuf. Thus this function is used to get a pointer to
 *             the header for incoming packets.
 *
 */
void *packetbuf_dataptr(struct net_buf *buf);

/**
 * \brief      Get a pointer to the header in the packetbuf, for outbound packets
 * \return     Pointer to the packetbuf header
 *
 *             For outbound packets, the packetbuf consists of two
 *             parts: header and data. This function is used to get a
 *             pointer to the header in the packetbuf. The header is
 *             stored in the packetbuf.
 *
 */
void *packetbuf_hdrptr(struct net_buf *buf);

/**
 * \brief      Get the length of the header in the packetbuf
 * \return     Length of the header in the packetbuf
 *
 *             For outbound packets, the packetbuf consists of two
 *             parts: header and data. This function is used to get
 *             the length of the header in the packetbuf. The header is
 *             stored in the packetbuf and accessed via the
 *             packetbuf_hdrptr() function.
 *
 */
uint8_t packetbuf_hdrlen(struct net_buf *buf);


/**
 * \brief      Get the length of the data in the packetbuf
 * \return     Length of the data in the packetbuf
 *
 *             For outbound packets, the packetbuf consists of two
 *             parts: header and data. This function is used to get
 *             the length of the data in the packetbuf. The data is
 *             stored in the packetbuf and accessed via the
 *             packetbuf_dataptr() function.
 *
 *             For incoming packets, both the packet header and the
 *             packet data is stored in the data portion of the
 *             packetbuf. This function is then used to get the total
 *             length of the packet - both header and data.
 *
 */
uint16_t packetbuf_datalen(struct net_buf *buf);

/**
 * \brief      Get the total length of the header and data in the packetbuf
 * \return     Length of data and header in the packetbuf
 *
 */
uint16_t packetbuf_totlen(struct net_buf *buf);

/**
 * \brief      Set the length of the data in the packetbuf
 * \param len  The length of the data
 *
 *             For outbound packets, the packetbuf consists of two
 *             parts: header and data. This function is used to set
 *             the length of the data in the packetbuf.
 */
void packetbuf_set_datalen(struct net_buf *buf, uint16_t len);

/**
 * \brief      Point the packetbuf to external data
 * \param ptr  A pointer to the external data
 * \param len  The length of the external data
 *
 *             For outbound packets, the packetbuf consists of two
 *             parts: header and data. This function is used to make
 *             the packetbuf point to external data. The function also
 *             specifies the length of the external data that the
 *             packetbuf references.
 */
void packetbuf_reference(struct net_buf *buf, void *ptr, uint16_t len);

/**
 * \brief      Check if the packetbuf references external data
 * \retval     Non-zero if the packetbuf references external data, zero otherwise.
 *
 *             For outbound packets, the packetbuf consists of two
 *             parts: header and data. This function is used to check
 *             if the packetbuf points to external data that has
 *             previously been referenced with packetbuf_reference().
 *
 */
int packetbuf_is_reference(struct net_buf *buf);

/**
 * \brief      Get a pointer to external data referenced by the packetbuf
 * \retval     A pointer to the external data
 *
 *             For outbound packets, the packetbuf consists of two
 *             parts: header and data. The data may point to external
 *             data that has previously been referenced with
 *             packetbuf_reference(). This function is used to get a
 *             pointer to the external data.
 *
 */
void *packetbuf_reference_ptr(struct net_buf *buf);

/**
 * \brief      Compact the packetbuf
 *
 *             This function compacts the packetbuf by copying the data
 *             portion of the packetbuf so that becomes consecutive to
 *             the header. It also copies external data that has
 *             previously been referenced with packetbuf_reference()
 *             into the packetbuf.
 *
 *             This function is called by the Rime code before a
 *             packet is to be sent by a device driver. This assures
 *             that the entire packet is consecutive in memory.
 *
 */
void packetbuf_compact(struct net_buf *buf);

/**
 * \brief      Copy from external data into the packetbuf
 * \param from A pointer to the data from which to copy
 * \param len  The size of the data to copy
 * \retval     The number of bytes that was copied into the packetbuf
 *
 *             This function copies data from a pointer into the
 *             packetbuf. If the data that is to be copied is larger
 *             than the packetbuf, only the data that fits in the
 *             packetbuf is copied. The number of bytes that could be
 *             copied into the rimbuf is returned.
 *
 */
int packetbuf_copyfrom(struct net_buf *buf, const void *from, uint16_t len);

/**
 * \brief      Copy the entire packetbuf to an external buffer
 * \param to   A pointer to the buffer to which the data is to be copied
 * \retval     The number of bytes that was copied to the external buffer
 *
 *             This function copies the packetbuf to an external
 *             buffer. Both the data portion and the header portion of
 *             the packetbuf is copied. If the packetbuf referenced
 *             external data (referenced with packetbuf_reference()) the
 *             external data is copied.
 *
 *             The external buffer to which the packetbuf is to be
 *             copied must be able to accommodate at least
 *             (PACKETBUF_SIZE + PACKETBUF_HDR_SIZE) bytes. The number of
 *             bytes that was copied to the external buffer is
 *             returned.
 *
 */
int packetbuf_copyto(struct net_buf *buf, void *to);

/**
 * \brief      Copy the header portion of the packetbuf to an external buffer
 * \param to   A pointer to the buffer to which the data is to be copied
 * \retval     The number of bytes that was copied to the external buffer
 *
 *             This function copies the header portion of the packetbuf
 *             to an external buffer.
 *
 *             The external buffer to which the packetbuf is to be
 *             copied must be able to accommodate at least
 *             PACKETBUF_HDR_SIZE bytes. The number of bytes that was
 *             copied to the external buffer is returned.
 *
 */
int packetbuf_copyto_hdr(struct net_buf *buf, uint8_t *to);

/**
 * \brief      Extend the header of the packetbuf, for outbound packets
 * \param size The number of bytes the header should be extended
 * \retval     Non-zero if the header could be extended, zero otherwise
 *
 *             This function is used to allocate extra space in the
 *             header portion in the packetbuf, when preparing outbound
 *             packets for transmission. If the function is unable to
 *             allocate sufficient header space, the function returns
 *             zero and does not allocate anything.
 *
 */
int packetbuf_hdralloc(struct net_buf *buf, int size);

/**
 * \brief      Reduce the header in the packetbuf, for incoming packets
 * \param size The number of bytes the header should be reduced
 * \retval     Non-zero if the header could be reduced, zero otherwise
 *
 *             This function is used to remove the first part of the
 *             header in the packetbuf, when processing incoming
 *             packets. If the function is unable to remove the
 *             requested amount of header space, the function returns
 *             zero and does not allocate anything.
 *
 */
int packetbuf_hdrreduce(struct net_buf *buf, int size);

/* Packet attributes stuff below: */

typedef uint16_t packetbuf_attr_t;

struct packetbuf_attr {
/*   uint8_t type; */
  packetbuf_attr_t val;
};
struct packetbuf_addr {
/*   uint8_t type; */
  linkaddr_t addr;
};

#define PACKETBUF_ATTR_PACKET_TYPE_DATA      0
#define PACKETBUF_ATTR_PACKET_TYPE_ACK       1
#define PACKETBUF_ATTR_PACKET_TYPE_STREAM    2
#define PACKETBUF_ATTR_PACKET_TYPE_STREAM_END 3
#define PACKETBUF_ATTR_PACKET_TYPE_TIMESTAMP 4

enum {
  PACKETBUF_ATTR_NONE,

  /* Scope 0 attributes: used only on the local node. */
  PACKETBUF_ATTR_CHANNEL,
  PACKETBUF_ATTR_NETWORK_ID,
  PACKETBUF_ATTR_LINK_QUALITY,
  PACKETBUF_ATTR_RSSI,
  PACKETBUF_ATTR_TIMESTAMP,
  PACKETBUF_ATTR_RADIO_TXPOWER,
  PACKETBUF_ATTR_LISTEN_TIME,
  PACKETBUF_ATTR_TRANSMIT_TIME,
  PACKETBUF_ATTR_MAX_MAC_TRANSMISSIONS,
  PACKETBUF_ATTR_MAC_SEQNO,
  PACKETBUF_ATTR_MAC_ACK,
  PACKETBUF_ATTR_IS_CREATED_AND_SECURED,
  
  /* Scope 1 attributes: used between two neighbors only. */
  PACKETBUF_ATTR_RELIABLE,
  PACKETBUF_ATTR_PACKET_ID,
  PACKETBUF_ATTR_PACKET_TYPE,
#if NETSTACK_CONF_WITH_RIME
  PACKETBUF_ATTR_REXMIT,
  PACKETBUF_ATTR_MAX_REXMIT,
  PACKETBUF_ATTR_NUM_REXMIT,
#endif /* NETSTACK_CONF_WITH_RIME */
  PACKETBUF_ATTR_PENDING,
  PACKETBUF_ATTR_FRAME_TYPE,
#if LLSEC802154_SECURITY_LEVEL
  PACKETBUF_ATTR_SECURITY_LEVEL,
  PACKETBUF_ATTR_FRAME_COUNTER_BYTES_0_1,
  PACKETBUF_ATTR_FRAME_COUNTER_BYTES_2_3,
#if LLSEC802154_USES_EXPLICIT_KEYS
  PACKETBUF_ATTR_KEY_ID_MODE,
  PACKETBUF_ATTR_KEY_INDEX,
  PACKETBUF_ATTR_KEY_SOURCE_BYTES_0_1,
#endif /* LLSEC802154_USES_EXPLICIT_KEYS */
#endif /* LLSEC802154_SECURITY_LEVEL */
  
  /* Scope 2 attributes: used between end-to-end nodes. */
#if NETSTACK_CONF_WITH_RIME
  PACKETBUF_ATTR_HOPS,
  PACKETBUF_ATTR_TTL,
  PACKETBUF_ATTR_EPACKET_ID,
  PACKETBUF_ATTR_EPACKET_TYPE,
  PACKETBUF_ATTR_ERELIABLE,
#endif /* NETSTACK_CONF_WITH_RIME */

  /* These must be last */
  PACKETBUF_ADDR_SENDER,
  PACKETBUF_ADDR_RECEIVER,
#if NETSTACK_CONF_WITH_RIME
  PACKETBUF_ADDR_ESENDER,
  PACKETBUF_ADDR_ERECEIVER,
#endif /* NETSTACK_CONF_WITH_RIME */

  PACKETBUF_ATTR_MAX
};

/* Define surrogates when 802.15.4 security is off */
#if !LLSEC802154_SECURITY_LEVEL
enum {
  PACKETBUF_ATTR_SECURITY_LEVEL,
  PACKETBUF_ATTR_FRAME_COUNTER_BYTES_0_1,
  PACKETBUF_ATTR_FRAME_COUNTER_BYTES_2_3
};
#endif /* LLSEC802154_SECURITY_LEVEL */

/* Define surrogates when not using explicit keys */
#if !LLSEC802154_USES_EXPLICIT_KEYS
enum {
  PACKETBUF_ATTR_KEY_ID_MODE,
  PACKETBUF_ATTR_KEY_INDEX,
  PACKETBUF_ATTR_KEY_SOURCE_BYTES_0_1
};
#endif /* LLSEC802154_USES_EXPLICIT_KEYS */

#if NETSTACK_CONF_WITH_RIME
#define PACKETBUF_NUM_ADDRS 4
#else /* NETSTACK_CONF_WITH_RIME */
#define PACKETBUF_NUM_ADDRS 2
#endif /* NETSTACK_CONF_WITH_RIME */
#define PACKETBUF_NUM_ATTRS (PACKETBUF_ATTR_MAX - PACKETBUF_NUM_ADDRS)
#define PACKETBUF_ADDR_FIRST PACKETBUF_ADDR_SENDER

#define PACKETBUF_IS_ADDR(type) ((type) >= PACKETBUF_ADDR_FIRST)

#if PACKETBUF_CONF_ATTRS_INLINE

#if 0
/* Moved to net_buf.h */
extern struct packetbuf_attr packetbuf_attrs[];
extern struct packetbuf_addr packetbuf_addrs[];
#endif

static int               packetbuf_set_attr(struct net_buf *buf, uint8_t type, const packetbuf_attr_t val);
static packetbuf_attr_t    packetbuf_attr(struct net_buf *buf, uint8_t type);
static int               packetbuf_set_addr(struct net_buf *buf, uint8_t type, const linkaddr_t *addr);
static const linkaddr_t *packetbuf_addr(struct net_buf *buf, uint8_t type);

static inline int
packetbuf_set_attr(struct net_buf *buf, uint8_t type, const packetbuf_attr_t val)
{
/*   packetbuf_attrs[type].type = type; */
  uip_pkt_packetbuf_attrs(buf)[type].val = val;
  return 1;
}
static inline packetbuf_attr_t
packetbuf_attr(struct net_buf *buf, uint8_t type)
{
  return uip_pkt_packetbuf_attrs(buf)[type].val;
}

static inline int
packetbuf_set_addr(struct net_buf *buf, uint8_t type, const linkaddr_t *addr)
{
/*   packetbuf_addrs[type - PACKETBUF_ADDR_FIRST].type = type; */
  linkaddr_copy(&uip_pkt_packetbuf_addrs(buf)[type - PACKETBUF_ADDR_FIRST].addr, addr);
  return 1;
}

static inline const linkaddr_t *
packetbuf_addr(struct net_buf *buf, uint8_t type)
{
  return &uip_pkt_packetbuf_addrs(buf)[type - PACKETBUF_ADDR_FIRST].addr;
}
#else /* PACKETBUF_CONF_ATTRS_INLINE */
int               packetbuf_set_attr(struct net_buf *buf, uint8_t type, const packetbuf_attr_t val);
packetbuf_attr_t packetbuf_attr(struct net_buf *buf, uint8_t type);
int               packetbuf_set_addr(struct net_buf *buf, uint8_t type, const linkaddr_t *addr);
const linkaddr_t *packetbuf_addr(struct net_buf *buf, uint8_t type);
#endif /* PACKETBUF_CONF_ATTRS_INLINE */

/**
 * \brief      Checks whether the current packet is a broadcast.
 * \retval 0   iff current packet is not a broadcast
 */
int               packetbuf_holds_broadcast(struct net_buf *buf);

void              packetbuf_attr_clear(struct net_buf *buf);

void              packetbuf_attr_copyto(struct net_buf *buf,
					struct packetbuf_attr *attrs,
					struct packetbuf_addr *addrs);
void              packetbuf_attr_copyfrom(struct net_buf *buf,
					  struct packetbuf_attr *attrs,
					  struct packetbuf_addr *addrs);

#define PACKETBUF_ATTRIBUTES(...) { __VA_ARGS__ PACKETBUF_ATTR_LAST }
#define PACKETBUF_ATTR_LAST { PACKETBUF_ATTR_NONE, 0 }

#define PACKETBUF_ATTR_BIT  1
#define PACKETBUF_ATTR_BYTE 8
#define PACKETBUF_ADDRSIZE (LINKADDR_SIZE * PACKETBUF_ATTR_BYTE)

struct packetbuf_attrlist {
  uint8_t type;
  uint8_t len;
};

#endif /* PACKETBUF_H_ */
/** @} */
/** @} */
