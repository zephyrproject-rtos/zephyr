/* sicslowpan_fragmentation.c - 802.15.4 6lowpan packet fragmentation */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Copyright (c) 2008, Swedish Institute of Computer Science.
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
 *         6lowpan implementation (RFC4944 and draft-ietf-6lowpan-hc-06)
 *
 * \author Adam Dunkels <adam@sics.se>
 * \author Nicolas Tsiftes <nvt@sics.se>
 * \author Niclas Finne <nfi@sics.se>
 * \author Mathilde Durvy <mdurvy@cisco.com>
 * \author Julien Abeille <jabeille@cisco.com>
 * \author Joakim Eriksson <joakime@sics.se>
 * \author Joel Hoglund <joel@sics.se>
 */

#include <stdio.h>
#include <string.h>

#include <net/l2_buf.h>
#include <net_driver_15_4.h>
#include "contiki/sicslowpan/sicslowpan_fragmentation.h"
#include "contiki/netstack.h"
#include "contiki/packetbuf.h"
#include "contiki/queuebuf.h"
#include "contiki/ip/uip.h"
#include "contiki/ip/tcpip.h"
#include "dev/watchdog.h"

#include "contiki/ipv6/uip-ds6-nbr.h"

#ifdef CONFIG_NETWORK_IP_STACK_DEBUG_15_4_6LOWPAN_FRAG
#define DEBUG 1
#endif
#include "contiki/ip/uip-debug.h"

#if DEBUG
#define PRINTPACKETBUF() do { uint8_t p; PRINTF("packetbuf buffer: "); for(p = 0; p < packetbuf_datalen(); p++){PRINTF("%.2X", *(packetbuf_ptr + p));} PRINTF("\n"); } while(0)
#define PRINTUIPBUF() do { uint8_t p; PRINTF("UIP buffer: "); for(p = 0; p < uip_len; p++){PRINTF("%.2X", uip_buf[p]);}PRINTF("\n"); } while(0)
#define PRINTSICSLOWPANBUF() do { uint8_t p; PRINTF("SICSLOWPAN buffer: "); for(p = 0; p < sicslowpan_len; p++){PRINTF("%.2X", sicslowpan_buf[p]);}PRINTF("\n"); } while(0)
#else
#define PRINTPACKETBUF()
#define PRINTUIPBUF()
#define PRINTSICSLOWPANBUF()
#endif /* DEBUG == 1*/

#if UIP_LOGGING
#include <stdio.h>
void uip_log(char *msg);
#define UIP_LOG(m) uip_log(m)
#else
#define UIP_LOG(m)
#endif /* UIP_LOGGING == 1 */

#define GET16(ptr,index) (((uint16_t)((ptr)[index] << 8)) | ((ptr)[(index) + 1]))
#define SET16(ptr,index,value) do {     \
  (ptr)[index] = ((value) >> 8) & 0xff; \
  (ptr)[index + 1] = (value) & 0xff;    \
} while(0)

/** \name Pointers in the packetbuf buffer
 *  @{
 */
//#define PACKETBUF_FRAG_PTR           (packetbuf_ptr)
#define PACKETBUF_FRAG_DISPATCH_SIZE 0   /* 16 bit */
#define PACKETBUF_FRAG_TAG           2   /* 16 bit */
#define PACKETBUF_FRAG_OFFSET        4   /* 8 bit */

/** \name Pointers in the sicslowpan and uip buffer
 *  @{
 */

/* NOTE: In the multiple-reassembly context there is only room for the header / first fragment */
#define SICSLOWPAN_IP_BUF(buf)   ((struct uip_ip_hdr *)&sicslowpan_buf(buf)[UIP_LLH_LEN])
#define SICSLOWPAN_UDP_BUF(buf) ((struct uip_udp_hdr *)&sicslowpan_buf(buf)[UIP_LLIPH_LEN])

#define UIP_IP_BUF(buf)          ((struct uip_ip_hdr *)&uip_buf(buf)[UIP_LLH_LEN])
#define UIP_UDP_BUF(buf)          ((struct uip_udp_hdr *)&uip_buf(buf)[UIP_LLIPH_LEN])
#define UIP_TCP_BUF(buf)          ((struct uip_tcp_hdr *)&uip_buf(buf)[UIP_LLIPH_LEN])
#define UIP_ICMP_BUF(buf)          ((struct uip_icmp_hdr *)&uip_buf(buf)[UIP_LLIPH_LEN])
/** @} */

#define sicslowpan_buf uip_buf
#define sicslowpan_len uip_len

#ifdef SICSLOWPAN_CONF_MAX_MAC_TRANSMISSIONS
#define SICSLOWPAN_MAX_MAC_TRANSMISSIONS SICSLOWPAN_CONF_MAX_MAC_TRANSMISSIONS
#else
#define SICSLOWPAN_MAX_MAC_TRANSMISSIONS 4
#endif

/** \brief Maximum available size for frame headers,
           link layer security-related overhead, as well as
           6LoWPAN payload. */
#ifdef SICSLOWPAN_CONF_MAC_MAX_PAYLOAD
#define MAC_MAX_PAYLOAD SICSLOWPAN_CONF_MAC_MAX_PAYLOAD
#else /* SICSLOWPAN_CONF_MAC_MAX_PAYLOAD */
#define MAC_MAX_PAYLOAD (127 - 2)
#endif /* SICSLOWPAN_CONF_MAC_MAX_PAYLOAD */

#define SICSLOWPAN_DISPATCH_IPV6	0x41 /* 01000001 = 65 */

static int last_rssi;

/** Datagram tag to be put in the fragments I send. */
static uint16_t my_tag;

/* This needs to be defined in NBR / Nodes depending on available RAM   */
/*   and expected reassembly requirements                               */
#ifdef SICSLOWPAN_CONF_FRAGMENT_BUFFERS
#define SICSLOWPAN_FRAGMENT_BUFFERS SICSLOWPAN_CONF_FRAGMENT_BUFFERS
#else
#define SICSLOWPAN_FRAGMENT_BUFFERS 16
#endif

/* REASS_CONTEXTS corresponds to the number of simultaneous             */
/* reassemblys that can be made.                                        */
#ifdef SICSLOWPAN_CONF_REASS_CONTEXTS
#define SICSLOWPAN_REASS_CONTEXTS SICSLOWPAN_CONF_REASS_CONTEXTS
#else
#define SICSLOWPAN_REASS_CONTEXTS 2
#endif

/* The size of each fragment (IP payload) for the 6lowpan fragmentation */
#ifdef SICSLOWPAN_CONF_FRAGMENT_SIZE
#define SICSLOWPAN_FRAGMENT_SIZE SICSLOWPAN_CONF_FRAGMENT_SIZE
#else
#define SICSLOWPAN_FRAGMENT_SIZE 110
#endif

/* Assuming that the worst growth for uncompression is 38 bytes */
#define SICSLOWPAN_FIRST_FRAGMENT_SIZE (SICSLOWPAN_FRAGMENT_SIZE + 38)

/* all information needed for reassembly */
struct sicslowpan_frag_info {
  /** When reassembling, the source address of the fragments being merged */
  linkaddr_t sender;
  /** The destination address of the fragments being merged */
  linkaddr_t receiver;
  /** When reassembling, the tag in the fragments being merged. */
  uint16_t tag;
  /** Total length of the fragmented packet */
  uint16_t len;
  /** Current length of reassembled fragments */
  uint16_t reassembled_len;
  /** Last fragment */
  uint8_t last_fragment;
  /** Reassembly %process %timer. */
  struct timer reass_timer;
};

static struct sicslowpan_frag_info frag_info[SICSLOWPAN_REASS_CONTEXTS];

struct sicslowpan_frag_buf {
  /* the index of the frag_info */
  uint8_t index;
  /* Fragment offset */
  uint8_t offset;
  /* Length of this fragment (if zero this buffer is not allocated) */
  uint8_t len;
  uint8_t data[SICSLOWPAN_FRAGMENT_SIZE];
};

static struct sicslowpan_frag_buf frag_buf[SICSLOWPAN_FRAGMENT_BUFFERS];

/*---------------------------------------------------------------------------*/
static int
clear_fragments(uint8_t frag_info_index)
{
  int i, clear_count;
  clear_count = 0;
  frag_info[frag_info_index].len = 0;
  for(i = 0; i < SICSLOWPAN_FRAGMENT_BUFFERS; i++) {
    if(frag_buf[i].len > 0 && frag_buf[i].index == frag_info_index) {
      /* deallocate the buffer */
      frag_buf[i].len = 0;
      clear_count++;
    }
  }
  return clear_count;
}
/*---------------------------------------------------------------------------*/
static int
store_fragment(struct net_buf *mbuf, uint8_t index, uint8_t offset)
{
  int i;
  for(i = 0; i < SICSLOWPAN_FRAGMENT_BUFFERS; i++) {
    if(frag_buf[i].len == 0) {
      /* copy over the data from packetbuf into the fragment buffer
       * and store offset and len */
      frag_buf[i].offset = offset; /* frag offset */
      frag_buf[i].len = packetbuf_datalen(mbuf) - uip_packetbuf_hdr_len(mbuf);
      frag_buf[i].index = index;
      memcpy(frag_buf[i].data, uip_packetbuf_ptr(mbuf) + uip_packetbuf_hdr_len(mbuf),
             packetbuf_datalen(mbuf) - uip_packetbuf_hdr_len(mbuf));

      PRINTF("Fragment payload length: %d\n", frag_buf[i].len);
      /* return the length of the stored fragment */
      return frag_buf[i].len;
    }
  }
  /* failed */
  return -1;
}
/*---------------------------------------------------------------------------*/
/* add a new fragment to the buffer */
static int8_t
add_fragment(struct net_buf *mbuf, uint16_t tag, uint16_t frag_size, uint8_t offset)
{
  int i;
  int len;
  int8_t found = -1;

  if(offset == 0) {
    /* This is a first fragment - check if we can add this */
    for(i = 0; i < SICSLOWPAN_REASS_CONTEXTS; i++) {
      /* clear all fragment info with expired timer to free all fragment buffers */
      if(frag_info[i].len > 0 && timer_expired(&frag_info[i].reass_timer)) {
       clear_fragments(i);
      }

      /* We use len as indication on used or not used */
      if(found < 0 && frag_info[i].len == 0) {
        /* We remember the first free fragment info but must continue
           the loop to free any other expired fragment buffers. */
        found = i;
      }
    }

    if(found < 0) {
      PRINTF("*** Failed to store new fragment session - tag: %d\n", tag);
      return -1;
    }

    /* Found a free fragment info to store data in */
    frag_info[found].len = frag_size;
    frag_info[found].tag = tag;
    linkaddr_copy(&frag_info[found].sender,
                  packetbuf_addr(mbuf, PACKETBUF_ADDR_SENDER));
    linkaddr_copy(&frag_info[found].receiver,
                  packetbuf_addr(mbuf, PACKETBUF_ADDR_RECEIVER));

    timer_set(&frag_info[found].reass_timer, SICSLOWPAN_REASS_MAXAGE * CLOCK_SECOND / 16);
    i = found;
    goto store;
  }

  /* This is a N-fragment - should find the info */
  for(i = 0; i < SICSLOWPAN_REASS_CONTEXTS; i++) {
    if(frag_info[i].tag == tag && frag_info[i].len > 0 &&
       linkaddr_cmp(&frag_info[i].sender, packetbuf_addr(mbuf, PACKETBUF_ADDR_SENDER))) {
      /* Tag and Sender match - this must be the correct info to store in */
      found = i;
      break;
    }
  }

  if(found < 0) {
    /* no entry found for storing the new fragment */
    PRINTF("*** Failed to store N-fragment - could not find session - tag: %d offset: %d\n", tag, offset);
    return -1;
  }

store:
  /* i is the index of the reassembly context */
  len = store_fragment(mbuf, i, offset);
  if(len > 0) {
    frag_info[i].reassembled_len += len;
    if((offset << 3)+ len >= frag_size) {
      frag_info[i].last_fragment = 1;
    } else {
      frag_info[i].last_fragment = 0;
    }
    return i;
  } else {
    /* should we also clear all fragments since we failed to store this fragment? */
    PRINTF("*** Failed to store fragment - packet reassembly will fail tag:%d l\n", frag_info[i].tag);
    return -1;
  }
}

/*---------------------------------------------------------------------------*/
/* Copy all the fragments that are associated with a specific context into uip */
static struct net_buf *copy_frags2uip(int context)
{
  int i, total_len = 0;
  struct net_buf *buf;

  buf = ip_buf_get_reserve_rx(0);
  if(!buf) {
    return NULL;
  }

  /* Copy from the fragment context info buffer first */
  linkaddr_copy(&ip_buf_ll_dest(buf), &frag_info[context].receiver);
  linkaddr_copy(&ip_buf_ll_src(buf), &frag_info[context].sender);

  for(i = 0; i < SICSLOWPAN_FRAGMENT_BUFFERS; i++) {
    if(i == 0) {
       uip_first_frag_len(buf) = frag_buf[i].len;
       if(frag_buf[i].data[0] == SICSLOWPAN_DISPATCH_IPV6) {
         memmove(frag_buf[i].data, frag_buf[i].data + 1, frag_buf[i].len - 1);
         frag_buf[i].len -= 1;
         uip_uncompressed(buf) = 1;
        } else {
         uip_uncompressed(buf) = 0;
        }
    }
    /* And also copy all matching fragments */
    if(frag_buf[i].len > 0 && frag_buf[i].index == context) {
      memcpy(uip_buf(buf) + (uint16_t)(frag_buf[i].offset << 3),
            (uint8_t *)frag_buf[i].data, frag_buf[i].len);
      total_len += frag_buf[i].len;
    }
  }
  net_buf_add(buf, total_len);
  uip_len(buf) = total_len;

  /* deallocate all the fragments for this context */
  clear_fragments(context);

  return buf;
}

static struct net_buf *copy_buf(struct net_buf *mbuf)
{
  struct net_buf *buf;

  buf = ip_buf_get_reserve_rx(0);
  if(!buf) {
    return NULL;
  }

  /* Copy from the fragment context info buffer first */
  linkaddr_copy(&ip_buf_ll_dest(buf),
		packetbuf_addr(mbuf, PACKETBUF_ADDR_RECEIVER));
  linkaddr_copy(&ip_buf_ll_src(buf),
		packetbuf_addr(mbuf, PACKETBUF_ADDR_SENDER));

  PRINTF("%s: mbuf datalen %d dataptr %p buf %p\n", __FUNCTION__,
	  packetbuf_datalen(mbuf), packetbuf_dataptr(mbuf), uip_buf(buf));
  if(packetbuf_datalen(mbuf) > 0 &&
     packetbuf_datalen(mbuf) <= UIP_BUFSIZE - UIP_LLH_LEN) {
    memcpy(uip_buf(buf), packetbuf_dataptr(mbuf), packetbuf_datalen(mbuf));
    uip_len(buf) = packetbuf_datalen(mbuf);
    net_buf_add(buf, uip_len(buf));
  } else {
    ip_buf_unref(buf);
    buf = NULL;
  }

  uip_first_frag_len(buf) = 0;
  uip_uncompressed(buf) = 0;
  return buf;
}

static void
packet_sent(struct net_buf *buf, void *ptr, int status, int transmissions)
{
  const linkaddr_t *dest = packetbuf_addr(buf, PACKETBUF_ADDR_RECEIVER);
  uip_ds6_link_neighbor_callback(dest, status, transmissions);
  uip_last_tx_status(buf) = status;
  l2_buf_unref(buf);
}

/*--------------------------------------------------------------------*/
/**
 * \brief This function is called by the 6lowpan code to send out a
 * packet.
 * \param dest the link layer destination address of the packet
 */
static void
send_packet(struct net_buf *buf, linkaddr_t *dest, bool last_fragment, void *ptr)
{
  /* Set the link layer destination address for the packet as a
   * packetbuf attribute. The MAC layer can access the destination
   * address with the function packetbuf_addr(PACKETBUF_ADDR_RECEIVER).
   */
  packetbuf_set_addr(buf, PACKETBUF_ADDR_RECEIVER, dest);

#if NETSTACK_CONF_BRIDGE_MODE
  /* This needs to be explicitly set here for bridge mode to work */
  packetbuf_set_addr(buf, PACKETBUF_ADDR_SENDER,(void*)&uip_lladdr);
#endif

  /* Force acknowledge from sender (test hardware autoacks) */
#if SICSLOWPAN_CONF_ACK_ALL
  packetbuf_set_attr(buf, PACKETBUF_ATTR_RELIABLE, 1);
#endif

  /* Provide a callback function to receive the result of
     a packet transmission. */
  NETSTACK_LLSEC.send(buf, &packet_sent, last_fragment, ptr);

  /* If we are sending multiple packets in a row, we need to let the
     watchdog know that we are still alive. */
  watchdog_periodic();
}

static int fragment(struct net_buf *buf, void *ptr)
{
   struct queuebuf *q;
   int max_payload;
   int framer_hdrlen;
   uint16_t frag_tag;
   uint16_t frag_offset;
   int hdr_diff;
   /* Number of bytes processed. */
   uint16_t processed_ip_out_len;
   struct net_buf *mbuf;
   bool last_fragment = false;

#define USE_FRAMER_HDRLEN 0
#if USE_FRAMER_HDRLEN
  framer_hdrlen = NETSTACK_FRAMER.length();
  if(framer_hdrlen < 0) {
    /* Framing failed, we assume the maximum header length */
    framer_hdrlen = 21;
  }
#else /* USE_FRAMER_HDRLEN */
  framer_hdrlen = 21;
#endif /* USE_FRAMER_HDRLEN */
  max_payload = MAC_MAX_PAYLOAD - framer_hdrlen - NETSTACK_LLSEC.get_overhead();

  PRINTF("max_payload: %d, framer_hdrlen: %d \n",max_payload, framer_hdrlen);

  mbuf = l2_buf_get_reserve(0);
  if (!mbuf) {
     goto fail;
  }
	uip_last_tx_status(mbuf) = MAC_TX_OK;

  /*
   * The destination address will be tagged to each outbound
   * packet. If the argument localdest is NULL, we are sending a
   * broadcast packet.
   */

  if((int)uip_len(buf) <= max_payload) {
    /* The packet does not need to be fragmented, send buf */
    packetbuf_copyfrom(mbuf, uip_buf(buf), uip_len(buf));
    send_packet(mbuf, &ip_buf_ll_dest(buf), true, ptr);
    ip_buf_unref(buf);
    return 1;
   }

    uip_uncomp_hdr_len(mbuf) = 0;
    uip_packetbuf_hdr_len(mbuf) = 0;
    packetbuf_clear(mbuf);
    uip_packetbuf_ptr(mbuf) = packetbuf_dataptr(mbuf);

    packetbuf_set_attr(mbuf, PACKETBUF_ATTR_MAX_MAC_TRANSMISSIONS,
                                     SICSLOWPAN_MAX_MAC_TRANSMISSIONS);

    PRINTF("fragmentation: total packet len %d\n", uip_len(buf));

    /*
     * The outbound IPv6 packet is too large to fit into a single 15.4
     * packet, so we fragment it into multiple packets and send them.
     * The first fragment contains frag1 dispatch, then
     * IPv6/HC1/HC06/HC_UDP dispatchs/headers.
     * The following fragments contain only the fragn dispatch.
     */
    int estimated_fragments = ((int)uip_len(buf)) / (max_payload - SICSLOWPAN_FRAGN_HDR_LEN) + 1;
    int freebuf = queuebuf_numfree(mbuf) - 1;
    PRINTF("uip_len: %d, fragments: %d, free bufs: %d\n", uip_len(buf), estimated_fragments, freebuf);
    if(freebuf < estimated_fragments) {
      PRINTF("Dropping packet, not enough free bufs\n");
      goto fail;
    }

    hdr_diff = uip_uncompressed_hdr_len(buf) - uip_compressed_hdr_len(buf);
    PRINTF("fragment: hdr difference %d\n", hdr_diff);
    /* Create 1st Fragment */
    SET16(uip_packetbuf_ptr(mbuf), PACKETBUF_FRAG_DISPATCH_SIZE,
          ((SICSLOWPAN_DISPATCH_FRAG1 << 8) | (uip_len(buf) + hdr_diff)));

    frag_tag = my_tag++;
    SET16(uip_packetbuf_ptr(mbuf), PACKETBUF_FRAG_TAG, frag_tag);
    PRINTF("fragment: tag %d \n", frag_tag);

    /* Copy payload and send */
    uip_packetbuf_hdr_len(mbuf) = uip_compressed_hdr_len(buf);
    uip_packetbuf_hdr_len(mbuf) += SICSLOWPAN_FRAG1_HDR_LEN;
    uip_packetbuf_payload_len(mbuf) = (max_payload - uip_packetbuf_hdr_len(mbuf)) & 0xf8;
    PRINTF("fragment: payload len %d, hdr len %d, tag %d\n",
               uip_packetbuf_payload_len(mbuf), uip_packetbuf_hdr_len(mbuf), frag_tag);

    memcpy(uip_packetbuf_ptr(mbuf) + SICSLOWPAN_FRAG1_HDR_LEN,
              uip_buf(buf), uip_packetbuf_payload_len(mbuf) +
              uip_packetbuf_hdr_len(mbuf));
    packetbuf_set_datalen(mbuf, uip_packetbuf_payload_len(mbuf) + uip_packetbuf_hdr_len(mbuf));
    PRINTF("fragment: packetbuf_datalen %d\n", packetbuf_datalen(mbuf));
    q = queuebuf_new_from_packetbuf(mbuf);
    if(q == NULL) {
      PRINTF("could not allocate queuebuf for first fragment, dropping packet\n");
      goto fail;
    }
    net_buf_ref(mbuf);
    send_packet(mbuf, &ip_buf_ll_dest(buf), last_fragment, ptr);
    queuebuf_to_packetbuf(mbuf, q);
    queuebuf_free(q);
    q = NULL;

    /* Check tx result. */
    if((uip_last_tx_status(mbuf) == MAC_TX_COLLISION) ||
       (uip_last_tx_status(mbuf) == MAC_TX_ERR) ||
       (uip_last_tx_status(mbuf) == MAC_TX_ERR_FATAL)) {
      PRINTF("error in fragment tx, dropping subsequent fragments.\n");
      goto fail;
    }

    /* set processed_ip_out_len to what we already sent from the IP payload*/
    processed_ip_out_len = uip_packetbuf_payload_len(mbuf) + uip_compressed_hdr_len(buf);
    /*
     * Create following fragments
     * Datagram tag is already in the buffer, we need to set the
     * FRAGN dispatch and for each fragment, the offset
     */
    uip_packetbuf_hdr_len(mbuf) = SICSLOWPAN_FRAGN_HDR_LEN;
    SET16(uip_packetbuf_ptr(mbuf), PACKETBUF_FRAG_DISPATCH_SIZE,
          ((SICSLOWPAN_DISPATCH_FRAGN << 8) | (uip_len(buf) + hdr_diff)));
    uip_packetbuf_payload_len(mbuf) = (max_payload - uip_packetbuf_hdr_len(mbuf)) & 0xf8;

    while(processed_ip_out_len < uip_len(buf)) {
      PRINTF("fragment: tag:%d, processed_ip_out_len:%d \n", frag_tag, processed_ip_out_len);
      frag_offset = processed_ip_out_len + hdr_diff;
      uip_packetbuf_ptr(mbuf)[PACKETBUF_FRAG_OFFSET] = frag_offset >> 3;
      /* Copy payload and send */
      if(uip_len(buf) - processed_ip_out_len < uip_packetbuf_payload_len(mbuf)) {
        /* last fragment */
        last_fragment = true;
        uip_packetbuf_payload_len(mbuf) = uip_len(buf) - processed_ip_out_len;
      }
      PRINTF("fragment: offset %d, len %d, tag %d\n",
             frag_offset, uip_packetbuf_payload_len(mbuf), frag_tag);
      memcpy(uip_packetbuf_ptr(mbuf) + uip_packetbuf_hdr_len(mbuf),
             (uint8_t *)UIP_IP_BUF(buf) + processed_ip_out_len, uip_packetbuf_payload_len(mbuf));
      packetbuf_set_datalen(mbuf, uip_packetbuf_payload_len(mbuf) + uip_packetbuf_hdr_len(mbuf));
      PRINTF("fragment: packetbuf_datalen %d\n", packetbuf_datalen(mbuf));
      q = queuebuf_new_from_packetbuf(mbuf);
      if(q == NULL) {
        PRINTF("could not allocate queuebuf, dropping fragment\n");
        goto fail;
      }
      net_buf_ref(mbuf);
      send_packet(mbuf, &ip_buf_ll_dest(buf), last_fragment, ptr);
      queuebuf_to_packetbuf(mbuf, q);
      queuebuf_free(q);
      q = NULL;
      processed_ip_out_len += uip_packetbuf_payload_len(mbuf);

      /* Check tx result. */
      if((uip_last_tx_status(mbuf) == MAC_TX_COLLISION) ||
         (uip_last_tx_status(mbuf) == MAC_TX_ERR) ||
         (uip_last_tx_status(mbuf) == MAC_TX_ERR_FATAL)) {
        PRINTF("error in fragment tx, dropping subsequent fragments.\n");
        goto fail;
      }
    }

    ip_buf_unref(buf);
    l2_buf_unref(mbuf);
    return 1;

fail:
    if (mbuf) {
      l2_buf_unref(mbuf);
    }
    return 0;
}

static int reassemble(struct net_buf *mbuf)
{
  /* size of the IP packet (read from fragment) */
  uint16_t frag_size = 0;
  int8_t frag_context = 0;
  /* offset of the fragment in the IP packet */
  uint8_t frag_offset = 0;
  uint8_t is_fragment = 0;
  /* tag of the fragment */
  uint16_t frag_tag = 0;
  uint8_t first_fragment = 0, last_fragment = 0;
  struct net_buf *buf = NULL; 

  /* init */
  uip_uncomp_hdr_len(mbuf) = 0;
  uip_packetbuf_hdr_len(mbuf) = 0;

  /* The MAC puts the 15.4 payload inside the packetbuf data buffer */
  uip_packetbuf_ptr(mbuf) = packetbuf_dataptr(mbuf);

  /* Save the RSSI of the incoming packet in case the upper layer will
     want to query us for it later. */
  last_rssi = (signed short)packetbuf_attr(mbuf, PACKETBUF_ATTR_RSSI);

   /*
   * Since we don't support the mesh and broadcast header, the first header
   * we look for is the fragmentation header
   */
  switch((GET16(uip_packetbuf_ptr(mbuf), PACKETBUF_FRAG_DISPATCH_SIZE) & 0xf800) >> 8) {
    case SICSLOWPAN_DISPATCH_FRAG1:
      PRINTF("reassemble: FRAG1 ");
      frag_offset = 0;
      frag_size = GET16(uip_packetbuf_ptr(mbuf), PACKETBUF_FRAG_DISPATCH_SIZE) & 0x07ff;
      frag_tag = GET16(uip_packetbuf_ptr(mbuf), PACKETBUF_FRAG_TAG);

      PRINTF("size %d, tag %d, offset %d\n", frag_size, frag_tag, frag_offset);

      if (frag_size > IP_BUF_MAX_DATA) {
        PRINTF("Too big packet %d bytes (max %d), fragment discarded\n",
	       frag_size, IP_BUF_MAX_DATA);
	goto fail;
      }

      uip_packetbuf_hdr_len(mbuf) += SICSLOWPAN_FRAG1_HDR_LEN;
      first_fragment = 1;
      is_fragment = 1;

      /* Add the fragment to the fragmentation context (this will also copy the payload)*/
      frag_context = add_fragment(mbuf, frag_tag, frag_size, frag_offset);
      if(frag_context == -1) {
        goto fail;
      }

      break;

    case SICSLOWPAN_DISPATCH_FRAGN:
      /*
       * set offset, tag, size
       * Offset is in units of 8 bytes
       */
      PRINTF("reassemble: FRAGN ");
      frag_offset = uip_packetbuf_ptr(mbuf)[PACKETBUF_FRAG_OFFSET];
      frag_tag = GET16(uip_packetbuf_ptr(mbuf), PACKETBUF_FRAG_TAG);
      frag_size = GET16(uip_packetbuf_ptr(mbuf), PACKETBUF_FRAG_DISPATCH_SIZE) & 0x07ff;

      PRINTF("reassemble: size %d, tag %d, offset %d\n", frag_size, frag_tag, frag_offset);

      uip_packetbuf_hdr_len(mbuf) += SICSLOWPAN_FRAGN_HDR_LEN;

      /* If this is the last fragment, we may shave off any extrenous
         bytes at the end. We must be liberal in what we accept. */
      /* Add the fragment to the fragmentation context  (this will also copy the payload) */
      frag_context = add_fragment(mbuf, frag_tag, frag_size, frag_offset);
      if(frag_context == -1) {
        goto fail;
      }

      if(frag_info[frag_context].reassembled_len >= frag_size
         || frag_info[frag_context].last_fragment) {
        last_fragment = 1;
      }
      is_fragment = 1;
      break;

    default:
      /* If there is no fragmentation header, then assume that the packet
       * is not fragmented and pass it as is to IP stack.
       */
      buf = copy_buf(mbuf);
      if(!buf || net_driver_15_4_recv(buf) < 0) {
        goto fail;
      }
      goto out;
  }

  /*
   * copy "payload" from the packetbuf buffer to the sicslowpan_buf
   * if this is a first fragment or not fragmented packet,
   * we have already copied the compressed headers, uncomp_hdr_len
   * and packetbuf_hdr_len are non 0, frag_offset is.
   * If this is a subsequent fragment, this is the contrary.
   */
  if(packetbuf_datalen(mbuf) < uip_packetbuf_hdr_len(mbuf)) {
    PRINTF("reassemble: packet dropped due to header > total packet\n");
    goto fail;
  }

  uip_packetbuf_payload_len(mbuf) = packetbuf_datalen(mbuf) - uip_packetbuf_hdr_len(mbuf);

  /* Sanity-check size of incoming packet to avoid buffer overflow */
  {
    int req_size = UIP_LLH_LEN + (uint16_t)(frag_offset << 3)
        + uip_packetbuf_payload_len(mbuf);

    if(req_size > UIP_BUFSIZE) {
      PRINTF("reassemble: packet dropped, minimum required IP_BUF size: %d+%d+%d=%d (current size: %d)\n", UIP_LLH_LEN, (uint16_t)(frag_offset << 3),
              uip_packetbuf_payload_len(mbuf), req_size, UIP_BUFSIZE);
      goto fail;
    }
  }

  if(frag_size > 0) {
    /* Add the size of the header only for the first fragment. */
    if(first_fragment != 0) {
      frag_info[frag_context].reassembled_len = uip_uncomp_hdr_len(mbuf) + uip_packetbuf_payload_len(mbuf);
    }

    /* For the last fragment, we are OK if there is extrenous bytes at
       the end of the packet. */
    if(last_fragment != 0) {
      frag_info[frag_context].reassembled_len = frag_size;
      /* copy to uip(net_buf) */
      buf = copy_frags2uip(frag_context);
      if(!buf)
        goto fail;
    }
  }

  /*
   * If we have a full IP packet in sicslowpan_buf, deliver it to
   * the IP stack
   */
  if(!is_fragment || last_fragment) {
    /* packet is in uip already - just set length */
    if(is_fragment != 0 && last_fragment != 0) {
      uip_len(buf) = frag_size;
    } else {
      uip_len(buf) = uip_packetbuf_payload_len(mbuf) + uip_uncomp_hdr_len(mbuf);
    }

    PRINTF("reassemble: IP packet ready (length %d)\n", uip_len(buf));

    if(net_driver_15_4_recv(buf) < 0) {
      goto fail;
    }
  }

out:
  /* free MAC buffer */
  l2_buf_unref(mbuf);
  return 1;

fail:
   if(buf) {
     ip_buf_unref(buf);
   }
   return 0;
}

const struct fragmentation sicslowpan_fragmentation = {
	fragment,
	reassemble
};
