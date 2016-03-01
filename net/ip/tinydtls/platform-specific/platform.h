/*
 * Copyright (c) 2016 Intel Corporation
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

/************************************************************************/
/* Contiki-specific parameters                                          */
/************************************************************************/

#ifndef _PLATFORM_H_
#define _PLATFORM_H_ 1

#ifdef CONTIKI
#include "contiki.h"
#include "contiki-lib.h"
#include "contiki-net.h"

#include "contiki-conf.h"

/* global constants for constrained devices running Contiki */
#ifndef DTLS_PEER_MAX
/** The maximum number DTLS peers (i.e. sessions). */
#  define DTLS_PEER_MAX 1
#endif

#ifndef DTLS_HANDSHAKE_MAX
/** The maximum number of concurrent DTLS handshakes. */
#  define DTLS_HANDSHAKE_MAX 1
#endif

#ifndef DTLS_SECURITY_MAX
/** The maximum number of concurrently used cipher keys */
#  define DTLS_SECURITY_MAX (DTLS_PEER_MAX + DTLS_HANDSHAKE_MAX)
#endif

#ifndef DTLS_HASH_MAX
/** The maximum number of hash functions that can be used in parallel. */
#  define DTLS_HASH_MAX (3 * DTLS_PEER_MAX)
#endif

/************************************************************************/
/* Specific Contiki platforms                                           */
/************************************************************************/

#if CONTIKI_TARGET_ECONOTAG
#  include "platform-specific/config-econotag.h"
#endif /* CONTIKI_TARGET_ECONOTAG */

#ifdef CONTIKI_TARGET_CC2538DK
#  include "platform-specific/config-cc2538dk.h"
#endif /* CONTIKI_TARGET_CC2538DK */

#ifdef CONTIKI_TARGET_FELICIA
#define CONTIKI_TARGET_CC2538DK 1
#  include "platform-specific/config-cc2538dk.h"
#endif /* CONTIKI_TARGET_CC2538DK */

#ifdef CONTIKI_TARGET_WISMOTE
#  include "platform-specific/config-wismote.h"
#endif /* CONTIKI_TARGET_WISMOTE */

#ifdef CONTIKI_TARGET_SKY
#  include "platform-specific/config-sky.h"
#endif /* CONTIKI_TARGET_SKY */

#ifdef CONTIKI_TARGET_MINIMAL_NET
#  include "platform-specific/config-minimal-net.h"
#endif /* CONTIKI_TARGET_MINIMAL_NET */

#ifdef CONTIKI_TARGET_NATIVE
#  include "platform-specific/config-native.h"
#endif /* CONTIKI_TARGET_NATIVE */

#ifdef CONTIKI_TARGET_ZEPHYR
#  include "platform-specific/config-zephyr.h"
#endif /* CONTIKI_TARGET_ZEPHYR */

#endif /* CONTIKI */

#endif /* _PLATFORM_H_ */
