/*
 * Copyright (c) 2024 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file clock.h
 * @brief PTP Clock data structure and interface API
 *
 * References are to version 2019 of IEEE 1588, ("PTP")
 */

#ifndef ZEPHYR_INCLUDE_PTP_CLOCK_H_
#define ZEPHYR_INCLUDE_PTP_CLOCK_H_

#include <zephyr/net/socket.h>

#include "ds.h"
#include "port.h"

#ifdef __cplusplus
extern "C" {
#endif

/* PTP Clock structure declaration. */
struct ptp_clock;

/**
 * @brief Types of PTP Clocks.
 */
enum ptp_clock_type {
	PTP_CLOCK_TYPE_ORDINARY,
	PTP_CLOCK_TYPE_BOUNDARY,
	PTP_CLOCK_TYPE_P2P,
	PTP_CLOCK_TYPE_E2E,
	PTP_CLOCK_TYPE_MANAGEMENT,
};

/**
 * @brief PTP Clock time source.
 */
enum ptp_time_src {
	PTP_TIME_SRC_ATOMIC_CLK = 0x10,
	PTP_TIME_SRC_GNSS = 0x20,
	PTP_TIME_SRC_TERRESTRIAL_RADIO = 0x30,
	PTP_TIME_SRC_SERIAL_TIME_CODE = 0x39,
	PTP_TIME_SRC_PTP = 0x40,
	PTP_TIME_SRC_NTP = 0x50,
	PTP_TIME_SRC_HAND_SET = 0x60,
	PTP_TIME_SRC_OTHER = 0x90,
	PTP_TIME_SRC_INTERNAL_OSC = 0xA0,
};

/**
 * @brief Function initializing PTP Clock instance.
 *
 * @return Pointer to the structure representing PTP Clock instance.
 */
const struct ptp_clock *ptp_clock_init(void);

/**
 * @brief Function for getting PTP Clock Default dataset.
 *
 * @return Pointer to the structure representing PTP Clock instance's Default dataset.
 */
const struct ptp_default_ds *ptp_clock_default_ds(void);

/**
 * @brief Function invalidating PTP Clock's array of file descriptors used for sockets.
 */
void ptp_clock_pollfd_invalidate(void);

/**
 * @brief Function adding PTP Port to the PTP Clock's list of initialized PTP Ports.
 *
 * @param[in] port Pointer to the PTP Port to be added to the PTP Clock's list.
 */
void ptp_clock_port_add(struct ptp_port *port);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_PTP_CLOCK_H_ */
