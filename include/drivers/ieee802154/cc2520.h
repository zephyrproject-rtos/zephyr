/* cc2520.h - Public utilities for TI CC2520 (IEEE 802.15.4) */

/*
 * Copyright (c) 2015 Intel Corporation
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

#ifndef __CC2520_H__
#define __CC2520_H__

#include <stdbool.h>

#include "dev/radio.h"

#define CC2520_MAX_PACKET_LEN      127

/* extern signed char cc2520_last_rssi; */
/* extern uint8_t cc2520_last_correlation; */

radio_result_t cc2520_get_value(radio_param_t param, radio_value_t *value);
radio_result_t cc2520_set_value(radio_param_t param, radio_value_t value);

int cc2520_set_channel(int channel);
int cc2520_get_channel(void);

bool cc2520_set_pan_addr(unsigned pan,
			 unsigned addr,
			 const uint8_t *ieee_addr);
int cc2520_on(void);
int cc2520_rssi(void);

void cc2520_set_cca_threshold(int value);

/* Power is between 1 and 31 */
void cc2520_set_txpower(uint8_t power);
int cc2520_get_txpower(void);
#define CC2520_TXPOWER_MAX  31
#define CC2520_TXPOWER_MIN   0

typedef struct radio_driver cc2520_driver_api_t;

#endif /* __CC2520_H__ */
