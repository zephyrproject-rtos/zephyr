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
 *         Header file for the Rime queue buffer management
 * \author
 *         Adam Dunkels <adam@sics.se>
 */

/**
 * \addtogroup rime
 * @{
 */

/**
 * \defgroup rimequeuebuf Rime queue buffer management
 * @{
 *
 * The queuebuf module handles buffers that are queued.
 *
 */

#ifndef QUEUEBUF_H_
#define QUEUEBUF_H_

#include "net/packetbuf.h"

/* QUEUEBUF_NUM is the total number of queuebuf */
#ifdef QUEUEBUF_CONF_NUM
#define QUEUEBUF_NUM QUEUEBUF_CONF_NUM
#else
#define QUEUEBUF_NUM 8
#endif

/* QUEUEBUFRAM_NUM is the number of queuebufs stored in RAM.
   If QUEUEBUFRAM_CONF_NUM is set lower than QUEUEBUF_NUM,
   swapping is enabled and queuebufs are stored either in RAM of CFS.
   If QUEUEBUFRAM_CONF_NUM is unset or >= to QUEUEBUF_NUM, all
   queuebufs are in RAM and swapping is disabled. */
#ifdef QUEUEBUFRAM_CONF_NUM
  #if QUEUEBUFRAM_CONF_NUM>QUEUEBUF_NUM
    #error "QUEUEBUFRAM_CONF_NUM cannot be greater than QUEUEBUF_NUM"
  #else
    #define QUEUEBUFRAM_NUM QUEUEBUFRAM_CONF_NUM
    #define WITH_SWAP (QUEUEBUFRAM_NUM < QUEUEBUF_NUM)
  #endif
#else /* QUEUEBUFRAM_CONF_NUM */
  #define QUEUEBUFRAM_NUM QUEUEBUF_NUM
  #define WITH_SWAP 0
#endif /* QUEUEBUFRAM_CONF_NUM */

#ifdef QUEUEBUF_CONF_DEBUG
#define QUEUEBUF_DEBUG QUEUEBUF_CONF_DEBUG
#else /* QUEUEBUF_CONF_DEBUG */
#define QUEUEBUF_DEBUG 0
#endif /* QUEUEBUF_CONF_DEBUG */

struct queuebuf;

void queuebuf_init(void);

#if QUEUEBUF_DEBUG
struct queuebuf *queuebuf_new_from_packetbuf_debug(const char *file, int line);
#define queuebuf_new_from_packetbuf() queuebuf_new_from_packetbuf_debug(__FILE__, __LINE__)
#else /* QUEUEBUF_DEBUG */
struct queuebuf *queuebuf_new_from_packetbuf(void);
#endif /* QUEUEBUF_DEBUG */
void queuebuf_update_attr_from_packetbuf(struct queuebuf *b);
void queuebuf_update_from_packetbuf(struct queuebuf *b);

void queuebuf_to_packetbuf(struct queuebuf *b);
void queuebuf_free(struct queuebuf *b);

void *queuebuf_dataptr(struct queuebuf *b);
int queuebuf_datalen(struct queuebuf *b);

linkaddr_t *queuebuf_addr(struct queuebuf *b, uint8_t type);
packetbuf_attr_t queuebuf_attr(struct queuebuf *b, uint8_t type);

void queuebuf_debug_print(void);

int queuebuf_numfree(void);

#endif /* __QUEUEBUF_H__ */

/** @} */
/** @} */
