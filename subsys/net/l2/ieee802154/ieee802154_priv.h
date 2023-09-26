/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Private IEEE 802.15.4 low level L2 helper utilities
 *
 * These utilities are internal to the native IEEE 802.15.4 L2
 * stack and must not be included and used elsewhere.
 *
 * All references to the spec refer to IEEE 802.15.4-2020.
 */

#ifndef __IEEE802154_PRIV_H__
#define __IEEE802154_PRIV_H__

#include <zephyr/net/buf.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_pkt.h>

/**
 * @brief Sends the given fragment respecting the configured IEEE 802.15.4 access arbitration
 * algorithm (CSMA/CA, ALOHA, etc.) and re-transmission protocol. See sections 6.2.5 (random
 * access methods) and 6.7.4.4 (retransmissions).
 *
 * This function checks for and supports both, software and hardware access arbitration and
 * acknowledgment depending on driver capabilities.
 *
 * @param iface A valid pointer on a network interface to send from
 * @param pkt A valid pointer on a packet to send
 * @param frag The fragment to be sent
 *
 * @return 0 on success, negative value otherwise
 */
int ieee802154_radio_send(struct net_if *iface, struct net_pkt *pkt, struct net_buf *frag);

/**
 * @brief This function implements the configured channel access algorithm (CSMA/CA, ALOHA,
 *        etc.). Currently only one implementation of this function may be compiled into the
 *        source code. The implementation will be selected via Kconfig variables (see
 *        @ref NET_L2_IEEE802154_RADIO_CSMA_CA and @ref NET_L2_IEEE802154_RADIO_ALOHA).
 *
 *        This method will be called by ieee802154_radio_send() to determine if and when the
 *        radio channel is clear to send. It blocks the thread during backoff if the selected
 *        algorithm implements a backoff strategy.
 *
 *        See sections 6.2.5 and 10.2.8.
 *
 * @param iface A valid pointer on a network interface to assesss
 *
 * @return 0 if the channel is clear to send, -EBUSY if a timeout was reached while waiting for
 *         a clear channel, other negative values to signal internal error conditions.
 */
int ieee802154_wait_for_clear_channel(struct net_if *iface);

/**
 * @brief Checks whether the given packet requires acknowledgment, and if so, prepares ACK
 *        reception on the TX path, i.e. sets up the necessary internal state before a transmission.
 *
 *        This function has side effects and must be called before each individual transmission
 *        attempt.
 *
 *        This function checks for and supports both, software and hardware acknowlegement,
 *        depending on driver capabilities.
 *
 *        See sections 6.7.4.1 through 6.7.4.3.
 *
 * @param iface A valid pointer on the network the packet will be transmitted to
 * @param pkt A valid pointer on a packet to send
 * @param frag The fragment that needs to be acknowledged
 *
 * @return true if the given packet requires acknowlegement, false otherwise.
 */
bool ieee802154_prepare_for_ack(struct net_if *iface, struct net_pkt *pkt, struct net_buf *frag);

/**
 * @brief Waits for ACK reception on the TX path with standard compliant timeout settings, i.e.
 * listens for incoming packages with the correct attributes and sequence number, see
 * section 6.7.4.4 (retransmissions).
 *
 * This function has side effects and must be called after each transmission attempt if (and only
 * if) @ref ieee802154_prepare_for_ack() had been called before.
 *
 * This function checks for and supports both, software and hardware acknowlegement, depending on
 * driver capabilities.
 *
 * @param iface A valid pointer on the network the packet was transmitted to
 * @param ack_required The return value from the corresponding call to
 *        @ref ieee802154_prepare_for_ack()
 *
 * @return 0 if no ACK was required or the exected ACK was received in time, -EIO if the expected
 *         ACK was not received within the standard compliant timeout.
 */
int ieee802154_wait_for_ack(struct net_if *iface, bool ack_required);

#endif /* __IEEE802154_PRIV_H__ */
