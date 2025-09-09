/*
 * Copyright (c) 2019, Prevas A/S
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief UDP transport for the MCUmgr SMP protocol.
 * @ingroup mcumgr_transport_udp
 */

#ifndef ZEPHYR_INCLUDE_MGMT_SMP_UDP_H_
#define ZEPHYR_INCLUDE_MGMT_SMP_UDP_H_

/**
 * @brief This allows to use the MCUmgr SMP protocol over UDP.
 * @defgroup mcumgr_transport_udp UDP transport
 * @ingroup mcumgr_transport
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief	Enables the UDP SMP MCUmgr transport thread(s) which will open a socket and
 *		listen to requests.
 *
 * @note	API is not thread safe.
 *
 * @return	0 on success
 * @return	-errno code on failure.
 */
int smp_udp_open(void);

/**
 * @brief	Disables the UDP SMP MCUmgr transport thread(s) which will close open sockets.
 *
 * @note	API is not thread safe.
 *
 * @return	0 on success
 * @return	-errno code on failure.
 */
int smp_udp_close(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif
