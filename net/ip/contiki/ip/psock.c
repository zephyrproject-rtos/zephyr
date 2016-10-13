/*
 * Copyright (c) 2004, Swedish Institute of Computer Science.
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
 * Author: Adam Dunkels <adam@sics.se>
 *
 */

#include <string.h>

#include <net/ip_buf.h>

#ifdef CONFIG_NETWORK_IP_STACK_DEBUG_TCP_PSOCK
#define DEBUG 1
#endif
#include "contiki/ip/uip-debug.h"

#include "contiki/ip/psock.h"

#define STATE_NONE 0
#define STATE_ACKED 1
#define STATE_READ 2
#define STATE_BLOCKED_NEWDATA 3
#define STATE_BLOCKED_CLOSE 4
#define STATE_BLOCKED_SEND 5
#define STATE_DATA_SENT 6

/*
 * Return value of the buffering functions that indicates that a
 * buffer was not filled by incoming data.
 *
 */
#define BUF_NOT_FULL 0
#define BUF_NOT_FOUND 0

/*
 * Return value of the buffering functions that indicates that a
 * buffer was completely filled by incoming data.
 *
 */
#define BUF_FULL 1

/*
 * Return value of the buffering functions that indicates that an
 * end-marker byte was found.
 *
 */
#define BUF_FOUND 2

/*---------------------------------------------------------------------------*/
static void
buf_setup(struct psock_buf *buf,
	  uint8_t *bufptr, uint16_t bufsize)
{
  buf->ptr = bufptr;
  buf->left = bufsize;
}
/*---------------------------------------------------------------------------*/
static uint8_t
buf_bufdata(struct psock_buf *buf, uint16_t len,
	    uint8_t **dataptr, uint16_t *datalen)
{
  if(*datalen < buf->left) {
    PRINTF("%d: *datalen(%d) < left(%d), copy buf->ptr(%p) <- *dataptr(%p)\n",
	   __LINE__, *datalen, buf->left, buf->ptr, *dataptr);
    memcpy(buf->ptr, *dataptr, *datalen);
    buf->ptr += *datalen;
    buf->left -= *datalen;
    *dataptr += *datalen;
    *datalen = 0;
    return BUF_NOT_FULL;
  } else if(*datalen == buf->left) {
    PRINTF("%d: *datalen(%d) == left(%d), copy buf->ptr(%p) <- *dataptr(%p)\n",
	   __LINE__, *datalen, buf->left, buf->ptr, *dataptr);
    memcpy(buf->ptr, *dataptr, *datalen);
    buf->ptr += *datalen;
    buf->left = 0;
    *dataptr += *datalen;
    *datalen = 0;
    return BUF_FULL;
  } else {
    PRINTF("%d: *datalen(%d) > left(%d), copy buf->ptr(%p) <- buf->left(%p)\n",
	   __LINE__, *datalen, buf->left, buf->ptr, buf->left);
    memcpy(buf->ptr, *dataptr, buf->left);
    buf->ptr += buf->left;
    *datalen -= buf->left;
    *dataptr += buf->left;
    buf->left = 0;
    return BUF_FULL;
  }
}
/*---------------------------------------------------------------------------*/
static uint8_t
buf_bufto(CC_REGISTER_ARG struct psock_buf *buf, uint8_t endmarker,
	  CC_REGISTER_ARG uint8_t **dataptr, CC_REGISTER_ARG uint16_t *datalen)
{
  uint8_t c;
  while(buf->left > 0 && *datalen > 0) {
    c = *buf->ptr = **dataptr;
    ++*dataptr;
    ++buf->ptr;
    --*datalen;
    --buf->left;
    
    if(c == endmarker) {
      return BUF_FOUND;
    }
  }

  if(*datalen == 0) {
    return BUF_NOT_FOUND;
  }

  return BUF_FULL;
}
/*---------------------------------------------------------------------------*/
static char
data_is_sent_and_acked(CC_REGISTER_ARG struct psock *s)
{
  /* If data has previously been sent, and the data has been acked, we
     increase the send pointer and call send_data() to send more
     data. */
  PRINTF("%s: psock %p buf %p data[%p..%p] sendptr %p sendlen %d mss %d\n",
	 __func__, s, s->net_buf, ip_buf_appdata(s->net_buf),
	 ip_buf_appdata(s->net_buf) + ip_buf_appdatalen(s->net_buf),
	 s->sendptr, s->sendlen,
	 uip_mss(s->net_buf));

  if(s->state != STATE_DATA_SENT || uip_rexmit(s->net_buf)) {
    if(s->sendlen > uip_mss(s->net_buf)) {
      uip_send(s->net_buf, s->sendptr, uip_mss(s->net_buf));
    } else {
      uip_send(s->net_buf, s->sendptr, s->sendlen);
    }
    s->state = STATE_DATA_SENT;
    return 0;
  } else if(s->state == STATE_DATA_SENT && uip_acked(s->net_buf)) {
    if(s->sendlen > uip_mss(s->net_buf)) {
      s->sendlen -= uip_mss(s->net_buf);
      s->sendptr += uip_mss(s->net_buf);
    } else {
      s->sendptr += s->sendlen;
      s->sendlen = 0;
    }
    s->state = STATE_ACKED;
    return 1;
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
PT_THREAD(psock_send(CC_REGISTER_ARG struct psock *s, struct net_buf *net_buf))
{
  PT_BEGIN(&s->psockpt);

  PRINTF("%s: psock %p buf %p len %d\n", __func__, s, net_buf,
	 ip_buf_appdatalen(net_buf));

  /* If there is no data to send, we exit immediately. */
  if(ip_buf_appdatalen(net_buf) == 0) {
    PT_EXIT(&s->psockpt);
  }

  /* Save the length of and a pointer to the data that is to be
     sent. */
  s->sendptr = ip_buf_appdata(net_buf);
  s->sendlen = ip_buf_appdatalen(net_buf);
  s->net_buf = net_buf;

  s->state = STATE_NONE;

  /* We loop here until all data is sent. The s->sendlen variable is
     updated by the data_sent() function. */
  while(s->sendlen > 0) {

    /*
     * The protothread will wait here until all data has been
     * acknowledged and sent (data_is_acked_and_send() returns 1).
     */
    PT_WAIT_UNTIL(&s->psockpt, data_is_sent_and_acked(s));
  }

  s->state = STATE_NONE;
  
  PT_END(&s->psockpt);
}
/*---------------------------------------------------------------------------*/
PT_THREAD(psock_generator_send(CC_REGISTER_ARG struct psock *s,
			       unsigned short (*generate)(void *), void *arg))
{
  PT_BEGIN(&s->psockpt);

  /* Ensure that there is a generator function to call. */
  if(generate == NULL) {
    PT_EXIT(&s->psockpt);
  }

  s->state = STATE_NONE;
  do {
    /* Call the generator function to generate the data in the
     uip_appdata buffer. */
    s->sendlen = generate(arg);
    s->sendptr = uip_appdata(s->net_buf);
    
    if(s->sendlen > uip_mss(s->net_buf)) {
      uip_send(s->net_buf, s->sendptr, uip_mss(s->net_buf));
    } else {
      uip_send(s->net_buf, s->sendptr, s->sendlen);
    }
    s->state = STATE_DATA_SENT;

    /* Wait until all data is sent and acknowledged. */
 // if (!s->sendlen) break;   //useful debugging aid
    PT_YIELD_UNTIL(&s->psockpt, uip_acked(s->net_buf) || \
				uip_rexmit(s->net_buf));
  } while(!uip_acked(s->net_buf));
  
  s->state = STATE_NONE;
  
  PT_END(&s->psockpt);
}
/*---------------------------------------------------------------------------*/
uint16_t
psock_datalen(struct psock *psock)
{
  return psock->bufsize - psock->buf.left;
}
/*---------------------------------------------------------------------------*/
char
psock_newdata(struct psock *s)
{
  if(s->readlen > 0) {
    /* There is data in the uip_appdata buffer that has not yet been
       read with the PSOCK_READ functions. */
    return 1;
  } else if(s->state == STATE_READ) {
    /* All data in uip_appdata buffer already consumed. */
    s->state = STATE_BLOCKED_NEWDATA;
    return 0;
  } else if(uip_newdata(s->net_buf)) {
    /* There is new data that has not been consumed. */
    return 1;
  } else {
    /* There is no new data. */
    return 0;
  }
}
/*---------------------------------------------------------------------------*/
PT_THREAD(psock_readto(CC_REGISTER_ARG struct psock *psock, unsigned char c))
{
  PT_BEGIN(&psock->psockpt);

  buf_setup(&psock->buf, psock->bufptr, psock->bufsize);
  
  /* XXX: Should add buf_checkmarker() before do{} loop, if
     incoming data has been handled while waiting for a write. */

  do {
    if(psock->readlen == 0) {
      PT_WAIT_UNTIL(&psock->psockpt, psock_newdata(psock));
      psock->state = STATE_READ;
      psock->readptr = (uint8_t *)uip_appdata(psock->net_buf);
      psock->readlen = uip_datalen(psock->net_buf);
    }
  } while(buf_bufto(&psock->buf, c,
		    &psock->readptr,
		    &psock->readlen) == BUF_NOT_FOUND);
  
  if(psock_datalen(psock) == 0) {
    psock->state = STATE_NONE;
    PT_RESTART(&psock->psockpt);
  }
  PT_END(&psock->psockpt);
}
/*---------------------------------------------------------------------------*/
PT_THREAD(psock_readbuf_len(CC_REGISTER_ARG struct psock *psock, uint16_t len))
{
  PT_BEGIN(&psock->psockpt);

  buf_setup(&psock->buf, psock->bufptr, psock->bufsize);

  /* XXX: Should add buf_checkmarker() before do{} loop, if
     incoming data has been handled while waiting for a write. */
  
  /* read len bytes or to end of data */
  do {
    if(psock->readlen == 0) {
      PT_WAIT_UNTIL(&psock->psockpt, psock_newdata(psock));
      psock->state = STATE_READ;
      psock->readptr = (uint8_t *)uip_appdata(psock->net_buf);
      psock->readlen = uip_datalen(psock->net_buf);
    }
  } while(buf_bufdata(&psock->buf, psock->bufsize,
		      &psock->readptr, &psock->readlen) == BUF_NOT_FULL &&
	  psock_datalen(psock) < len);
  
  if(psock_datalen(psock) == 0) {
    psock->state = STATE_NONE;
    PT_RESTART(&psock->psockpt);
  }
  PT_END(&psock->psockpt);
}

/*---------------------------------------------------------------------------*/
void
psock_init(CC_REGISTER_ARG struct psock *psock, struct net_buf *net_buf)
{
  psock->state = STATE_NONE;
  psock->readlen = 0;
  psock->bufptr = ip_buf_appdata(net_buf);
  psock->bufsize = ip_buf_appdatalen(net_buf);
  psock->net_buf = net_buf;
  buf_setup(&psock->buf, psock->bufptr, psock->bufsize);
  PT_INIT(&psock->pt);
  PT_INIT(&psock->psockpt);
}
/*---------------------------------------------------------------------------*/
