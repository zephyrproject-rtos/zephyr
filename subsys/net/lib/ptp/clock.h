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

#include <zephyr/kernel.h>

#include <zephyr/net/socket.h>
#include <zephyr/sys/slist.h>

#include "ds.h"
#include "port.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Threshold value for accepting PTP Instance for consideration in BTCA */
#define FOREIGN_TIME_TRANSMITTER_THRESHOLD 2

/** @brief Multiplication factor of message intervals to create time window for announce messages */
#define FOREIGN_TIME_TRANSMITTER_TIME_WINDOW_MUL 4

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
 * @brief Foreign TimeTransmitter record.
 */
struct ptp_foreign_tt_clock {
	/** @cond INTERNAL_HIDDEN */
	/** List object. */
	sys_snode_t	   node;
	/** @endcond */
	/** Foreign timeTransmitter's Port ID. */
	struct ptp_port_id port_id;
	/** List of messages received from Foreign timeTransmitter. */
	struct k_fifo	   messages;
	/** Number of messeges received within a FOREIGN_TIME_TRANSMITTER_TIME_WINDOW. */
	uint16_t	   messages_count;
	/** Generic dataset of the Foreign timeTransmitter for BTCA. */
	struct ptp_dataset dataset;
	/** Pointer to the receiver. */
	struct ptp_port	   *port;
};

/**
 * @brief Function initializing PTP Clock instance.
 *
 * @return Pointer to the structure representing PTP Clock instance.
 */
const struct ptp_clock *ptp_clock_init(void);

/**
 * @brief Function synchronizing local PTP Hardware Clock to the remote.
 *
 * @param[in] ingress Timestamp of the message reception from the remote node in nanoseconds.
 * @param[in] egress  Timestamp of the message transmission to the local node in nanoseconds.
 */
void ptp_clock_synchronize(uint64_t ingress, uint64_t egress);

/**
 * @brief Function updating PTP Clock path delay.
 *
 * @param[in] egress  Timestamp of the message transmission in nanoseconds.
 * @param[in] ingress Timestamp of the message reception on the remote node in nanoseconds.
 */
void ptp_clock_delay(uint64_t egress, uint64_t ingress);

/**
 * @brief Function for getting PTP Clock Default dataset.
 *
 * @return Pointer to the structure representing PTP Clock instance's Default dataset.
 */
const struct ptp_default_ds *ptp_clock_default_ds(void);

/**
 * @brief Function for getting PTP Clock Parent dataset.
 *
 * @return Pointer to the structure representing PTP Clock instance's Parent dataset.
 */
const struct ptp_parent_ds *ptp_clock_parent_ds(void);

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

/**
 * @brief Function returning Pointer to the best PTP Clock's Foreign TimeTransmitter record.
 *
 * @return Pointer to the clock's best Foreign TimeTransmitter or
 * NULL if there is no Foreign TimeTransmitter.
 */
const struct ptp_foreign_tt_clock *ptp_clock_best_time_transmitter(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_PTP_CLOCK_H_ */
