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
	/** Number of messages received within a FOREIGN_TIME_TRANSMITTER_TIME_WINDOW. */
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
 * @brief Function polling all sockets descriptors for incoming PTP messages.
 *
 * @return Pointer to the first element of array of socket descriptors assigned to PTP Ports.
 */
struct zsock_pollfd *ptp_clock_poll_sockets(void);

/**
 * @brief Function handling STATE DECISION EVENT for the PTP Clock instance.
 */
void ptp_clock_handle_state_decision_evt(void);

/**
 * @brief Function processing received PTP Management message at the PTP Clock level.
 *
 * @param[in] port PTP Port that received the message.
 * @param[in] msg  Received PTP Management message.
 *
 * @return 1 if the message processing results in State Decision Event.
 */
int ptp_clock_management_msg_process(struct ptp_port *port, struct ptp_msg *msg);

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
 * @brief Function for getting list of PTP Ports for the PTP Clock instance.
 *
 * @return Pointer to the single-linked list of PTP Ports.
 */
sys_slist_t *ptp_clock_ports_list(void);

/**
 * @brief Function returning PTP Clock type of the instance.
 *
 * @return PTP Clock type of the instance.
 */
enum ptp_clock_type ptp_clock_type(void);

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
 * @brief Function for getting PTP Clock Current dataset.
 *
 * @return Pointer to the structure representing PTP Clock instance's Current dataset.
 */
const struct ptp_current_ds *ptp_clock_current_ds(void);

/**
 * @brief Function for getting PTP Clock Time Properties dataset.
 *
 * @return Pointer to the structure representing PTP Clock instance's Time Properties dataset.
 */
const struct ptp_time_prop_ds *ptp_clock_time_prop_ds(void);

/**
 * @brief Function for getting a common dataset for the clock's default dataset.
 *
 * @return Pointer to the common dataset that contain data from ptp_default_ds structure.
 */
const struct ptp_dataset *ptp_clock_ds(void);

/**
 * @brief Function for getting a common dataset for the clock's best foreign timeTransmitter clock.
 *
 * @return NULL if the clock doesn't have best foreign timeTransmitter clock or
 * pointer to the ptp_dataset of the best foreign timeTransmitter clock.
 */
const struct ptp_dataset *ptp_clock_best_foreign_ds(void);

/**
 * @brief Get PTP Port from network interface.
 *
 * @param[in] iface Pointer to the network interface.
 *
 * @return Pointer to the PTP Port binded with given interface. If no PTP Port assigned
 * to the interface NULL is returned.
 */
struct ptp_port *ptp_clock_port_from_iface(struct net_if *iface);

/**
 * @brief Function invalidating PTP Clock's array of file descriptors used for sockets.
 */
void ptp_clock_pollfd_invalidate(void);

/**
 * @brief Function signalling timoeout of one of PTP Ports timer
 * to the PTP Clock's file descriptor. The function should be called only from
 * the context of timer expiration function.
 */
void ptp_clock_signal_timeout(void);

/**
 * @brief Function signalling to the PTP Clock that STATE_DECISION_EVENT occurred and
 * it needs to be handled.
 */
void ptp_clock_state_decision_req(void);

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

/**
 * @brief Function checking if given PTP Clock IDs are the same.
 *
 * @param[in] c1 Pointer to the PTP Clock ID.
 * @param[in] c2 Pointer to the PTP Clock ID.
 *
 * @return True if the same, false otherwise.
 */
bool ptp_clock_id_eq(const ptp_clk_id *c1, const ptp_clk_id *c2);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_PTP_CLOCK_H_ */
