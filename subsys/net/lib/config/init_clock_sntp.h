/*
 * Copyright (c) 2025 Pedro Ramos
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if CONFIG_NET_CONFIG_SNTP_TIMEZONE

/** Timezones **/
enum timezone {
  UTC_MINUS_12,  // Baker Island, Howland Island
  UTC_MINUS_11,  // American Samoa, Niue
  UTC_MINUS_10,  // Hawaii-Aleutian Standard Time, Cook Islands
  UTC_MINUS_9,   // Alaska Standard Time, Gambier Islands
  UTC_MINUS_8,   // Pacific Standard Time (PST), Los Angeles, Vancouver
  UTC_MINUS_7,   // Mountain Standard Time (MST), Denver, Phoenix
  UTC_MINUS_6,   // Central Standard Time (CST), Mexico City, Chicago
  UTC_MINUS_5,   // Eastern Standard Time (EST), New York, Toronto
  UTC_MINUS_4,   // Atlantic Standard Time (AST), Caracas
  UTC_MINUS_3,   // Buenos Aires, São Paulo, Greenland
  UTC_MINUS_2,   // South Georgia and the South Sandwich Islands
  UTC_MINUS_1,   // Azores, Cape Verde
  UTC_0,         // Greenwich Mean Time (GMT), London
  UTC_PLUS_1,    // Central European Time (CET), Berlin, Paris, Rome
  UTC_PLUS_2,    // Eastern European Time (EET), Athens, Cairo, South Africa
  UTC_PLUS_3,    // Moscow, Istanbul, Saudi Arabia
  UTC_PLUS_4,    // Dubai, Baku, Samara
  UTC_PLUS_5,    // Pakistan, Yekaterinburg
  UTC_PLUS_6,    // Bangladesh, Omsk
  UTC_PLUS_7,    // Thailand, Novosibirsk, Jakarta
  UTC_PLUS_8,    // China, Singapore, Perth
  UTC_PLUS_9,    // Japan, Korea, Irkutsk
  UTC_PLUS_10,   // Sydney, Vladivostok
  UTC_PLUS_11,   // Solomon Islands, Magadan
  UTC_PLUS_12,   // New Zealand, Fiji
  UTC_PLUS_13,   // Tonga, Samoa
  UTC_PLUS_14,   // Line Islands (Kiribati)
  TIMEZONE_MAX
};

void sntp_set_timezone(const enum timezone tz);

/*
* @brief Get timezone description.
*
* @param tz Timezone value.
* 
* @return String content about timezone, i.e., "UTC-4".
*/
const char* get_timezone_string(const enum timezone tz);

#endif

int net_init_clock_via_sntp(void);