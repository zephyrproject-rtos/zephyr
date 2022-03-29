/*
 * Copyright (c) 2019, Prevas A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief UDP transport for the mcumgr SMP protocol.
 */

#ifndef ZEPHYR_INCLUDE_MGMT_SMP_UDP_H_
#define ZEPHYR_INCLUDE_MGMT_SMP_UDP_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Opens a UDP socket for the SMP UDP service.
 *
 * @return 0 on success; negative error code on failure.
 */
int smp_udp_open(void);

/**
 * @brief Closes the UDP socket for the SMP UDP service.
 *
 * @return 0 on success; negative error code on failure.
 */
int smp_udp_close(void);

#ifdef __cplusplus
}
#endif

#endif
