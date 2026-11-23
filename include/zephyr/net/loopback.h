/** @file
 * @brief Loopback control interface
 */

/*
 * Copyright (c) 2022 Radarxense B.V.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NET_LOOPBACK_H_
#define ZEPHYR_INCLUDE_NET_LOOPBACK_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_NET_LOOPBACK_SIMULATE_PACKET_DROP
/**
 * @brief Set the packet drop rate
 *
 * @param[in] ratio Value between 0 = no packet loss and 1 = all packets dropped
 *
 * @return 0 on success, otherwise a negative integer.
 */
int loopback_set_packet_drop_ratio(float ratio);

/**
 * @brief Get the number of dropped packets
 *
 * @return number of packets dropped by the loopback interface
 */
int loopback_get_num_dropped_packets(void);
#endif

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_NET_LOOPBACK_H_ */
