/*
 * Copyright (c) 2024 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file port.h
 * @brief PTP port data structure and interface to operate on PTP Ports.
 *
 * References are to version 2019 of IEEE 1588, ("PTP")
 */

#ifndef ZEPHYR_INCLUDE_PTP_PORT_H_
#define ZEPHYR_INCLUDE_PTP_PORT_H_

#include <stdbool.h>

#include <zephyr/kernel.h>

#include "ds.h"
#include "state_machine.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PTP_PORT_TIMER_ANNOUNCE_TO	(0)
#define PTP_PORT_TIMER_DELAY_TO		(1)
#define PTP_PORT_TIMER_SYNC_TO		(2)
#define PTP_PORT_TIMER_QUALIFICATION_TO (3)

/**
 * @brief Structure describing PTP Port.
 */
struct ptp_port {
	/** Object list. */
	sys_snode_t		    node;
	/** PTP Port Dataset*/
	struct ptp_port_ds	    port_ds;
	/** Interface related to the Port. */
	struct net_if		    *iface;
	/** Array of BSD sockets. */
	int			    socket[2];
	/** Structure of system timers used by the Port. */
	struct {
		struct k_timer      announce;
		struct k_timer      delay;
		struct k_timer      sync;
		struct k_timer      qualification;
	} timers;
	/** Bitmask of tiemouts. */
	atomic_t		    timeouts;
	/** Structure of unique sequence IDs used for messages. */
	struct {
		uint16_t	    announce;
		uint16_t	    delay;
		uint16_t	    signaling;
		uint16_t	    sync;
	} seq_id;
	/** Pointer to finite state machine. */
	enum ptp_port_state	    (*state_machine)(enum ptp_port_state state,
						     enum ptp_port_event event,
						     bool tt_diff);
	/** Pointer to the Port's best Foreign TimeTransmitter. */
	struct ptp_foreign_tt_clock *best;
	/** List of Foreign TimeTransmitters discovered through received Announce messages. */
	sys_slist_t		    foreign_list;
};

/**
 * @brief Function initializing PTP Port.
 *
 * @param[in] iface     Pointer to current network interface.
 * @param[in] user_data Unused argument needed to comply with @ref net_if_cb_t type.
 */
void ptp_port_init(struct net_if *iface, void *user_data);

/**
 * @brief Function checking if two port identities are equal.
 *
 * @param[in] p1 Pointer to the port identity structure.
 * @param[in] p2 Pointer to the port identity structure.
 *
 * @return True if port identities are equal, False otherwise.
 */
bool ptp_port_id_eq(const struct ptp_port_id *p1, const struct ptp_port_id *p2);

/**
 * @brief Function adding foreign TimeTransmitter Clock for the PTP Port based on specified message.
 *
 * @param[in] port Pointer to the PTP Port.
 * @param[in] msg  Pointer to the announce message containg PTP TimeTransmitter data.
 *
 * @return Non-zero if the announce message is different than the last.
 */
int ptp_port_add_foreign_tt(struct ptp_port *port, struct ptp_msg *msg);

/**
 * @brief Function freeing memory used by foreign timeTransmitters assigned to given PTP Port.
 *
 * @param[in] port Pointer to the PTP Port.
 */
void ptp_port_free_foreign_tts(struct ptp_port *port);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_PTP_PORT_H_ */
