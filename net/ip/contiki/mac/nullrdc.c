/*
 * Copyright (c) 2010, Swedish Institute of Computer Science.
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
 *         A null RDC implementation that uses framer for headers.
 * \author
 *         Adam Dunkels <adam@sics.se>
 *         Niclas Finne <nfi@sics.se>
 */

#include <net/net_buf.h>

#include "net/mac/mac-sequence.h"
#include "net/mac/nullrdc.h"
#include "net/packetbuf.h"
#include "net/queuebuf.h"
#include "net/netstack.h"
#include "net/llsec/anti-replay.h"
#include <string.h>

#if CONTIKI_TARGET_COOJA
#include "lib/simEnvChange.h"
#endif /* CONTIKI_TARGET_COOJA */

#define DEBUG DEBUG_NONE
#include "net/ip/uip-debug.h"

#ifdef NULLRDC_CONF_ADDRESS_FILTER
#define NULLRDC_ADDRESS_FILTER NULLRDC_CONF_ADDRESS_FILTER
#else
#define NULLRDC_ADDRESS_FILTER 1
#endif /* NULLRDC_CONF_ADDRESS_FILTER */

#ifndef NULLRDC_802154_AUTOACK
#ifdef NULLRDC_CONF_802154_AUTOACK
#define NULLRDC_802154_AUTOACK NULLRDC_CONF_802154_AUTOACK
#else
#define NULLRDC_802154_AUTOACK 0
#endif /* NULLRDC_CONF_802154_AUTOACK */
#endif /* NULLRDC_802154_AUTOACK */

#ifndef NULLRDC_802154_AUTOACK_HW
#ifdef NULLRDC_CONF_802154_AUTOACK_HW
#define NULLRDC_802154_AUTOACK_HW NULLRDC_CONF_802154_AUTOACK_HW
#else
#define NULLRDC_802154_AUTOACK_HW 0
#endif /* NULLRDC_CONF_802154_AUTOACK_HW */
#endif /* NULLRDC_802154_AUTOACK_HW */

#if NULLRDC_802154_AUTOACK
#include "sys/rtimer.h"
#include "dev/watchdog.h"

#ifdef NULLRDC_CONF_ACK_WAIT_TIME
#define ACK_WAIT_TIME NULLRDC_CONF_ACK_WAIT_TIME
#else /* NULLRDC_CONF_ACK_WAIT_TIME */
#define ACK_WAIT_TIME                      RTIMER_SECOND / 2500
#endif /* NULLRDC_CONF_ACK_WAIT_TIME */
#ifdef NULLRDC_CONF_AFTER_ACK_DETECTED_WAIT_TIME
#define AFTER_ACK_DETECTED_WAIT_TIME NULLRDC_CONF_AFTER_ACK_DETECTED_WAIT_TIME
#else /* NULLRDC_CONF_AFTER_ACK_DETECTED_WAIT_TIME */
#define AFTER_ACK_DETECTED_WAIT_TIME       RTIMER_SECOND / 1500
#endif /* NULLRDC_CONF_AFTER_ACK_DETECTED_WAIT_TIME */
#endif /* NULLRDC_802154_AUTOACK */

#ifdef NULLRDC_CONF_SEND_802154_ACK
#define NULLRDC_SEND_802154_ACK NULLRDC_CONF_SEND_802154_ACK
#else /* NULLRDC_CONF_SEND_802154_ACK */
#define NULLRDC_SEND_802154_ACK 0
#endif /* NULLRDC_CONF_SEND_802154_ACK */

#if NULLRDC_SEND_802154_ACK
#include "net/mac/frame802154.h"
#endif /* NULLRDC_SEND_802154_ACK */

#define ACK_LEN 3

#ifdef NULLRDC_CONF_ENABLE_RETRANSMISSIONS
#define NULLRDC_ENABLE_RETRANSMISSIONS NULLRDC_CONF_ENABLE_RETRANSMISSIONS
#else
#define NULLRDC_ENABLE_RETRANSMISSIONS 0
#endif /* NULLRDC_ENABLE_RETRANSMISSIONS */

#if NULLRDC_ENABLE_RETRANSMISSIONS
#ifdef NULLRDC_CONF_MAX_RETRANSMISSIONS
#define NULLRDC_MAX_RETRANSMISSIONS NULLRDC_CONF_MAX_RETRANSMISSIONS
#else
#define NULLRDC_MAX_RETRANSMISSIONS 3
#endif /* NULLRDC_MAX_RETRANSMISSIONS */
#ifdef NULLRDC_CONF_TX_RETRY_DELAY_MS
#define NULLRDC_TX_RETRY_DELAY_MS NULLRDC_CONF_TX_RETRY_DELAY_MS
#else
#define NULLRDC_TX_RETRY_DELAY_MS 1
#endif /* NULLRDC_TX_RETRY_DELAY_MS */
#ifdef NULLRDC_CONF_ENABLE_RETRANSMISSIONS_BCAST
#define NULLRDC_ENABLE_RETRANSMISSIONS_BCAST NULLRDC_CONF_ENABLE_RETRANSMISSIONS_BCAST
#else
#define NULLRDC_ENABLE_RETRANSMISSIONS_BCAST 0
#endif /* NULLRDC_ENABLE_RETRANSMISSIONS_BCAST */
#endif/* NULLRDC_ENABLE_RETRANSMISSIONS */

/*---------------------------------------------------------------------------*/
static int
send_one_packet(struct net_mbuf *buf, mac_callback_t sent, void *ptr)
{
  int ret;
  int last_sent_ok = 0;
#if NULLRDC_ENABLE_RETRANSMISSIONS
  rtimer_clock_t target_time;
  uint8_t tx_attempts = 0;
  uint8_t max_tx_attempts;

  if(packetbuf_attr(PACKETBUF_ATTR_MAX_MAC_TRANSMISSIONS) > 0) {
    max_tx_attempts = packetbuf_attr(PACKETBUF_ATTR_MAX_MAC_TRANSMISSIONS);
  } else {
    max_tx_attempts = NULLRDC_MAX_RETRANSMISSIONS + 1;
  }
#endif /* NULLRDC_ENABLE_RETRANSMISSIONS */

  packetbuf_set_addr(buf, PACKETBUF_ADDR_SENDER, &linkaddr_node_addr);
#if NULLRDC_802154_AUTOACK || NULLRDC_802154_AUTOACK_HW
  packetbuf_set_attr(buf, PACKETBUF_ATTR_MAC_ACK, 1);
#endif /* NULLRDC_802154_AUTOACK || NULLRDC_802154_AUTOACK_HW */

  if(NETSTACK_FRAMER.create_and_secure(buf) < 0) {
    /* Failed to allocate space for headers */
    PRINTF("nullrdc: send failed, too large header\n");
    ret = MAC_TX_ERR_FATAL;
  } else {
#if NULLRDC_802154_AUTOACK
    int is_broadcast;
    uint8_t dsn;
    dsn = ((uint8_t *)packetbuf_hdrptr())[2] & 0xff;

    NETSTACK_RADIO.prepare(packetbuf_hdrptr(), packetbuf_totlen());

    is_broadcast = packetbuf_holds_broadcast();

    if(NETSTACK_RADIO.receiving_packet() ||
       (!is_broadcast && NETSTACK_RADIO.pending_packet())) {

      /* Currently receiving a packet over air or the radio has
         already received a packet that needs to be read before
         sending with auto ack. */
      ret = MAC_TX_COLLISION;
    } else {
      if(!is_broadcast) {
        RIMESTATS_ADD(reliabletx);
      }
#if NULLRDC_ENABLE_RETRANSMISSIONS
  while(1) {
    /* Transmit packet and check status */
    tx_attempts++;
#endif /* NULLRDC_ENABLE_RETRANSMISSIONS */

      switch(NETSTACK_RADIO.transmit(packetbuf_totlen(buf))) {
      case RADIO_TX_OK:
        if(is_broadcast) {
          ret = MAC_TX_OK;
        } else {
          rtimer_clock_t wt;

          /* Check for ack */
          wt = RTIMER_NOW();
          watchdog_periodic();
          while(RTIMER_CLOCK_LT(RTIMER_NOW(), wt + ACK_WAIT_TIME)) {
#if CONTIKI_TARGET_COOJA
            simProcessRunValue = 1;
            cooja_mt_yield();
#endif /* CONTIKI_TARGET_COOJA */
          }

          ret = MAC_TX_NOACK;
          if(NETSTACK_RADIO.receiving_packet() ||
             NETSTACK_RADIO.pending_packet() ||
             NETSTACK_RADIO.channel_clear() == 0) {
            int len;
            uint8_t ackbuf[ACK_LEN];

            if(AFTER_ACK_DETECTED_WAIT_TIME > 0) {
              wt = RTIMER_NOW();
              watchdog_periodic();
              while(RTIMER_CLOCK_LT(RTIMER_NOW(),
                                    wt + AFTER_ACK_DETECTED_WAIT_TIME)) {
      #if CONTIKI_TARGET_COOJA
                  simProcessRunValue = 1;
                  cooja_mt_yield();
      #endif /* CONTIKI_TARGET_COOJA */
              }
            }

            if(NETSTACK_RADIO.pending_packet()) {
              len = NETSTACK_RADIO.read(ackbuf, ACK_LEN);
              if(len == ACK_LEN && ackbuf[2] == dsn) {
                /* Ack received */
                RIMESTATS_ADD(ackrx);
                ret = MAC_TX_OK;
              } else {
                /* Not an ack or ack not for us: collision */
                ret = MAC_TX_COLLISION;
              }
            }
          } else {
	    PRINTF("nullrdc tx noack\n");
	  }
        }
        break;
      case RADIO_TX_COLLISION:
        ret = MAC_TX_COLLISION;
        break;
      default:
        ret = MAC_TX_ERR;
        break;
      }
#if NULLRDC_ENABLE_RETRANSMISSIONS
    if(is_broadcast) {
#if !NULLRDC_ENABLE_RETRANSMISSIONS_BCAST
      break;
#else  /* NULLRDC_ENABLE_RETRANSMISSIONS_BCAST */
      if(ret != MAC_TX_COLLISION) {
        /* Retry broadcast frame only upon collision */
        break;
      }
#endif /* NULLRDC_ENABLE_RETRANSMISSIONS_BCAST */
    } else {
      /* Frame is unicast. Do not retry unless NO_ACK or COLLISION */
      if((ret != MAC_TX_NOACK) && (ret != MAC_TX_COLLISION)) {
        break;
      }
    }
    /* Do not retry if max attempts reached. */
    if(tx_attempts >= max_tx_attempts) {
      PRINTF("nullrdc: max tx attempts reached\n");
      break;
    }
    /* Block-wait before retrying the frame. */
    target_time = RTIMER_NOW() +
      (RTIMER_SECOND * NULLRDC_TX_RETRY_DELAY_MS / 1000);
    watchdog_periodic();
    while(RTIMER_CLOCK_LT(RTIMER_NOW(), target_time)) {
      /* Wait */
    }
    /* Attempt a new frame (re)transmission */
  }
#endif /* NULLRDC_ENABLE_RETRANSMISSIONS */
    }

#else /* ! NULLRDC_802154_AUTOACK */

    switch(NETSTACK_RADIO.send(buf, packetbuf_hdrptr(buf), packetbuf_totlen(buf))) {
    case RADIO_TX_OK:
      ret = MAC_TX_OK;
      break;
    case RADIO_TX_COLLISION:
      ret = MAC_TX_COLLISION;
      break;
    case RADIO_TX_NOACK:
      ret = MAC_TX_NOACK;
      break;
    default:
      ret = MAC_TX_ERR;
      break;
    }

#endif /* ! NULLRDC_802154_AUTOACK */
  }
  if(ret == MAC_TX_OK) {
    last_sent_ok = 1;
  }
#if ! NULLRDC_ENABLE_RETRANSMISSIONS
  mac_call_sent_callback(buf, sent, ptr, ret, 1);
#else
  mac_call_sent_callback(buf, sent, ptr, ret, tx_attempts);
#endif /* !NULLRDC_ENABLE_RETRANSMISSIONS */
  return last_sent_ok;
}
/*---------------------------------------------------------------------------*/
static uint8_t
send_packet(struct net_mbuf *buf, mac_callback_t sent, void *ptr)
{
  return send_one_packet(buf, sent, ptr);
}
/*---------------------------------------------------------------------------*/
static uint8_t
send_list(struct net_mbuf *buf, mac_callback_t sent, void *ptr, struct rdc_buf_list *buf_list)
{
  while(buf_list != NULL) {
    /* We backup the next pointer, as it may be nullified by
     * mac_call_sent_callback() */
    struct rdc_buf_list *next = buf_list->next;
    int last_sent_ok;

    queuebuf_to_packetbuf(buf, buf_list->buf);
    last_sent_ok = send_one_packet(buf, sent, ptr);

    /* If packet transmission was not successful, we should back off and let
     * upper layers retransmit, rather than potentially sending out-of-order
     * packet fragments. */
    if(!last_sent_ok) {
      return 0;
    }
    buf_list = next;
  }

  return 1;
}
/*---------------------------------------------------------------------------*/
static uint8_t
packet_input(struct net_mbuf *buf)
{
#if NULLRDC_SEND_802154_ACK
  int original_datalen;
  uint8_t *original_dataptr;

  original_datalen = packetbuf_datalen(buf);
  original_dataptr = packetbuf_dataptr(buf);
#endif

#if NULLRDC_802154_AUTOACK
  if(packetbuf_datalen(buf) == ACK_LEN) {
    /* Ignore ack packets */
    PRINTF("nullrdc: ignored ack\n");
  } else
#endif /* NULLRDC_802154_AUTOACK */
  if(NETSTACK_FRAMER.parse(buf) < 0) {
    PRINTF("nullrdc: failed to parse %u\n", packetbuf_datalen(buf));
#if NULLRDC_ADDRESS_FILTER
  } else if(!linkaddr_cmp(packetbuf_addr(buf, PACKETBUF_ADDR_RECEIVER),
                                         &linkaddr_node_addr) &&
            !packetbuf_holds_broadcast(buf)) {
    PRINTF("nullrdc: not for us\n");
#endif /* NULLRDC_ADDRESS_FILTER */
  } else {
    int duplicate = 0;

#if NULLRDC_802154_AUTOACK || NULLRDC_802154_AUTOACK_HW
    /* If llsec is not doing anti replay checks this needs to be done */
    if(anti_replay_is_anti_replay_checked(buf) == 0) {
      /* Check for duplicate packet. */
      duplicate = mac_sequence_is_duplicate(buf);
      if(duplicate) {
        /* Drop the packet. */
        PRINTF("nullrdc: drop duplicate link layer packet %u\n",
               packetbuf_attr(buf, PACKETBUF_ATTR_MAC_SEQNO));
      } else {
        mac_sequence_register_seqno(buf);
      }
    }
#endif /* NULLRDC_802154_AUTOACK */

/* TODO We may want to acknowledge only authentic frames */
#if NULLRDC_SEND_802154_ACK
    {
      frame802154_t info154;
      frame802154_parse(original_dataptr, original_datalen, &info154);
      if(info154.fcf.frame_type == FRAME802154_DATAFRAME &&
         info154.fcf.ack_required != 0 &&
         linkaddr_cmp((linkaddr_t *)&info154.dest_addr,
                      &linkaddr_node_addr)) {
        uint8_t ackdata[ACK_LEN] = {0, 0, 0};

        ackdata[0] = FRAME802154_ACKFRAME;
        ackdata[1] = 0;
        ackdata[2] = info154.seq;
        return NETSTACK_RADIO.send(ackdata, ACK_LEN);
      }
    }
#endif /* NULLRDC_SEND_ACK */
    if(!duplicate) {
      return NETSTACK_MAC.input(buf);
    }
  }

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
static unsigned short
channel_check_interval(void)
{
  return 0;
}
/*---------------------------------------------------------------------------*/
static void
init(void)
{
  on();
}
/*---------------------------------------------------------------------------*/
const struct rdc_driver nullrdc_driver = {
  "nullrdc",
  init,
  send_packet,
  send_list,
  packet_input,
  on,
  off,
  channel_check_interval,
};
/*---------------------------------------------------------------------------*/
