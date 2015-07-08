/* -*- C -*- */
/*
 * Copyright (c) 2005, Swedish Institute of Computer Science
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

#if defined(CONFIG_NETWORKING_UART)

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include <console/uart_pipe.h>
#include <net/net_core.h>

#include <net/l2_buf.h>

#include "contiki.h"

#include "contiki/ip/uip.h"

#if !defined(CONFIG_NETWORKING_DEBUG_UART)
#undef NET_DBG
#define NET_DBG(fmt, ...)
#endif

#define BUF(buf) ((struct uip_tcpip_hdr *)&uip_buf(buf)[UIP_LLH_LEN])

#include "dev/slip.h"

#define SLIP_END     0300
#define SLIP_ESC     0333
#define SLIP_ESC_END 0334
#define SLIP_ESC_ESC 0335

PROCESS(slip_process, "SLIP driver");

uint8_t slip_active;

#if 1
#define SLIP_STATISTICS(statement)
#else
uint16_t slip_rubbish, slip_twopackets, slip_overflow, slip_ip_drop;
#define SLIP_STATISTICS(statement) statement
#endif

/* Must be at least one byte larger than UIP_BUFSIZE! */
#define RX_BUFSIZE (UIP_BUFSIZE - UIP_LLH_LEN + 16)

enum {
  STATE_TWOPACKETS = 0,	/* We have 2 packets and drop incoming data. */
  STATE_OK = 1,
  STATE_ESC = 2,
  STATE_RUBBISH = 3,
};

/*
 * Variables begin and end manage the buffer space in a cyclic
 * fashion. The first used byte is at begin and end is one byte past
 * the last. I.e. [begin, end) is the actively used space.
 *
 * If begin != pkt_end we have a packet at [begin, pkt_end),
 * furthermore, if state == STATE_TWOPACKETS we have one more packet at
 * [pkt_end, end). If more bytes arrive in state STATE_TWOPACKETS
 * they are discarded.
 */

static uint8_t state = STATE_TWOPACKETS;
static uint16_t begin, end;
static uint8_t rxbuf[RX_BUFSIZE];
static uint16_t pkt_end;		/* SLIP_END tracker. */

static void (* input_callback)(void) = NULL;
/*---------------------------------------------------------------------------*/
void
slip_set_input_callback(void (*c)(void))
{
  input_callback = c;
}
/*---------------------------------------------------------------------------*/
/* slip_send: forward (IPv4) packets with {UIP_FW_NETIF(..., slip_send)}
 * was used in slip-bridge.c
 */
uint8_t
slip_send(struct net_buf *buf)
{
  uint16_t i;
  uint8_t *ptr;
  uint8_t c;

  slip_arch_writeb(SLIP_END);

  ptr = &uip_buf(buf)[UIP_LLH_LEN];
  for(i = 0; i < uip_len(buf); ++i) {
    if(i == UIP_TCPIP_HLEN) {
      ptr = (uint8_t *)uip_appdata(buf);
    }
    c = *ptr++;
    if(c == SLIP_END) {
      slip_arch_writeb(SLIP_ESC);
      c = SLIP_ESC_END;
    } else if(c == SLIP_ESC) {
      slip_arch_writeb(SLIP_ESC);
      c = SLIP_ESC_ESC;
    }
    slip_arch_writeb(c);
  }
  slip_arch_writeb(SLIP_END);

  return 0; /* UIP_FW_OK */
}
/*---------------------------------------------------------------------------*/
uint8_t
slip_write(const void *_ptr, int len)
{
  const uint8_t *ptr = _ptr;
  uint16_t i;
  uint8_t c;

  slip_arch_writeb(SLIP_END);

  for(i = 0; i < len; ++i) {
    c = *ptr++;
    if(c == SLIP_END) {
      slip_arch_writeb(SLIP_ESC);
      c = SLIP_ESC_END;
    } else if(c == SLIP_ESC) {
      slip_arch_writeb(SLIP_ESC);
      c = SLIP_ESC_ESC;
    }
    slip_arch_writeb(c);
  }
  slip_arch_writeb(SLIP_END);

  return len;
}
/*---------------------------------------------------------------------------*/
static void
rxbuf_init(void)
{
  begin = end = pkt_end = 0;
  state = STATE_OK;
}
/*---------------------------------------------------------------------------*/
/* Upper half does the polling. */
static uint16_t
slip_poll_handler(uint8_t *outbuf, uint16_t blen)
{
  /* This is a hack and won't work across buffer edge! */
  if(rxbuf[begin] == 'C') {
    int i;
    if(begin < end && (end - begin) >= 6
       && memcmp(&rxbuf[begin], "CLIENT", 6) == 0) {
      state = STATE_TWOPACKETS;	/* Interrupts do nothing. */
      memset(&rxbuf[begin], 0x0, 6);

      rxbuf_init();

      for(i = 0; i < 13; i++) {
	slip_arch_writeb("CLIENTSERVER\300"[i]);
      }
      return 0;
    }
  }
#ifdef SLIP_CONF_ANSWER_MAC_REQUEST
  else if(rxbuf[begin] == '?') {
    /* Used by tapslip6 to request mac for auto configure */
    int j, addr_len;
    linkaddr_t *addr;
    char* hexchar = "0123456789abcdef";
    if(begin < end && (end - begin) >= 2
       && rxbuf[begin + 1] == 'M') {
      state = STATE_TWOPACKETS; /* Interrupts do nothing. */
      rxbuf[begin] = 0;
      rxbuf[begin + 1] = 0;

      rxbuf_init();

      addr = linkaddr_get_node_addr(&addr_len);
      /* this is just a test so far... just to see if it works */
      slip_arch_writeb('!');
      slip_arch_writeb('M');
      for(j = 0; j < addr_len; j++) {
        slip_arch_writeb(hexchar[addr->u8[j] >> 4]);
        slip_arch_writeb(hexchar[addr->u8[j] & 15]);
      }
      slip_arch_writeb(SLIP_END);
      return 0;
    }
  }
#endif /* SLIP_CONF_ANSWER_MAC_REQUEST */

  /*
   * Interrupt can not change begin but may change pkt_end.
   * If pkt_end != begin it will not change again.
   */
  if(begin != pkt_end) {
    uint16_t len;

    if(begin < pkt_end) {
      len = pkt_end - begin;
      if(len > blen) {
	len = 0;
      } else {
	memcpy(outbuf, &rxbuf[begin], len);
      }
    } else {
      len = (RX_BUFSIZE - begin) + (pkt_end - 0);
      if(len > blen) {
	len = 0;
      } else {
	unsigned i;
	for(i = begin; i < RX_BUFSIZE; i++) {
	  *outbuf++ = rxbuf[i];
	}
	for(i = 0; i < pkt_end; i++) {
	  *outbuf++ = rxbuf[i];
	}
      }
    }

    /* Remove data from buffer together with the copied packet. */
    begin = pkt_end;
    if(state == STATE_TWOPACKETS) {
      pkt_end = end;
      state = STATE_OK;		/* Assume no bytes where lost! */

      /* One more packet is buffered, need to be polled again! */
      process_poll(&slip_process);
    }
    return len;
  }

  return 0;
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(slip_process, ev, data, not_used, user_data)
{
  struct net_buf *buf;

  PROCESS_BEGIN();

  rxbuf_init();

  while(1) {
    PROCESS_YIELD_UNTIL(ev == PROCESS_EVENT_POLL);

    slip_active = 1;

    buf = ip_buf_get_reserve_rx(0);
    if (!buf) {
      NET_ERR("No RX buffers left, slip msg discarded\n");
      rxbuf_init();
      continue;
    }

    /* Move packet from rxbuf to buffer provided by uIP. */
    uip_len(buf) = slip_poll_handler(&uip_buf(buf)[UIP_LLH_LEN],
				UIP_BUFSIZE - UIP_LLH_LEN);
#if !NETSTACK_CONF_WITH_IPV6
    if(uip_len(buf) == 4 && strncmp((char*)&uip_buf(buf)[UIP_LLH_LEN], "?IPA", 4) == 0) {
      char sbuf[8];
      memcpy(&sbuf[0], "=IPA", 4);
      memcpy(&sbuf[4], &uip_hostaddr, 4);
      if(input_callback) {
	input_callback();
      }
      slip_write(sbuf, 8);
      /* If the packet is being discarded in uip.c:uip_input(), then the
       * uip_len(buf) is set to 0. If that is done, then do not release
       * the buffer here as that would cause double free.
       */
      if (uip_len(buf) != 0) {
	ip_buf_unref(buf);
      }
    } else if(uip_len(buf) > 0
       && uip_len(buf) == (((uint16_t)(BUF(buf)->len[0]) << 8) + BUF(buf)->len[1])
       && uip_ipchksum(buf) == 0xffff) {
#define IP_DF   0x40
      if(BUF(buf)->ipid[0] == 0 && BUF(buf)->ipid[1] == 0 && BUF(buf)->ipoffset[0] & IP_DF) {
	static uint16_t ip_id;
	uint16_t nid = ip_id++;
	BUF(buf)->ipid[0] = nid >> 8;
	BUF(buf)->ipid[1] = nid;
	nid = uip_htons(nid);
	nid = ~nid;		/* negate */
	BUF(buf)->ipchksum += nid;	/* add */
	if(BUF(buf)->ipchksum < nid) { /* 1-complement overflow? */
	  BUF(buf)->ipchksum++;
	}
      }

      net_buf_add(buf, uip_len(buf));

#ifdef SLIP_CONF_TCPIP_INPUT
      if (SLIP_CONF_TCPIP_INPUT(buf) < 0) {
	ip_buf_unref(buf);
      }
#else
      tcpip_input(buf);
#endif
    } else {
      NET_DBG("Dropping slip message buf %p\n", buf);
      uip_len(buf) = 0;
      ip_buf_unref(buf);
      SLIP_STATISTICS(slip_ip_drop++);
    }
#else /* NETSTACK_CONF_WITH_IPV6 */
    if(uip_len(buf) > 0) {
      net_buf_add(buf, uip_len(buf));

      if(input_callback) {
        input_callback();
      }
#ifdef SLIP_CONF_TCPIP_INPUT
      if (SLIP_CONF_TCPIP_INPUT(buf) < 0) {
	ip_buf_unref(buf);
      }
#else
      tcpip_input(buf);
#endif
    }
#endif /* NETSTACK_CONF_WITH_IPV6 */
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
int
slip_input_byte(unsigned char c)
{
  switch(state) {
  case STATE_RUBBISH:
    if(c == SLIP_END) {
      state = STATE_OK;
    }
    return 0;

  case STATE_TWOPACKETS:       /* Two packets are already buffered! */
    return 0;

  case STATE_ESC:
    if(c == SLIP_ESC_END) {
      c = SLIP_END;
    } else if(c == SLIP_ESC_ESC) {
      c = SLIP_ESC;
    } else {
      state = STATE_RUBBISH;
      SLIP_STATISTICS(slip_rubbish++);
      end = pkt_end;		/* remove rubbish */
      return 0;
    }
    state = STATE_OK;
    break;

  case STATE_OK:
    if(c == SLIP_ESC) {
      state = STATE_ESC;
      return 0;
    } else if(c == SLIP_END) {
	/*
	 * We have a new packet, possibly of zero length.
	 *
	 * There may already be one packet buffered.
	 */
      if(end != pkt_end) {	/* Non zero length. */
	if(begin == pkt_end) {	/* None buffered. */
	  pkt_end = end;
	} else {
	  state = STATE_TWOPACKETS;
	  SLIP_STATISTICS(slip_twopackets++);
	}
	process_poll(&slip_process);
	return 1;
      }
      return 0;
    }
    break;
  }

  /* add_char: */
  {
    unsigned next;
    next = end + 1;
    if(next == RX_BUFSIZE) {
      next = 0;
    }
    if(next == begin) {		/* rxbuf is full */
      state = STATE_RUBBISH;
      SLIP_STATISTICS(slip_overflow++);
      end = pkt_end;		/* remove rubbish */
      return 0;
    }
    rxbuf[end] = c;
    end = next;
  }

  /* There could be a separate poll routine for this. */
  if(c == 'T' && rxbuf[begin] == 'C') {
    process_poll(&slip_process);
    return 1;
  }

  return 0;
}
/*---------------------------------------------------------------------------*/

void slip_recv(void)
{
  process_post_synch(&slip_process, PROCESS_EVENT_POLL, NULL, NULL);
}

/*---------------------------------------------------------------------------*/
static uint8_t *recv_cb(uint8_t *buf, size_t *off)
{
  int i;

  for (i = 0; i < *off; i++) {
    if (slip_input_byte(buf[i])) {
      /*
       * The magic happens in slip.c:PROCESS_THREAD()
       * It will copy the slip.c internal buffer into
       * net_buf and call IP stack input function. The input
       * function is set to net_recv() which will then
       * feed the buffer into rx fiber.
       */
      slip_recv();
      break;
    }
  }

  *off = 0;
  return buf;
}

/*---------------------------------------------------------------------------*/
void slip_start(void)
{
  /* Use small temp buffer for receiving data */
  static uint8_t buf[32];

  uart_pipe_register(buf, sizeof(buf), recv_cb);

  process_start(&slip_process, NULL, NULL);
}

#endif /* defined(CONFIG_NETWORKING_UART) */
