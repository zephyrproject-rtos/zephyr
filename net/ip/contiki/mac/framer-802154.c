/*
 * Copyright (c) 2009, Swedish Institute of Computer Science.
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
 */

/**
 * \file
 *         MAC framer for IEEE 802.15.4
 * \author
 *         Niclas Finne <nfi@sics.se>
 *         Joakim Eriksson <joakime@sics.se>
 */

#include <net/l2_buf.h>

#include "contiki/mac/framer-802154.h"
#include "contiki/mac/frame802154.h"
#include "contiki/llsec/llsec802154.h"
#include "contiki/packetbuf.h"
#include "lib/random.h"
#include <string.h>

#ifdef CONFIG_NETWORK_IP_STACK_DEBUG_15_4_FRAMING
#define DEBUG 1
#endif
#include "contiki/ip/uip-debug.h"

/**  \brief The 16-bit identifier of the PAN on which the device is
 *   sending to.  If this value is 0xffff, the device is not
 *   associated.
 */
static uint16_t mac_dst_pan_id = IEEE802154_PANID;

/**  \brief The 16-bit identifier of the PAN on which the device is
 *   operating.  If this value is 0xffff, the device is not
 *   associated.
 */
static uint16_t mac_src_pan_id = IEEE802154_PANID;

#ifdef FRAMER_802154_HANDLER
int FRAMER_802154_HANDLER(frame802154_t *frame);
#endif

/**  \brief The sequence number (0x00 - 0xff) added to the transmitted
 *   data or MAC command frame. The default is a random value within
 *   the range.
 */
uint8_t
framer_802154_next_seqno(void)
{
  static uint8_t mac_dsn;
  static uint8_t initialized = 0;
  if(!initialized) {
    initialized = 1;
    mac_dsn = random_rand() & 0xff;
  }
  /* Ensure that the sequence number 0 is not used as it would bypass the PACKETBUF_PARAM_MAC_SEQNO check. */
  if(mac_dsn == 0) {
    mac_dsn++;
  }
  return mac_dsn++;
}
/*---------------------------------------------------------------------------*/
uint16_t
framer_802154_get_pan_id(void)
{
  return mac_src_pan_id;
}
/*---------------------------------------------------------------------------*/
void
framer_802154_set_pan_id(uint16_t pan)
{
  mac_dst_pan_id = pan;
  mac_src_pan_id = pan;
}
/*---------------------------------------------------------------------------*/
static int
is_broadcast_addr(uint8_t mode, uint8_t *addr)
{
  int i = mode == FRAME802154_SHORTADDRMODE ? 2 : 8;
  while(i-- > 0) {
    if(addr[i] != 0xff) {
      return 0;
    }
  }
  return 1;
}
/*---------------------------------------------------------------------------*/
static int
create_frame(struct net_buf *buf, int do_create)
{
  frame802154_t params;
  int hdr_len;

  /* init to zeros */
  memset(&params, 0, sizeof(params));

  /* Build the FCF. */
  params.fcf.frame_type = packetbuf_attr(buf, PACKETBUF_ATTR_FRAME_TYPE);
  params.fcf.frame_pending = packetbuf_attr(buf, PACKETBUF_ATTR_PENDING);
  if(packetbuf_holds_broadcast(buf)) {
    params.fcf.ack_required = 0;
  } else {
    params.fcf.ack_required = packetbuf_attr(buf, PACKETBUF_ATTR_MAC_ACK);
  }
  params.fcf.panid_compression = 0;

  /* Insert IEEE 802.15.4 (2006) version bits. */
  params.fcf.frame_version = FRAME802154_IEEE802154_2006;
  
#if LLSEC802154_SECURITY_LEVEL
  if(packetbuf_attr(buf, PACKETBUF_ATTR_SECURITY_LEVEL)) {
    params.fcf.security_enabled = 1;
  }
  /* Setting security-related attributes */
  params.aux_hdr.security_control.security_level = packetbuf_attr(buf, PACKETBUF_ATTR_SECURITY_LEVEL);
  params.aux_hdr.frame_counter.u16[0] = packetbuf_attr(buf, PACKETBUF_ATTR_FRAME_COUNTER_BYTES_0_1);
  params.aux_hdr.frame_counter.u16[1] = packetbuf_attr(buf, PACKETBUF_ATTR_FRAME_COUNTER_BYTES_2_3);
#if LLSEC802154_USES_EXPLICIT_KEYS
  params.aux_hdr.security_control.key_id_mode = packetbuf_attr(buf, PACKETBUF_ATTR_KEY_ID_MODE);
  params.aux_hdr.key_index = packetbuf_attr(buf, PACKETBUF_ATTR_KEY_INDEX);
  params.aux_hdr.key_source.u16[0] = packetbuf_attr(buf, PACKETBUF_ATTR_KEY_SOURCE_BYTES_0_1);
#endif /* LLSEC802154_USES_EXPLICIT_KEYS */
#endif /* LLSEC802154_SECURITY_LEVEL */

  /* Increment and set the data sequence number. */
  if(!do_create) {
    /* Only length calculation - no sequence number is needed and
       should not be consumed. */

  } else if(packetbuf_attr(buf, PACKETBUF_ATTR_MAC_SEQNO)) {
    params.seq = packetbuf_attr(buf, PACKETBUF_ATTR_MAC_SEQNO);

  } else {
    params.seq = framer_802154_next_seqno();
    packetbuf_set_attr(buf, PACKETBUF_ATTR_MAC_SEQNO, params.seq);
  }

  /* Complete the addressing fields. */
  /**
     \todo For phase 1 the addresses are all long. We'll need a mechanism
     in the rime attributes to tell the mac to use long or short for phase 2.
  */
  if(LINKADDR_SIZE == 2) {
    /* Use short address mode if linkaddr size is short. */
    params.fcf.src_addr_mode = FRAME802154_SHORTADDRMODE;
  } else {
    params.fcf.src_addr_mode = FRAME802154_LONGADDRMODE;
  }
  params.dest_pid = mac_dst_pan_id;

  if(packetbuf_holds_broadcast(buf)) {
    /* Broadcast requires short address mode. */
    params.fcf.dest_addr_mode = FRAME802154_SHORTADDRMODE;
    params.dest_addr[0] = 0xFF;
    params.dest_addr[1] = 0xFF;

  } else {
    linkaddr_copy((linkaddr_t *)&params.dest_addr,
                  packetbuf_addr(buf, PACKETBUF_ADDR_RECEIVER));
    /* Use short address mode if linkaddr size is small */
    if(LINKADDR_SIZE == 2) {
      params.fcf.dest_addr_mode = FRAME802154_SHORTADDRMODE;
    } else {
      params.fcf.dest_addr_mode = FRAME802154_LONGADDRMODE;
    }
  }

  /* Set the source PAN ID to the global variable. */
  params.src_pid = mac_src_pan_id;

  /*
   * Set up the source address using only the long address mode for
   * phase 1.
   */
  linkaddr_copy((linkaddr_t *)&params.src_addr, &linkaddr_node_addr);

  params.payload = packetbuf_dataptr(buf);
  params.payload_len = packetbuf_datalen(buf);
  hdr_len = frame802154_hdrlen(&params);
  if(!do_create) {
    /* Only calculate header length */
    return hdr_len;

  } else if(packetbuf_hdralloc(buf, hdr_len)) {
    frame802154_create(&params, packetbuf_hdrptr(buf), hdr_len);

    PRINTF("15.4-OUT: %2X ", params.fcf.frame_type);
    PRINTLLADDR((const uip_lladdr_t *)params.dest_addr);
    PRINTF(" %d %u (%u)\n", hdr_len, packetbuf_datalen(buf), packetbuf_totlen(buf));

    return hdr_len;
  } else {
    PRINTF("15.4-OUT: too large header: %u\n", hdr_len);
    return FRAMER_FAILED;
  }
}
/*---------------------------------------------------------------------------*/
static int
hdr_length(struct net_buf *buf)
{
  return create_frame(buf, 0);
}
/*---------------------------------------------------------------------------*/
static int
create(struct net_buf *buf)
{
  return create_frame(buf, 1);
}
/*---------------------------------------------------------------------------*/
static int
parse(struct net_buf *buf)
{
  frame802154_t frame;
  int hdr_len;
  
  hdr_len = frame802154_parse(packetbuf_dataptr(buf), packetbuf_datalen(buf), &frame);
  
  if(hdr_len && packetbuf_hdrreduce(buf, hdr_len)) {
    packetbuf_set_attr(buf, PACKETBUF_ATTR_FRAME_TYPE, frame.fcf.frame_type);
    
    if(frame.fcf.dest_addr_mode) {
      if(frame.dest_pid != mac_src_pan_id &&
          frame.dest_pid != FRAME802154_BROADCASTPANDID) {
        /* Packet to another PAN */
        PRINTF("15.4: for another pan %u (0x%x)\n", frame.dest_pid,
               frame.dest_pid);
        return FRAMER_FAILED;
      }
      if(!is_broadcast_addr(frame.fcf.dest_addr_mode, frame.dest_addr)) {
        packetbuf_set_addr(buf, PACKETBUF_ADDR_RECEIVER, (linkaddr_t *)&frame.dest_addr);
      }
    }
    packetbuf_set_addr(buf, PACKETBUF_ADDR_SENDER, (linkaddr_t *)&frame.src_addr);
    packetbuf_set_attr(buf, PACKETBUF_ATTR_PENDING, frame.fcf.frame_pending);
    /*    packetbuf_set_attr(PACKETBUF_ATTR_RELIABLE, frame.fcf.ack_required);*/
    packetbuf_set_attr(buf, PACKETBUF_ATTR_PACKET_ID, frame.seq);
    
#if LLSEC802154_SECURITY_LEVEL
    if(frame.fcf.security_enabled) {
      packetbuf_set_attr(buf, PACKETBUF_ATTR_SECURITY_LEVEL, frame.aux_hdr.security_control.security_level);
      packetbuf_set_attr(buf, PACKETBUF_ATTR_FRAME_COUNTER_BYTES_0_1, frame.aux_hdr.frame_counter.u16[0]);
      packetbuf_set_attr(buf, PACKETBUF_ATTR_FRAME_COUNTER_BYTES_2_3, frame.aux_hdr.frame_counter.u16[1]);
#if LLSEC802154_USES_EXPLICIT_KEYS
      packetbuf_set_attr(buf, PACKETBUF_ATTR_KEY_ID_MODE, frame.aux_hdr.security_control.key_id_mode);
      packetbuf_set_attr(buf, PACKETBUF_ATTR_KEY_INDEX, frame.aux_hdr.key_index);
      packetbuf_set_attr(buf, PACKETBUF_ATTR_KEY_SOURCE_BYTES_0_1, frame.aux_hdr.key_source.u16[0]);
#endif /* LLSEC802154_USES_EXPLICIT_KEYS */
    }
#endif /* LLSEC802154_SECURITY_LEVEL */

    PRINTF("15.4-IN: %2X ", frame.fcf.frame_type);
    PRINTLLADDR((const uip_lladdr_t *)packetbuf_addr(buf, PACKETBUF_ADDR_SENDER));
    PRINTF(" ");
    PRINTLLADDR((const uip_lladdr_t *)packetbuf_addr(buf, PACKETBUF_ADDR_RECEIVER));
    PRINTF(" %d %u (%u)\n", hdr_len, packetbuf_datalen(buf), packetbuf_totlen(buf));

#ifdef FRAMER_802154_HANDLER
    if(FRAMER_802154_HANDLER(&frame)) {
      return FRAMER_FRAME_HANDLED;
    }
#endif
    
    return hdr_len;
  }
  return FRAMER_FAILED;
}
/*---------------------------------------------------------------------------*/
const struct framer framer_802154 = {
  hdr_length,
  create,
  framer_canonical_create_and_secure,
  parse
};
/*---------------------------------------------------------------------------*/
