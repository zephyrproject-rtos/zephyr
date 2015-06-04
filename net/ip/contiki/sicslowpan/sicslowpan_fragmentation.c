/* compression.h - Header Compression */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Intel Corporation nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <string.h>

#include <net/net_buf.h>
#include <net_driver_15_4.h>
#include <net/sicslowpan/sicslowpan_fragmentation.h>
#include <net/netstack.h>
#include "net/packetbuf.h"
#include "net/queuebuf.h"
#include "net/ip/uip.h"
#include "net/ip/tcpip.h"
#include "dev/watchdog.h"

#include "contiki/ipv6/uip-ds6-nbr.h"

#define DEBUG 0
#include "net/ip/uip-debug.h"
#if DEBUG
/* PRINTFI and PRINTFO are defined for input and output to debug one without changing the timing of the other */
uint8_t p;
#include <stdio.h>
#define PRINTFI(...) PRINTF(__VA_ARGS__)
#define PRINTFO(...) PRINTF(__VA_ARGS__)
#define PRINTPACKETBUF() PRINTF("packetbuf buffer: "); for(p = 0; p < packetbuf_datalen(); p++){PRINTF("%.2X", *(packetbuf_ptr + p));} PRINTF("\n")
#define PRINTUIPBUF() PRINTF("UIP buffer: "); for(p = 0; p < uip_len; p++){PRINTF("%.2X", uip_buf[p]);}PRINTF("\n")
#define PRINTSICSLOWPANBUF() PRINTF("SICSLOWPAN buffer: "); for(p = 0; p < sicslowpan_len; p++){PRINTF("%.2X", sicslowpan_buf[p]);}PRINTF("\n")
#else
#define PRINTFI(...)
#define PRINTFO(...)
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
#define SICSLOWPAN_IP_BUF(buf)   ((struct uip_ip_hdr *)&sicslowpan_buf(buf)[UIP_LLH_LEN])
#define SICSLOWPAN_UDP_BUF(buf) ((struct uip_udp_hdr *)&sicslowpan_buf(buf)[UIP_LLIPH_LEN])

#define UIP_IP_BUF(buf)          ((struct uip_ip_hdr *)&uip_buf(buf)[UIP_LLH_LEN])
#define UIP_UDP_BUF(buf)          ((struct uip_udp_hdr *)&uip_buf(buf)[UIP_LLIPH_LEN])
#define UIP_TCP_BUF(buf)          ((struct uip_tcp_hdr *)&uip_buf(buf)[UIP_LLIPH_LEN])
#define UIP_ICMP_BUF(buf)          ((struct uip_icmp_hdr *)&uip_buf(buf)[UIP_LLIPH_LEN])
/** @} */

#define sicslowpan_buf uip_buf
#define sicslowpan_len uip_len

/** \brief Maximum available size for frame headers,
           link layer security-related overhead, as well as
           6LoWPAN payload. */
#ifdef SICSLOWPAN_CONF_MAC_MAX_PAYLOAD
#define MAC_MAX_PAYLOAD SICSLOWPAN_CONF_MAC_MAX_PAYLOAD
#else /* SICSLOWPAN_CONF_MAC_MAX_PAYLOAD */
#define MAC_MAX_PAYLOAD (127 - 2)
#endif /* SICSLOWPAN_CONF_MAC_MAX_PAYLOAD */

/** \name Fragmentation related variables
 *  @{
 */
//static uint16_t sicslowpan_len;

/**
 * The buffer used for the 6lowpan reassembly.
 * This buffer contains only the IPv6 packet (no MAC header, 6lowpan, etc).
 * It has a fix size as we do not use dynamic memory allocation.
 */
//static uip_buf_t sicslowpan_aligned_buf;
//#define sicslowpan_buf (sicslowpan_aligned_buf.u8)

/** The total length of the IPv6 packet in the sicslowpan_buf. */

/**
 * length of the ip packet already sent / received.
 * It includes IP and transport headers.
 */
static uint16_t processed_ip_in_len;

/** Datagram tag to be put in the fragments I send. */
static uint16_t my_tag;

/** When reassembling, the tag in the fragments being merged. */
static uint16_t reass_tag;

/** When reassembling, the source address of the fragments being merged */
linkaddr_t frag_sender;

/** Reassembly %process %timer. */
/*static struct timer reass_timer;*/

/** @} */

#define sicslowpan_buf uip_buf
#define sicslowpan_len uip_len

static void
packet_sent(struct net_mbuf *buf, void *ptr, int status, int transmissions)
{
  const linkaddr_t *dest = packetbuf_addr(buf, PACKETBUF_ADDR_RECEIVER);
  uip_ds6_link_neighbor_callback(dest, status, transmissions);
  uip_last_tx_status(buf) = status;
}

/*--------------------------------------------------------------------*/
/**
 * \brief This function is called by the 6lowpan code to send out a
 * packet.
 * \param dest the link layer destination address of the packet
 */
static void
send_packet(struct net_mbuf *buf, linkaddr_t *dest, bool last_fragment, void *ptr)
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

   /* Number of bytes processed. */
   uint16_t processed_ip_out_len;
   struct net_mbuf *mbuf;
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

  PRINTFO("max_payload: %d, framer_hdrlen: %d \n",max_payload, framer_hdrlen);

  mbuf = net_mbuf_get_reserve(0);
  if (!mbuf) {
     goto fail;
  }

  /*
   * The destination address will be tagged to each outbound
   * packet. If the argument localdest is NULL, we are sending a
   * broadcast packet.
   */

  if((int)uip_len(buf) <= max_payload) {
    /* The packet does not need to be fragmented, send buf */
    packetbuf_copyfrom(mbuf, uip_buf(buf), uip_len(buf));
    packetbuf_set_addr(mbuf, PACKETBUF_ADDR_RECEIVER, &buf->dest);
    net_buf_put(buf);
    NETSTACK_LLSEC.send(mbuf, &packet_sent, true, ptr);
    return 1;
   }

    uip_uncomp_hdr_len(mbuf) = 0;
    uip_packetbuf_hdr_len(mbuf) = 0;
    packetbuf_clear(mbuf);
    uip_packetbuf_ptr(mbuf) = packetbuf_dataptr(mbuf);

    PRINTFO("fragmentation: total packet len %d\n", uip_len(buf));

    /*
     * The outbound IPv6 packet is too large to fit into a single 15.4
     * packet, so we fragment it into multiple packets and send them.
     * The first fragment contains frag1 dispatch, then
     * IPv6/HC1/HC06/HC_UDP dispatchs/headers.
     * The following fragments contain only the fragn dispatch.
     */
    int estimated_fragments = ((int)uip_len(buf)) / ((int)MAC_MAX_PAYLOAD - SICSLOWPAN_FRAGN_HDR_LEN) + 1;
    int freebuf = queuebuf_numfree(mbuf) - 1;
    PRINTFO("uip_len: %d, fragments: %d, free bufs: %d\n", uip_len(buf), estimated_fragments, freebuf);
    if(freebuf < estimated_fragments) {
      PRINTFO("Dropping packet, not enough free bufs\n");
      goto fail;
    }

    /* Create 1st Fragment */
    PRINTFO("fragmentation: fragment %d \n", my_tag);

    SET16(uip_packetbuf_ptr(mbuf), PACKETBUF_FRAG_DISPATCH_SIZE,
          ((SICSLOWPAN_DISPATCH_FRAG1 << 8) | uip_len(buf)));

/*     PACKETBUF_FRAG_BUF->tag = uip_htons(my_tag); */
    SET16(uip_packetbuf_ptr(mbuf), PACKETBUF_FRAG_TAG, my_tag);
    my_tag++;

    /* Copy payload and send */
    uip_packetbuf_hdr_len(mbuf) += SICSLOWPAN_FRAG1_HDR_LEN;
    uip_packetbuf_payload_len(mbuf) = (max_payload - uip_packetbuf_hdr_len(mbuf)) & 0xfffffff8;
    PRINTFO("(payload len %d, hdr len %d, tag %d)\n",
               uip_packetbuf_payload_len(mbuf), uip_packetbuf_hdr_len(mbuf), my_tag);

    memcpy(uip_packetbuf_ptr(mbuf) + uip_packetbuf_hdr_len(mbuf),
              uip_buf(buf), uip_packetbuf_payload_len(mbuf));
    packetbuf_set_datalen(mbuf, uip_packetbuf_payload_len(mbuf) + uip_packetbuf_hdr_len(mbuf));
    q = queuebuf_new_from_packetbuf(mbuf);
    if(q == NULL) {
      PRINTFO("could not allocate queuebuf for first fragment, dropping packet\n");
      goto fail;
    }
    send_packet(mbuf, &buf->dest, last_fragment, ptr);
    queuebuf_to_packetbuf(mbuf, q);
    queuebuf_free(q);
    q = NULL;

    /* Check tx result. */
    if((uip_last_tx_status(mbuf) == MAC_TX_COLLISION) ||
       (uip_last_tx_status(mbuf) == MAC_TX_ERR) ||
       (uip_last_tx_status(mbuf) == MAC_TX_ERR_FATAL)) {
      PRINTFO("error in fragment tx, dropping subsequent fragments.\n");
      goto fail;
    }

    /* set processed_ip_out_len to what we already sent from the IP payload*/
    processed_ip_out_len = uip_packetbuf_payload_len(mbuf);

    /*
     * Create following fragments
     * Datagram tag is already in the buffer, we need to set the
     * FRAGN dispatch and for each fragment, the offset
     */
    uip_packetbuf_hdr_len(mbuf) = SICSLOWPAN_FRAGN_HDR_LEN;
/*     PACKETBUF_FRAG_BUF->dispatch_size = */
/*       uip_htons((SICSLOWPAN_DISPATCH_FRAGN << 8) | uip_len(buf)); */
    SET16(uip_packetbuf_ptr(mbuf), PACKETBUF_FRAG_DISPATCH_SIZE,
          ((SICSLOWPAN_DISPATCH_FRAGN << 8) | uip_len(buf)));
    uip_packetbuf_payload_len(mbuf) = (max_payload - uip_packetbuf_hdr_len(mbuf)) & 0xfffffff8;

    while(processed_ip_out_len < uip_len(buf)) {
      PRINTFO("fragmentation: fragment:%d, processed_ip_out_len:%d \n", my_tag, processed_ip_out_len);
      uip_packetbuf_ptr(mbuf)[PACKETBUF_FRAG_OFFSET] = processed_ip_out_len >> 3;

      /* Copy payload and send */
      if(uip_len(buf) - processed_ip_out_len < uip_packetbuf_payload_len(mbuf)) {
        /* last fragment */
        last_fragment = true;
        uip_packetbuf_payload_len(mbuf) = uip_len(buf) - processed_ip_out_len;
      }
      PRINTFO("(offset %d, len %d, tag %d)\n",
             processed_ip_out_len >> 3, uip_packetbuf_payload_len(mbuf), my_tag);
      memcpy(uip_packetbuf_ptr(mbuf) + uip_packetbuf_hdr_len(mbuf),
             (uint8_t *)UIP_IP_BUF(buf) + processed_ip_out_len, uip_packetbuf_payload_len(mbuf));
      packetbuf_set_datalen(mbuf, uip_packetbuf_payload_len(mbuf) + uip_packetbuf_hdr_len(mbuf));
      q = queuebuf_new_from_packetbuf(mbuf);
      if(q == NULL) {
        PRINTFO("could not allocate queuebuf, dropping fragment\n");
        goto fail;
      }
      send_packet(mbuf, &buf->dest, last_fragment, ptr);
      queuebuf_to_packetbuf(mbuf, q);
      queuebuf_free(q);
      q = NULL;
      processed_ip_out_len += uip_packetbuf_payload_len(mbuf);

      /* Check tx result. */
      if((uip_last_tx_status(mbuf) == MAC_TX_COLLISION) ||
         (uip_last_tx_status(mbuf) == MAC_TX_ERR) ||
         (uip_last_tx_status(mbuf) == MAC_TX_ERR_FATAL)) {
        PRINTFO("error in fragment tx, dropping subsequent fragments.\n");
        goto fail;
      }
    }

    net_buf_put(buf);
    return 1;

fail:
    net_mbuf_put(mbuf);
    return 0;
}

static int reassemble(struct net_mbuf *mbuf)
{
  /* size of the IP packet (read from fragment) */
  uint16_t frag_size = 0;
  /* offset of the fragment in the IP packet */
  uint8_t frag_offset = 0;
  uint8_t is_fragment = 0;
  /* tag of the fragment */
  uint16_t frag_tag = 0;
  uint8_t first_fragment = 0, last_fragment = 0;
  static struct net_buf *buf = NULL;

  /* init */
  uip_uncomp_hdr_len(mbuf) = 0;
  uip_packetbuf_hdr_len(mbuf) = 0;

  /* The MAC puts the 15.4 payload inside the packetbuf data buffer */
  uip_packetbuf_ptr(mbuf) = packetbuf_dataptr(mbuf);

  /* Save the RSSI of the incoming packet in case the upper layer will
     want to query us for it later. */
  //last_rssi = (signed short)packetbuf_attr(mbuf, PACKETBUF_ATTR_RSSI);

  /* FIXME:if reassembly timed out, cancel it */
/*  if(timer_expired(&reass_timer)) {
    PRINTF("\n\ntimer expired \n\n");
    sicslowpan_len(mbuf) = 0;
    processed_ip_in_len = 0;
  }
*/
   /*
   * Since we don't support the mesh and broadcast header, the first header
   * we look for is the fragmentation header
   */
  switch((GET16(uip_packetbuf_ptr(mbuf), PACKETBUF_FRAG_DISPATCH_SIZE) & 0xf800) >> 8) {
    case SICSLOWPAN_DISPATCH_FRAG1:

      if (buf) {
         PRINTFI("net_buf %p already exits, freeing it!\n", buf);
         net_buf_put(buf);
      }
      /* reserve when first fragment appears */
      buf = net_buf_get_reserve(0);
      if(!buf) {
         return 0;
      }

      PRINTFI("reassemble: FRAG1 ");
      frag_offset = 0;
/*       frag_size = (uip_ntohs(PACKETBUF_FRAG_BUF->dispatch_size) & 0x07ff); */
      frag_size = GET16(uip_packetbuf_ptr(mbuf), PACKETBUF_FRAG_DISPATCH_SIZE) & 0x07ff;
/*       frag_tag = uip_ntohs(PACKETBUF_FRAG_BUF->tag); */
      frag_tag = GET16(uip_packetbuf_ptr(mbuf), PACKETBUF_FRAG_TAG);
      PRINTFI("size %d, tag %d, offset %d)\n",
             frag_size, frag_tag, frag_offset);
      uip_packetbuf_hdr_len(mbuf) += SICSLOWPAN_FRAG1_HDR_LEN;
      /*      printf("frag1 %d %d\n", reass_tag, frag_tag);*/
      first_fragment = 1;
      is_fragment = 1;
      break;
    case SICSLOWPAN_DISPATCH_FRAGN:
      /*
       * set offset, tag, size
       * Offset is in units of 8 bytes
       */
      PRINTFI("reasseble: FRAGN ");
      frag_offset = uip_packetbuf_ptr(mbuf)[PACKETBUF_FRAG_OFFSET];
      frag_tag = GET16(uip_packetbuf_ptr(mbuf), PACKETBUF_FRAG_TAG);
      frag_size = GET16(uip_packetbuf_ptr(mbuf), PACKETBUF_FRAG_DISPATCH_SIZE) & 0x07ff;
      PRINTFI("reassemble: size %d, tag %d, offset %d)\n",
             frag_size, frag_tag, frag_offset);
      uip_packetbuf_hdr_len(mbuf) += SICSLOWPAN_FRAGN_HDR_LEN;

      /* If this is the last fragment, we may shave off any extrenous
         bytes at the end. We must be liberal in what we accept. */
      PRINTFI("reassemble: last_fragment?: processed_ip_in_len %d packetbuf_payload_len %d frag_size %d\n",
      processed_ip_in_len, packetbuf_datalen(mbuf) - uip_packetbuf_hdr_len(mbuf), frag_size);

      if(processed_ip_in_len + packetbuf_datalen(mbuf) - uip_packetbuf_hdr_len(mbuf) >=
           frag_size) {
        last_fragment = 1;
      }
      is_fragment = 1;
      break;
    default:
      PRINTF("Unknown FRAG diapatch \n");
      goto fail;
  }

   if (!buf) {
       PRINTF("reassemble: net_buf not initialized\n");
       goto fail;
   }

  /* We are currently reassembling a packet, but have just received the first
   * fragment of another packet. We can either ignore it and hope to receive
   * the rest of the under-reassembly packet fragments, or we can discard the
   * previous packet altogether, and start reassembling the new packet.
   *
   * We discard the previous packet, and start reassembling the new packet.
   * This lessens the negative impacts of too high SICSLOWPAN_REASS_MAXAGE.
   */
#define PRIORITIZE_NEW_PACKETS 1
#if PRIORITIZE_NEW_PACKETS
  if(!is_fragment) {
    /* Prioritize non-fragment packets too. */
    sicslowpan_len(buf) = 0;
    processed_ip_in_len = 0;
  } else if(processed_ip_in_len > 0 && first_fragment
      && !linkaddr_cmp(&frag_sender, packetbuf_addr(mbuf, PACKETBUF_ADDR_SENDER))) {
    sicslowpan_len(buf) = 0;
    processed_ip_in_len = 0;
  }
#endif /* PRIORITIZE_NEW_PACKETS */

  if(processed_ip_in_len > 0) {
    /* reassembly is ongoing */
    /*    printf("frag %d %d\n", reass_tag, frag_tag);*/
    if((frag_size > 0 &&
        (frag_size != sicslowpan_len(buf) ||
         reass_tag  != frag_tag ||
         !linkaddr_cmp(&frag_sender, packetbuf_addr(mbuf, PACKETBUF_ADDR_SENDER))))  ||
       frag_size == 0) {
      /*
       * the packet is a fragment that does not belong to the packet
       * being reassembled or the packet is not a fragment.
       */

      PRINTFI("reassemble: Dropping 6lowpan packet that is not a fragment of the packet currently being reassembled\n");
      goto fail;
    }
  } else {
    /*
     * reassembly is off
     * start it if we received a fragment
     */
    if((frag_size > 0) && (frag_size <= UIP_BUFSIZE)) {
      /* We are currently not reassembling a packet, but have received a packet fragment
       * that is not the first one. */
      if(is_fragment && !first_fragment) {
        goto fail;
      }

      sicslowpan_len(buf) = frag_size;
      reass_tag = frag_tag;
      //timer_set(&reass_timer, SICSLOWPAN_REASS_MAXAGE * CLOCK_SECOND);
      PRINTFI("reassemble: INIT FRAGMENTATION (len %d, tag %d)\n",
             sicslowpan_len(buf), reass_tag);
      linkaddr_copy(&frag_sender, packetbuf_addr(mbuf, PACKETBUF_ADDR_SENDER));
    }
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
    if(req_size > sizeof(sicslowpan_buf(buf))) {
      PRINTF(
          "reassemble: packet dropped, minimum required SICSLOWPAN_IP_BUF size: %d+%d+%d=%d (current size: %d)\n",
          UIP_LLH_LEN, (uint16_t)(frag_offset << 3),
          uip_packetbuf_payload_len(mbuf), req_size, sizeof(sicslowpan_buf(buf)));
      goto fail;
    }
  }

  memcpy((uint8_t *)SICSLOWPAN_IP_BUF(buf) + uip_uncomp_hdr_len(mbuf) + (uint16_t)(frag_offset << 3), uip_packetbuf_ptr(mbuf) + uip_packetbuf_hdr_len(mbuf), uip_packetbuf_payload_len(mbuf));

  /* update processed_ip_in_len if fragment, sicslowpan_len otherwise */

  if(frag_size > 0) {
    /* For the last fragment, we are OK if there is extrenous bytes at
       the end of the packet. */
    if(last_fragment != 0) {
      processed_ip_in_len = frag_size;
    } else {
      processed_ip_in_len += uip_packetbuf_payload_len(mbuf);
    }
    PRINTF("reassemble: processed_ip_in_len %d, packetbuf_payload_len %d\n", processed_ip_in_len, uip_packetbuf_payload_len(mbuf));

  } else {
    sicslowpan_len(buf) = uip_packetbuf_payload_len(mbuf);
  }

  /*
   * If we have a full IP packet in sicslowpan_buf, deliver it to
   * the IP stack
   */
  PRINTF("reassemble: processed_ip_in_len %d, sicslowpan_len %d\n",
         processed_ip_in_len, sicslowpan_len(buf));
  if(processed_ip_in_len == 0 || (processed_ip_in_len == sicslowpan_len(buf))) {
    PRINTFI("reassemble: IP packet ready (length %d)\n",
           sicslowpan_len(buf));
    if (net_driver_15_4_recv(buf) < 0) {
        goto fail;
    } else {
      /* set to default after reassemble is completed */
      processed_ip_in_len = my_tag = reass_tag = 0;
    }
    buf = NULL;
  }

  /* free MAC buffer */
  net_mbuf_put(mbuf);
  return 1;

fail:
   /* set to default if reassemble is failed */
   processed_ip_in_len = my_tag = reass_tag = 0;
   net_buf_put(buf);
   buf = NULL;
   return 0;
}

const struct fragmentation sicslowpan_fragmentation = {
	fragment,
	reassemble
};
