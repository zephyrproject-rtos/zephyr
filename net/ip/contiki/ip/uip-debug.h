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
 *         A set of debugging macros.
 *
 * \author Nicolas Tsiftes <nvt@sics.se>
 *         Niclas Finne <nfi@sics.se>
 *         Joakim Eriksson <joakime@sics.se>
 */

#ifndef UIP_DEBUG_H
#define UIP_DEBUG_H

#include "contiki/ip/uip.h"
#include <stdio.h>

void uip_debug_ipaddr_print(const uip_ipaddr_t *addr);
void uip_debug_lladdr_print(const uip_lladdr_t *addr);
void uip_debug_hex_dump(const unsigned char *buffer, int len);

#if defined(CONFIG_STDOUT_CONSOLE)
#include <stdio.h>
#define PRINT          printf
#else
#include <misc/printk.h>
#define PRINT          printk
#endif

#define DEBUG_NONE      0
#define DEBUG_PRINT     1
#define DEBUG_ANNOTATE  2
#define DEBUG_FULL      DEBUG_ANNOTATE | DEBUG_PRINT

#if (DEBUG == 1)
#if defined(CONFIG_NETWORK_IP_STACK_DEBUG_PRINT)
#define _DEBUG_ DEBUG_PRINT
#elif defined(CONFIG_NETWORK_IP_STACK_DEBUG_ANNOTATE)
#define _DEBUG_ DEBUG_ANNOTATE
#elif defined(CONFIG_NETWORK_IP_STACK_DEBUG_FULL)
#define _DEBUG_ DEBUG_FULL
#else
#define _DEBUG_ DEBUG_NONE
#endif
#endif

/* PRINTA will always print if the debug routines are called directly */
#ifdef __AVR__
#include <avr/pgmspace.h>
#define PRINTA(FORMAT,args...) printf_P(PSTR(FORMAT),##args)
#else
#define PRINTA(...) PRINT(__VA_ARGS__)
#endif

#if (_DEBUG_) & DEBUG_ANNOTATE
#ifdef __AVR__
#define ANNOTATE(FORMAT,args...) printf_P(PSTR(FORMAT),##args)
#else
#define ANNOTATE(...) PRINT(__VA_ARGS__)
#endif
#else
#define ANNOTATE(...)
#endif /* (_DEBUG_) & DEBUG_ANNOTATE */

#if (_DEBUG_) & DEBUG_PRINT
#ifdef __AVR__
#define PRINTF(FORMAT,args...) printf_P(PSTR(FORMAT),##args)
#else
#define PRINTF(...) PRINT(__VA_ARGS__)
#endif
#define PRINT6ADDR(addr) uip_debug_ipaddr_print(addr)
#define PRINTLLADDR(lladdr) uip_debug_lladdr_print(lladdr)
#else
#define PRINTF(...)
#define PRINT6ADDR(addr)
#define PRINTLLADDR(lladdr)
#endif /* (_DEBUG_) & DEBUG_PRINT */

#endif
