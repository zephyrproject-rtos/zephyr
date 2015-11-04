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

#ifndef HANDLER_802154_H_
#define HANDLER_802154_H_

#include "contiki/mac/frame802154.h"

#define BEACON_PAYLOAD_BUFFER_SIZE 128

typedef enum {
  CALLBACK_ACTION_RX,
  CALLBACK_ACTION_CHANNEL_DONE,
  CALLBACK_ACTION_SCAN_END,
} callback_action;

#define CALLBACK_STATUS_CONTINUE       0
#define CALLBACK_STATUS_NEED_MORE_TIME 1
#define CALLBACK_STATUS_FINISHED       2

typedef int (* scan_callback_t)(uint8_t channel, frame802154_t *frame, callback_action cba);

void handler_802154_join(uint16_t panid, int answer_beacons);

int handler_802154_active_scan(scan_callback_t callback);
int handler_802154_calculate_beacon_payload_length(uint8_t *beacon, int maxlen);
void handler_802154_set_beacon_payload(uint8_t *payload, uint8_t len);
uint8_t *handler_802154_get_beacon_payload(uint8_t *len);
void handler_802154_send_beacon_request(void);
void handler_802154_set_answer_beacons(int answer);

#ifndef HANDLER_802154_CONF_STATS
#define HANDLER_802154_CONF_STATS 0
#endif

#if HANDLER_802154_CONF_STATS
/* Statistics for fault management. */
typedef struct handler_802154_stats {
  uint16_t beacons_reqs_sent;
  uint16_t beacons_sent;
  uint16_t beacons_received;
} handler_802154_stats_t;

extern handler_802154_stats_t handler_802154_stats;

#define HANDLER_802154_STAT(code) (code)
#else /* HANDLER_802154_CONF_STATS */
#define HANDLER_802154_STAT(code)
#endif /* HANDLER_802154_CONF_STATS */

#endif /* HANDLER_802154_H_ */
