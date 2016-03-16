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
 *         MAC interface for packaging radio packets into 802.15.4 frames
 *
 * \author
 *         Adam Dunkels <adam@sics.se>
 *         Eric Gnoske <egnoske@gmail.com>
 *         Blake Leverett <bleverett@gmail.com>
 *         Niclas Finne <nfi@sics.se>
 *         Joakim Eriksson <joakime@sics.se>
 */

#include <string.h>

#include <net/l2_buf.h>

#include "contiki/mac/sicslowmac/sicslowmac.h"
#include "contiki/mac/frame802154.h"
#include "contiki/packetbuf.h"
#include "contiki/queuebuf.h"
#include "contiki/netstack.h"
#include "lib/random.h"

#ifdef CONFIG_NETWORK_IP_STACK_DEBUG_15_4_MAC
#define DEBUG 1
#endif
#include "contiki/ip/uip-debug.h"

#if UIP_LOGGING
#include <stdio.h>
void uip_log(char *msg);
#define UIP_LOG(m) uip_log(m)
#else
#define UIP_LOG(m)
#endif

/**  \brief The sequence number (0x00 - 0xff) added to the transmitted
 *   data or MAC command frame. The default is a random value within
 *   the range.
 */
static uint8_t mac_dsn;

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
static uint8_t
send_packet(struct net_buf *buf, mac_callback_t sent, void *ptr)
{
  frame802154_t params;
  uint8_t len;
  uint8_t ret = 0;

  /* init to zeros */
  memset(&params, 0, sizeof(params));

  /* Build the FCF. */
  params.fcf.frame_type = FRAME802154_DATAFRAME;
  params.fcf.security_enabled = 0;
  params.fcf.frame_pending = 0;
  params.fcf.ack_required = packetbuf_attr(buf, PACKETBUF_ATTR_RELIABLE);
  params.fcf.panid_compression = 0;

  /* Insert IEEE 802.15.4 (2003) version bit. */
  params.fcf.frame_version = FRAME802154_IEEE802154_2003;

  /* Increment and set the data sequence number. */
  params.seq = mac_dsn++;

  /* Complete the addressing fields. */
  /**
     \todo For phase 1 the addresses are all long. We'll need a mechanism
     in the rime attributes to tell the mac to use long or short for phase 2.
  */
  params.fcf.src_addr_mode = FRAME802154_LONGADDRMODE;
  params.dest_pid = mac_dst_pan_id;

  if(packetbuf_holds_broadcast(buf)) {
    /* Broadcast requires short address mode. */
    params.fcf.dest_addr_mode = FRAME802154_SHORTADDRMODE;
    params.dest_addr[0] = 0xFF;
    params.dest_addr[1] = 0xFF;

  } else {
    linkaddr_copy((linkaddr_t *)&params.dest_addr,
                  packetbuf_addr(buf, PACKETBUF_ADDR_RECEIVER));
    params.fcf.dest_addr_mode = FRAME802154_LONGADDRMODE;
  }

  /* Set the source PAN ID to the global variable. */
  params.src_pid = mac_src_pan_id;

  /*
   * Set up the source address using only the long address mode for
   * phase 1.
   */
#if NETSTACK_CONF_BRIDGE_MODE
  linkaddr_copy((linkaddr_t *)&params.src_addr,packetbuf_addr(PACKETBUF_ADDR_SENDER));
#else
  linkaddr_copy((linkaddr_t *)&params.src_addr, &linkaddr_node_addr);
#endif

  params.payload = packetbuf_dataptr(buf);
  params.payload_len = packetbuf_datalen(buf);
  len = frame802154_hdrlen(&params);
  if(packetbuf_hdralloc(buf, len)) {
    frame802154_create(&params, packetbuf_hdrptr(buf), len);

    PRINTF("6MAC-UT: type %X dest ", params.fcf.frame_type);
    PRINTLLADDR((uip_lladdr_t *)params.dest_addr);
    PRINTF(" len %u datalen %u (totlen %u)\n", len, packetbuf_datalen(buf),
	   packetbuf_totlen(buf));

    ret = NETSTACK_RADIO.send(buf, packetbuf_hdrptr(buf), packetbuf_totlen(buf));
    if(sent) {
      switch(ret) {
      case RADIO_TX_OK:
        sent(buf, ptr, MAC_TX_OK, 1);
        break;
      case RADIO_TX_ERR:
        sent(buf, ptr, MAC_TX_ERR, 1);
        break;
      case RADIO_TX_COLLISION:
        sent(buf, ptr, MAC_TX_COLLISION, 1);
        break;
      }
    }
  } else {
    PRINTF("6MAC-UT: too large header: %u\n", len);
  }

  return ret;
}
/*---------------------------------------------------------------------------*/
uint8_t
send_list(struct net_buf *buf, mac_callback_t sent, void *ptr, struct rdc_buf_list *buf_list)
{
  if(buf_list != NULL) {
    queuebuf_to_packetbuf(buf, buf_list->buf);
    if (!send_packet(buf, sent, ptr)) {
      return 0;
    }
  }

  return 1;
}
/*---------------------------------------------------------------------------*/
static uint8_t
input_packet(struct net_buf *buf)
{
  frame802154_t frame;
  int len;

  len = packetbuf_datalen(buf);
  PRINTF("6MAC: received %d bytes\n", len);

  if(frame802154_parse(packetbuf_dataptr(buf), len, &frame) &&
     packetbuf_hdrreduce(buf, len - frame.payload_len)) {
    if(frame.fcf.dest_addr_mode) {
      if(frame.dest_pid != mac_src_pan_id &&
         frame.dest_pid != FRAME802154_BROADCASTPANDID) {
        /* Not broadcast or for our PAN */
        PRINTF("6MAC: for another pan %u (0x%x)\n", frame.dest_pid,
               frame.dest_pid);
        goto error;
      }
      if(!is_broadcast_addr(frame.fcf.dest_addr_mode, frame.dest_addr)) {
        packetbuf_set_addr(buf, PACKETBUF_ADDR_RECEIVER, (linkaddr_t *)&frame.dest_addr);
#if !NETSTACK_CONF_BRIDGE_MODE
        if(!linkaddr_cmp(packetbuf_addr(buf, PACKETBUF_ADDR_RECEIVER),
                         &linkaddr_node_addr)) {
          /* Not for this node */
	  PRINTF("6MAC: not for us, we are ");
	  PRINTLLADDR((uip_lladdr_t *)&linkaddr_node_addr);
	  PRINTF(" recipient is ");
	  PRINTLLADDR((uip_lladdr_t *)packetbuf_addr(buf, PACKETBUF_ADDR_RECEIVER));
	  PRINTF("\n");
	  goto error;
        }
#endif
      }
    }
    packetbuf_set_addr(buf, PACKETBUF_ADDR_SENDER, (linkaddr_t *)&frame.src_addr);

    PRINTF("6MAC-IN: type 0x%X sender ", frame.fcf.frame_type);
    PRINTLLADDR((uip_lladdr_t *)packetbuf_addr(buf, PACKETBUF_ADDR_SENDER));
    PRINTF(" receiver ");
    PRINTLLADDR((uip_lladdr_t *)packetbuf_addr(buf, PACKETBUF_ADDR_RECEIVER));
    PRINTF(" len %u\n", packetbuf_datalen(buf));
    return NETSTACK_MAC.input(buf);
  } else {
    PRINTF("6MAC: failed to parse hdr\n");
  }
error:
  return 0;
}
/*---------------------------------------------------------------------------*/
static int
on(void)
{
  return NETSTACK_RADIO.on();
}
/*---------------------------------------------------------------------------*/
static int
off(int keep_radio_on)
{
  if(keep_radio_on) {
    return NETSTACK_RADIO.on();
  } else {
    return NETSTACK_RADIO.off();
  }
}
/*---------------------------------------------------------------------------*/
static void
init(void)
{
  mac_dsn = random_rand() % 256;

  NETSTACK_RADIO.on();
}
/*---------------------------------------------------------------------------*/
static unsigned short
channel_check_interval(void)
{
  return 0;
}
/*---------------------------------------------------------------------------*/
const struct rdc_driver sicslowmac_driver = {
  "sicslowmac",
  init,
  send_packet,
  send_list,
  input_packet,
  on,
  off,
  channel_check_interval
};
/*---------------------------------------------------------------------------*/
