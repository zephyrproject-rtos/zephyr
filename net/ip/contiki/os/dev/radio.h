/*
 * Copyright (c) 2005, Swedish Institute of Computer Science.
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
 *         Header file for the radio API
 * \author
 *         Adam Dunkels <adam@sics.se>
 *         Joakim Eriksson <joakime@sics.se>
 *         Niclas Finne <nfi@sics.se>
 *         Nicolas Tsiftes <nvt@sics.se>
 */

/**
 * \addtogroup dev
 * @{
 */

/**
 * \defgroup radio Radio API
 *
 * The radio API module defines a set of functions that a radio device
 * driver must implement.
 *
 * @{
 */

#ifndef RADIO_H_
#define RADIO_H_

#include <stddef.h>

/**
 * Each radio has a set of parameters that designate the current
 * configuration and state of the radio. Parameters can either have
 * values of type radio_value_t, or, when this type is insufficient, a
 * generic object that is specified by a memory pointer and the size
 * of the object.
 *
 * The radio_value_t type is set to an integer type that can hold most
 * values used to configure the radio, and is therefore the most
 * common type used for a parameter. Certain parameters require
 * objects of a considerably larger size than radio_value_t, however,
 * and in these cases the documentation below for the parameter will
 * indicate this.
 *
 * All radio parameters that can vary during runtime are prefixed by
 * "RADIO_PARAM", whereas those "parameters" that are guaranteed to
 * remain immutable are prefixed by "RADIO_CONST". Each mutable
 * parameter has a set of valid parameter values. When attempting to
 * set a parameter to an invalid value, the radio will return
 * RADIO_RESULT_INVALID_VALUE.
 *
 * Some radios support only a subset of the defined radio parameters.
 * When trying to set or get such an unsupported parameter, the radio
 * will return RADIO_RESULT_NOT_SUPPORTED.
 */

typedef int radio_value_t;
typedef unsigned radio_param_t;

enum {

  /* Radio power mode determines if the radio is on
    (RADIO_POWER_MODE_ON) or off (RADIO_POWER_MODE_OFF). */
  RADIO_PARAM_POWER_MODE,

  /*
   * Channel used for radio communication. The channel depends on the
   * communication standard used by the radio. The values can range
   * from RADIO_CONST_CHANNEL_MIN to RADIO_CONST_CHANNEL_MAX.
   */
  RADIO_PARAM_CHANNEL,

  /* Personal area network identifier, which is used by the address filter. */
  RADIO_PARAM_PAN_ID,

  /* Short address (16 bits) for the radio, which is used by the address
     filter. */
  RADIO_PARAM_16BIT_ADDR,

  /*
   * Radio receiver mode determines if the radio has address filter
   * (RADIO_RX_MODE_ADDRESS_FILTER) and auto-ACK (RADIO_RX_MODE_AUTOACK)
   * enabled. This parameter is set as a bit mask.
   */
  RADIO_PARAM_RX_MODE,

  /*
   * Radio transmission mode determines if the radio has send on CCA
   * (RADIO_TX_MODE_SEND_ON_CCA) enabled or not. This parameter is set
   * as a bit mask.
   */
  RADIO_PARAM_TX_MODE,

  /*
   * Transmission power in dBm. The values can range from
   * RADIO_CONST_TXPOWER_MIN to RADIO_CONST_TXPOWER_MAX.
   *
   * Some radios restrict the available values to a subset of this
   * range.  If an unavailable TXPOWER value is requested to be set,
   * the radio may select another TXPOWER close to the requested
   * one. When getting the value of this parameter, the actual value
   * used by the radio will be returned.
   */
  RADIO_PARAM_TXPOWER,

  /*
   * Clear channel assessment threshold in dBm. This threshold
   * determines the minimum RSSI level at which the radio will assume
   * that there is a packet in the air.
   *
   * The CCA threshold must be set to a level above the noise floor of
   * the deployment. Otherwise mechanisms such as send-on-CCA and
   * low-power-listening duty cycling protocols may not work
   * correctly. Hence, the default value of the system may not be
   * optimal for any given deployment.
   */
  RADIO_PARAM_CCA_THRESHOLD,

  /* Received signal strength indicator in dBm. */
  RADIO_PARAM_RSSI,

  /*
   * Long (64 bits) address for the radio, which is used by the address filter.
   * The address is specified in network byte order.
   *
   * Because this parameter value is larger than what fits in radio_value_t,
   * it needs to be used with radio.get_object()/set_object().
   */
  RADIO_PARAM_64BIT_ADDR,

  /* Constants (read only) */

  /* The lowest radio channel. */
  RADIO_CONST_CHANNEL_MIN,
  /* The highest radio channel. */
  RADIO_CONST_CHANNEL_MAX,

  /* The minimum transmission power in dBm. */
  RADIO_CONST_TXPOWER_MIN,
  /* The maximum transmission power in dBm. */
  RADIO_CONST_TXPOWER_MAX
};

/* Radio power modes */
enum {
  RADIO_POWER_MODE_OFF,
  RADIO_POWER_MODE_ON
};

/**
 * The radio reception mode controls address filtering and automatic
 * transmission of acknowledgements in the radio (if such operations
 * are supported by the radio). A single parameter is used to allow
 * setting these features simultaneously as an atomic operation.
 *
 * To enable both address filter and transmissions of automatic
 * acknowledgments:
 *
 * NETSTACK_RADIO.set_value(RADIO_PARAM_RX_MODE,
 *       RADIO_RX_MODE_ADDRESS_FILTER | RADIO_RX_MODE_AUTOACK);
 */
#define RADIO_RX_MODE_ADDRESS_FILTER   (1 << 0)
#define RADIO_RX_MODE_AUTOACK          (1 << 1)

/**
 * The radio transmission mode controls whether transmissions should
 * be done using clear channel assessment (if supported by the
 * radio). If send-on-CCA is enabled, the radio's send function will
 * wait for a radio-specific time window for the channel to become
 * clear. If this does not happen, the send function will return
 * RADIO_TX_COLLISION.
 */
#define RADIO_TX_MODE_SEND_ON_CCA      (1 << 0)

/* Radio return values when setting or getting radio parameters. */
typedef enum {
  RADIO_RESULT_OK,
  RADIO_RESULT_NOT_SUPPORTED,
  RADIO_RESULT_INVALID_VALUE,
  RADIO_RESULT_ERROR
} radio_result_t;

/* Radio return values for transmissions. */
enum {
  RADIO_TX_OK,
  RADIO_TX_ERR,
  RADIO_TX_COLLISION,
  RADIO_TX_NOACK,
};

/**
 * The structure of a device driver for a radio in Contiki.
 */
struct radio_driver {

  int (* init)(void);

  /** Prepare the radio with a packet to be sent. */
  int (* prepare)(const void *payload, unsigned short payload_len);

  /** Send the packet that has previously been prepared. */
  int (* transmit)(unsigned short transmit_len);

  /** Prepare & transmit a packet. */
  int (* send)(const void *payload, unsigned short payload_len);

  /** Read a received packet into a buffer. */
  int (* read)(void *buf, unsigned short buf_len);

  /** Perform a Clear-Channel Assessment (CCA) to find out if there is
      a packet in the air or not. */
  int (* channel_clear)(void);

  /** Check if the radio driver is currently receiving a packet */
  int (* receiving_packet)(void);

  /** Check if the radio driver has just received a packet */
  int (* pending_packet)(void);

  /** Turn the radio on. */
  int (* on)(void);

  /** Turn the radio off. */
  int (* off)(void);

  /** Get a radio parameter value. */
  radio_result_t (* get_value)(radio_param_t param, radio_value_t *value);

  /** Set a radio parameter value. */
  radio_result_t (* set_value)(radio_param_t param, radio_value_t value);

  /**
   * Get a radio parameter object. The argument 'dest' must point to a
   * memory area of at least 'size' bytes, and this memory area will
   * contain the parameter object if the function succeeds.
   */
  radio_result_t (* get_object)(radio_param_t param, void *dest, size_t size);

  /**
   * Set a radio parameter object. The memory area referred to by the
   * argument 'src' will not be accessed after the function returns.
   */
  radio_result_t (* set_object)(radio_param_t param, const void *src,
                                size_t size);

};

#endif /* RADIO_H_ */

/** @} */
/** @} */
