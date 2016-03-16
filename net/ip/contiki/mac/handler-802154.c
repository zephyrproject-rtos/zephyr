/*
 * Copyright (c) 2013-2015, Yanzi Networks AB.
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
 *         A MAC handler for IEEE 802.15.4
 * \author
 *         Joakim Eriksson <joakime@sics.se>
 *         Niclas Finne <nfi@sics.se>
 */

#include <net/l2_buf.h>

#include "contiki/mac/framer-802154.h"
#include "contiki/mac/frame802154.h"
#include "contiki/mac/handler-802154.h"
#include "contiki/packetbuf.h"
#include "contiki/netstack.h"
#include "lib/random.h"
#include "sys/ctimer.h"
#include <string.h>
#include <stdio.h>

#ifdef CONFIG_NETWORK_IP_STACK_DEBUG_15_4_MAC
#define DEBUG 1
#endif
#include "contiki/ip/uip-debug.h"

#define CHANNEL_LOW  11
#define CHANNEL_HIGH 26

#if HANDLER_802154_CONF_STATS
handler_802154_stats_t handler_802154_stats;
#endif /* HANDLER_802154_CONF_STATS */

/* ----------------------------------------------------- */
/* variables for beacon req/resp - active scan */
/* ----------------------------------------------------- */
static int current_channel = 0;
static uint8_t chseqno = 0;
static int scan = 0; /* flag if scan is already active */
static struct ctimer scan_timer;
static struct ctimer beacon_send_timer;

static void handle_beacon(frame802154_t *frame);
static void handle_beacon_send_timer(struct net_buf *mbuf, void *p);
static int answer_beacon_requests;
static uint16_t panid;
static scan_callback_t callback;
static uint8_t beacon_payload[BEACON_PAYLOAD_BUFFER_SIZE] = { 0xfe, 0x00 };
static uint8_t beacon_payload_len = 1;
static int others_beacon_reply;

#ifndef  DISABLE_NETSELECT_RANDOM_CHANNEL
#define DISABLE_NETSELECT_RANDOM_CHANNEL 0
#endif /* DISABLE_NETSELECT_RANDOM_CHANNEL */
#if (DISABLE_NETSELECT_RANDOM_CHANNEL == 0)
uint8_t channel_list[CHANNEL_HIGH - CHANNEL_LOW + 1];
uint8_t channel_index;
#endif /* (DISABLE_NETSELECT_RANDOM_CHANNEL == 0) */
/*---------------------------------------------------------------------------*/
/* the init function assumes that the framer 802.15.4 is active and
   used */
void
handler_802154_join(uint16_t pid, int answer_beacons)
{
  panid = pid;
  framer_802154_set_pan_id(pid);
  answer_beacon_requests = answer_beacons;
}
/*---------------------------------------------------------------------------*/
static void
end_scan(void)
{
  scan = 0;
  NETSTACK_RADIO.set_value(RADIO_PARAM_RX_MODE,
                           RADIO_RX_MODE_ADDRESS_FILTER | RADIO_RX_MODE_AUTOACK);
  if(callback) {
    /* nothing more will be scanned now */
    callback(0, NULL, CALLBACK_ACTION_SCAN_END);
  }
}
/*---------------------------------------------------------------------------*/
static void
handle_beacon(frame802154_t *frame)
{
  radio_value_t value;
  PRINTF("Beacon received on channel: %d  PanID: %x\n", current_channel, frame->src_pid);
  if(answer_beacon_requests &&
     (frame->payload_len == beacon_payload_len) &&
     (memcmp(beacon_payload, frame->payload, beacon_payload_len) == 0)) {
    others_beacon_reply++;
    if(others_beacon_reply >= 3) {
      /* Abort scheduled beacon transmit */
      ctimer_stop(&beacon_send_timer);
    }
  }

  /* scanned channel is already increased... */
  if(callback) {
    /* pick the channel from the RADIO if it have been changed since we started
       the scan - e.g. if we get a beacon later */
    NETSTACK_RADIO.get_value(RADIO_PARAM_CHANNEL, &value);
    if(callback(value, frame, CALLBACK_ACTION_RX) == CALLBACK_STATUS_FINISHED) {
      /* scan is over - application is satisfied */
      end_scan();
    }
  }
}
/*---------------------------------------------------------------------------*/
static void
handle_beacon_send_timer(struct net_buf *buf, void *p)
{
  struct net_buf *mbuf;
  frame802154_t params;
  uint8_t len;

  mbuf = l2_buf_get_reserve(0);
  if(!mbuf) {
    return;
  }

  /* init to zeros */
  memset(&params, 0, sizeof(params));

  /* use packetbuf for sending ?? */
  packetbuf_clear(mbuf);
  /* Build the FCF. */
  params.fcf.frame_type = FRAME802154_BEACONFRAME;

  /* Insert IEEE 802.15.4 (2006) version bits. */
  params.fcf.frame_version = FRAME802154_IEEE802154_2006;

  /* assume long for now */
  params.fcf.src_addr_mode = FRAME802154_LONGADDRMODE;
  linkaddr_copy((linkaddr_t *)&params.src_addr, &linkaddr_node_addr);

  /* Set the source PAN ID to the global variable. */
  params.src_pid = panid;

  params.fcf.dest_addr_mode = FRAME802154_SHORTADDRMODE;
  params.dest_addr[0] = 0xFF;
  params.dest_addr[1] = 0xFF;

  params.dest_pid = 0xffff;

  params.seq = framer_802154_next_seqno();

  /* Calculate beacon length and copy it to packetbuf */
  beacon_payload_len = handler_802154_calculate_beacon_payload_length(beacon_payload, BEACON_PAYLOAD_BUFFER_SIZE);
  packetbuf_copyfrom(mbuf, beacon_payload, beacon_payload_len);

  /* Set payload and payload length */
  params.payload = packetbuf_dataptr(mbuf);
  params.payload_len = packetbuf_datalen(mbuf);

  len = frame802154_hdrlen(&params);
  if(packetbuf_hdralloc(mbuf, len)) {
    frame802154_create(&params, packetbuf_hdrptr(mbuf), len);
    if(NETSTACK_RADIO.send(mbuf, packetbuf_hdrptr(mbuf),
                  packetbuf_totlen(mbuf)) != RADIO_TX_OK) {
      l2_buf_unref(mbuf);
      return;
    }

    HANDLER_802154_STAT(handler_802154_stats.beacons_sent++);
  }
}
/*---------------------------------------------------------------------------*/
/* called to send a beacon request */
void
handler_802154_send_beacon_request(void)
{
  struct net_buf *mbuf;
  frame802154_t params;
  uint8_t len;

  mbuf = l2_buf_get_reserve(0);
  if(!mbuf) {
    return;
  }

  /* init to zeros */
  memset(&params, 0, sizeof(params));

  /* use packetbuf for sending ?? */
  packetbuf_clear(mbuf);
  /* Build the FCF. */
  params.fcf.frame_type = FRAME802154_CMDFRAME;

  /* Insert IEEE 802.15.4 (2006) version bits. */
  params.fcf.frame_version = FRAME802154_IEEE802154_2006;

  params.fcf.src_addr_mode = FRAME802154_NOADDR;

  params.fcf.dest_addr_mode = FRAME802154_SHORTADDRMODE;
  params.dest_addr[0] = 0xFF;
  params.dest_addr[1] = 0xFF;

  params.dest_pid = 0xffff;

  params.seq = chseqno;

  packetbuf_set_datalen(mbuf, 1);
  params.payload = packetbuf_dataptr(mbuf);
  /* set the type in the payload */
  params.payload[0] = FRAME802154_BEACONREQ;
  params.payload_len = packetbuf_datalen(mbuf);
  len = frame802154_hdrlen(&params);

  if(packetbuf_hdralloc(mbuf, len)) {
    frame802154_create(&params, packetbuf_hdrptr(mbuf), len);
    if(NETSTACK_RADIO.send(mbuf, packetbuf_hdrptr(mbuf),
             packetbuf_totlen(mbuf)) != RADIO_TX_OK) {
      l2_buf_unref(mbuf);
      return;
    }
    HANDLER_802154_STAT(handler_802154_stats.beacons_reqs_sent++);
  }
}
/*---------------------------------------------------------------------------*/
static void
handle_scan_timer(struct net_buf *mbuf, void *p)
{
  if(!scan) {
    return;
  }

  if(scan > 1) {
    int callback_status;
    callback_status = callback(current_channel, NULL, CALLBACK_ACTION_CHANNEL_DONE);
    if(callback_status == CALLBACK_STATUS_FINISHED) {
      end_scan();
      return;
    } else if(callback_status == CALLBACK_STATUS_NEED_MORE_TIME) {
      ctimer_reset(&scan_timer);
      return;
    }
  }

  scan++;

#if DISABLE_NETSELECT_RANDOM_CHANNEL
  if(current_channel == 26) {
    /* Last channel has been scanned. Report that the process is over. */
    end_scan();
    return;
  }

  if(current_channel == 0) {
    current_channel = 11;
  } else {
    current_channel++;
  }
#else /* DISABLE_NETSELECT_RANDOM_CHANNEL */
  if(channel_index > (CHANNEL_HIGH - CHANNEL_LOW)) {
    /* Last channel has been scanned. Report that the process is over. */
    end_scan();
    return;
  }

  if(current_channel == 0) {
    /* new scan, randomize channels */
    int i;
    int j;
    channel_index = 0;

    /* Initialize a random list as shown by Durstenfelds' Fisher–Yates shuffle. */
    for(i = 0; i <= (CHANNEL_HIGH - CHANNEL_LOW); i++) {
      j = random_rand() % (i + 1);
      if(j != i) {
        channel_list[i] = channel_list[j];
      }
      channel_list[j] = i + CHANNEL_LOW;
    }

#if DEBUG
    PRINTF("New netselect search: channel order: ");
    for(i = 0; i <= (CHANNEL_HIGH - CHANNEL_LOW); i++) {
      PRINTF("%d ", channel_list[i]);
    }
    PRINTF("\n");
#endif
  }
  current_channel = channel_list[channel_index++];
#endif /* DISABLE_NETSELECT_RANDOM_CHANNEL */

  NETSTACK_RADIO.set_value(RADIO_PARAM_RX_MODE, 0);
  NETSTACK_RADIO.set_value(RADIO_PARAM_CHANNEL, current_channel);

  handler_802154_send_beacon_request();

  ctimer_reset(&scan_timer);
}
/*---------------------------------------------------------------------------*/
/*
 * active scan - will scan all 16 channels in the 802.15.4 network
 * NOTE: this assumes 16 channels, starting from 11. Needs to be changed
 * if running on other than 2.4 GHz
 */
int
handler_802154_active_scan(scan_callback_t cb)
{
  /* if no scan is active - start one */
  if(!scan) {
    callback = cb;
    current_channel = 0;
#if (DISABLE_NETSELECT_RANDOM_CHANNEL == 0)
    channel_index = 0;
#endif /* (DISABLE_NETSELECT_RANDOM_CHANNEL == 0) */
    scan = 1;
    chseqno = framer_802154_next_seqno();
    NETSTACK_RADIO.set_value(RADIO_PARAM_RX_MODE, 0);
    ctimer_set(NULL, &scan_timer, (CLOCK_SECOND * 6) / 10, &handle_scan_timer, callback);
    return 1;
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
int
handler_802154_frame_received(frame802154_t *frame)
{
  if(answer_beacon_requests &&
     frame->fcf.frame_type == FRAME802154_CMDFRAME &&
     frame->payload[0] == FRAME802154_BEACONREQ) {
    others_beacon_reply = 0;
    ctimer_set(NULL, &beacon_send_timer,
               CLOCK_SECOND / 16 +
               (CLOCK_SECOND * (random_rand() & 0xff)) / 0x200,
               &handle_beacon_send_timer, NULL);
    return 1;
  }
  if(frame->fcf.frame_type == FRAME802154_BEACONFRAME) {
    HANDLER_802154_STAT(handler_802154_stats.beacons_received++);
    handle_beacon(frame);
    return 1;
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
/*
 * Calculate beacon payload length.
 * Return length or -1 on error.
 */
int
handler_802154_calculate_beacon_payload_length(uint8_t *beacon, int maxlen)
{
  int i;

  if(beacon[0] != 0xfe) {
    return -1;
  }

  for(i = 1; beacon[i] != 0; i += beacon[i]) {
    if(i >= maxlen) {
      return -1;
    }
  }
  i++; /* include the last 0-length indicator */
  return i;
}
/*---------------------------------------------------------------------------*/
void
handler_802154_set_answer_beacons(int answer)
{
  answer_beacon_requests = answer;
}
/*---------------------------------------------------------------------------*/
void
handler_802154_set_beacon_payload(uint8_t *payload, uint8_t len)
{
  if(len <= BEACON_PAYLOAD_BUFFER_SIZE) {
    beacon_payload_len = 1;
    memset(beacon_payload, 0, BEACON_PAYLOAD_BUFFER_SIZE);
    memcpy(beacon_payload, payload, len);
    beacon_payload_len = len;
  }
}
/*---------------------------------------------------------------------------*/
/*
 * Write current beacon payload length to "len", if non-NULL.
 *
 * Return pointer to beacon payload buffer.
 */
uint8_t *
handler_802154_get_beacon_payload(uint8_t *len)
{
  if(len) {
    *len = beacon_payload_len;
  }
  return beacon_payload;
}
/*---------------------------------------------------------------------------*/
