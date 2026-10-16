/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Rapid Spanning Tree Protocol internals
 *
 * Shared between the RSTP sources and the tests. Not part of the public API.
 * Clause and figure numbers refer to IEEE Std 802.1Q-2014, which carries the
 * RSTP of IEEE Std 802.1D-2004 in Clause 13 and the BPDU formats in Clause 14.
 */

#ifndef ZEPHYR_SUBSYS_NET_L2_ETHERNET_BRIDGE_STP_RSTP_INTERNAL_H_
#define ZEPHYR_SUBSYS_NET_L2_ETHERNET_BRIDGE_STP_RSTP_INTERNAL_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Length of an STP TCN BPDU (Figure 14-2) */
#define ETH_RSTP_BPDU_TCN_LEN    4U
/** Length of an STP Configuration BPDU (14.4) */
#define ETH_RSTP_BPDU_CONFIG_LEN 35U
/** Length of an RST BPDU (14.4) */
#define ETH_RSTP_BPDU_RST_LEN    36U
/** Longest BPDU that is encoded */
#define ETH_RSTP_BPDU_MAX_LEN    ETH_RSTP_BPDU_RST_LEN

/** BPDU flags, octet 5 (14.4) */
#define ETH_RSTP_FLAG_TC         BIT(0)
#define ETH_RSTP_FLAG_PROPOSAL   BIT(1)
#define ETH_RSTP_FLAG_ROLE_SHIFT 2U
#define ETH_RSTP_FLAG_ROLE_MASK  (BIT(2) | BIT(3))
#define ETH_RSTP_FLAG_LEARNING   BIT(4)
#define ETH_RSTP_FLAG_FORWARDING BIT(5)
#define ETH_RSTP_FLAG_AGREEMENT  BIT(6)
#define ETH_RSTP_FLAG_TC_ACK     BIT(7)

/** Port Role values in the flags (14.2.9) */
#define ETH_RSTP_BPDU_ROLE_UNKNOWN    0U
#define ETH_RSTP_BPDU_ROLE_ALT_BACKUP 1U
#define ETH_RSTP_BPDU_ROLE_ROOT       2U
#define ETH_RSTP_BPDU_ROLE_DESIGNATED 3U

/** BPDU types after validation (14.5) */
enum eth_rstp_bpdu_type {
	ETH_RSTP_BPDU_CONFIG,
	ETH_RSTP_BPDU_TCN,
	ETH_RSTP_BPDU_RST,
};

/** Timer values of a Configuration Message, in seconds */
struct eth_rstp_times {
	uint16_t message_age;
	uint16_t max_age;
	uint16_t hello_time;
	uint16_t forward_delay;
};

/** A decoded BPDU. A TCN BPDU uses only @p type. */
struct eth_rstp_bpdu {
	enum eth_rstp_bpdu_type type;
	/** ETH_RSTP_FLAG_* bits that are defined for @p type */
	uint8_t flags;
	uint64_t root_id;
	uint32_t root_path_cost;
	uint64_t bridge_id;
	uint16_t port_id;
	struct eth_rstp_times times;
};

/**
 * @brief Validate and decode a received BPDU
 *
 * @param buf BPDU, starting at the Protocol Identifier
 * @param len Length of @p buf
 * @param bpdu Decoded BPDU
 *
 * @retval 0 The BPDU is valid and was decoded.
 * @retval -EINVAL The BPDU is not a Configuration, TCN or RST BPDU (14.5 h).
 */
int eth_rstp_bpdu_decode(const uint8_t *buf, size_t len, struct eth_rstp_bpdu *bpdu);

/**
 * @brief Encode a BPDU for transmission
 *
 * @param bpdu BPDU to encode
 * @param buf Buffer for the BPDU, starting at the Protocol Identifier
 * @param len Size of @p buf
 *
 * @return Length of the encoded BPDU, or 0 if @p buf is too short.
 */
size_t eth_rstp_bpdu_encode(const struct eth_rstp_bpdu *bpdu, uint8_t *buf, size_t len);

/** Default Bridge Priority (Table 13-3) */
#define ETH_RSTP_DEFAULT_BRIDGE_PRIORITY 32768U
/** Default Port Priority (Table 13-3) */
#define ETH_RSTP_DEFAULT_PORT_PRIORITY   128U
/** Port Path Cost of a 100 Mb/s link (Table 13-4) */
#define ETH_RSTP_DEFAULT_PATH_COST       200000U
/** Highest Port Path Cost (Table 13-4) */
#define ETH_RSTP_MAX_PATH_COST           200000000U
/** Default and fixed timer values in seconds (Table 13-5) */
#define ETH_RSTP_DEFAULT_MAX_AGE         20U
#define ETH_RSTP_DEFAULT_FORWARD_DELAY   15U
#define ETH_RSTP_HELLO_TIME              2U
#define ETH_RSTP_MIGRATE_TIME            3U
/** Default Transmit Hold Count (Table 13-5) */
#define ETH_RSTP_DEFAULT_TX_HOLD_COUNT   6U

/** Port Roles (13.27.66) */
enum eth_rstp_role {
	ETH_RSTP_ROLE_DISABLED,
	ETH_RSTP_ROLE_ROOT,
	ETH_RSTP_ROLE_DESIGNATED,
	ETH_RSTP_ROLE_ALTERNATE,
	ETH_RSTP_ROLE_BACKUP,
};

/** Origin of the spanning tree information of a port (13.27.32) */
enum eth_rstp_info_is {
	ETH_RSTP_INFO_DISABLED,
	ETH_RSTP_INFO_MINE,
	ETH_RSTP_INFO_AGED,
	ETH_RSTP_INFO_RECEIVED,
};

/** Result of rcvInfo() (13.29.12) */
enum eth_rstp_rcvd_info {
	ETH_RSTP_RCVD_SUPERIOR_DESIGNATED,
	ETH_RSTP_RCVD_REPEATED_DESIGNATED,
	ETH_RSTP_RCVD_INFERIOR_DESIGNATED,
	ETH_RSTP_RCVD_INFERIOR_ROOT_ALTERNATE,
	ETH_RSTP_RCVD_OTHER,
};

/**
 * Spanning tree priority vector (13.10). RSTP has no Regional Root and no
 * Internal Root Path Cost: they are the Designated Bridge Identifier and 0.
 */
struct eth_rstp_prio {
	uint64_t root_id;
	uint32_t root_path_cost;
	uint64_t designated_bridge_id;
	uint16_t designated_port_id;
	/** Receiving Port Identifier, the tie breaker within a bridge */
	uint16_t rcv_port_id;
};

struct eth_rstp_bridge;
struct eth_rstp_port;

/** Effects of the protocol on the bridge. Called with the caller's lock held. */
struct eth_rstp_ops {
	/** Transmit a BPDU through @p port */
	void (*tx_bpdu)(struct eth_rstp_port *port, const struct eth_rstp_bpdu *bpdu);
	/**
	 * Start or stop learning and forwarding on @p port (13.29.4 to
	 * 13.29.7). Returns once the change is in effect.
	 */
	void (*set_port_state)(struct eth_rstp_port *port, bool learning, bool forwarding);
	/** Remove the learned FDB entries of @p port (13.27.28) */
	void (*flush_fdb)(struct eth_rstp_port *port);
};

/**
 * State of one bridge port. The fields are the variables of 13.25 and 13.27
 * of the CIST. The caller sets the management fields with the setters below
 * and reads the others, and must not write them.
 */
struct eth_rstp_port {
	struct eth_rstp_bridge *bridge;
	/** Owner data for the ops */
	void *user_data;

	/* Management, kept over BEGIN */
	uint16_t port_id;
	uint32_t port_path_cost;
	bool admin_edge;
	bool auto_edge;
	bool oper_p2p;
	bool port_enabled;

	/* Everything from here on is reset at BEGIN */

	/* Timers (13.25), in seconds */
	uint16_t edge_delay_while;
	uint16_t fd_while;
	uint16_t hello_when;
	uint16_t mdelay_while;
	uint16_t rb_while;
	uint16_t rcvd_info_while;
	uint16_t rr_while;
	uint16_t tc_while;
	uint16_t tx_count;

	enum eth_rstp_info_is info_is;
	enum eth_rstp_rcvd_info rcvd_info;
	enum eth_rstp_role role;
	enum eth_rstp_role selected_role;

	struct eth_rstp_prio designated_prio;
	struct eth_rstp_prio msg_prio;
	struct eth_rstp_prio port_prio;
	struct eth_rstp_times designated_times;
	struct eth_rstp_times msg_times;
	struct eth_rstp_times port_times;

	/** The BPDU that rcvdBpdu refers to */
	struct eth_rstp_bpdu rcvd;

	bool agree: 1;
	bool agreed: 1;
	bool disputed: 1;
	bool fdb_flush: 1;
	bool forward: 1;
	bool forwarding: 1;
	bool learn: 1;
	bool learning: 1;
	bool mcheck: 1;
	bool new_info: 1;
	bool oper_edge: 1;
	bool proposed: 1;
	bool proposing: 1;
	bool rcvd_bpdu: 1;
	bool rcvd_msg: 1;
	bool rcvd_rstp: 1;
	bool rcvd_stp: 1;
	bool rcvd_tc: 1;
	bool rcvd_tc_ack: 1;
	bool rcvd_tcn: 1;
	bool re_root: 1;
	bool reselect: 1;
	bool selected: 1;
	bool send_rstp: 1;
	bool sync: 1;
	bool synced: 1;
	bool tc_ack: 1;
	bool tc_prop: 1;
	bool tick: 1;
	bool updt_info: 1;

	/* State of each per port state machine */
	uint8_t prx_state;
	uint8_t ppm_state;
	uint8_t bdm_state;
	uint8_t ptx_state;
	uint8_t pim_state;
	uint8_t prt_state;
	uint8_t pst_state;
	uint8_t tcm_state;
};

/** State of one bridge (13.26) */
struct eth_rstp_bridge {
	const struct eth_rstp_ops *ops;
	/** Ports of the bridge, a NULL entry is not in use */
	struct eth_rstp_port **ports;
	size_t num_ports;

	uint64_t bridge_id;
	struct eth_rstp_times bridge_times;
	uint8_t force_version;
	uint8_t tx_hold_count;

	struct eth_rstp_prio root_prio;
	uint16_t root_port_id;
	struct eth_rstp_times root_times;

	uint8_t prs_state;
	/** eth_rstp_start() has been called */
	bool started;
	/** Runs that stopped before the state machines settled */
	uint32_t unsettled;
};

/**
 * @brief Initialize a bridge
 *
 * The bridge starts with the default parameters. The state machines run
 * from eth_rstp_start() on, the setters only store values before that.
 *
 * @param br Bridge
 * @param ops Effects of the protocol
 * @param ports Array of port pointers of the bridge, NULL for an unused one
 * @param num_ports Number of elements in @p ports
 * @param mac Bridge Address, the MAC address part of the Bridge Identifier
 */
void eth_rstp_bridge_init(struct eth_rstp_bridge *br, const struct eth_rstp_ops *ops,
			  struct eth_rstp_port **ports, size_t num_ports, const uint8_t mac[6]);

/**
 * @brief Initialize a port
 *
 * The port starts disabled, with the default priority and path cost. Add
 * it to the ports array of @p br before calling eth_rstp_start(), or call
 * eth_rstp_port_begin() after adding it to a running bridge.
 *
 * @param port Port
 * @param br Bridge of the port
 * @param port_number Port Number, 1 to 4095 (14.2.7)
 */
void eth_rstp_port_init(struct eth_rstp_port *port, struct eth_rstp_bridge *br,
			uint16_t port_number);

/**
 * @brief Assert BEGIN for all state machines of a bridge and run them
 *
 * @param br Bridge
 */
void eth_rstp_start(struct eth_rstp_bridge *br);

/**
 * @brief Stop running the state machines of a bridge
 *
 * The setters only store values until eth_rstp_start() is called again.
 *
 * @param br Bridge
 */
void eth_rstp_stop(struct eth_rstp_bridge *br);

/**
 * @brief Put the state machines of one port to their initial states
 *
 * Used for a port added to a running bridge.
 *
 * @param port Port
 */
void eth_rstp_port_begin(struct eth_rstp_port *port);

/**
 * @brief Run the state machines after one second has passed
 *
 * @param br Bridge
 */
void eth_rstp_tick(struct eth_rstp_bridge *br);

/**
 * @brief Process a BPDU received on a port
 *
 * @param port Receiving port
 * @param bpdu BPDU validated by eth_rstp_bpdu_decode()
 */
void eth_rstp_rx(struct eth_rstp_port *port, const struct eth_rstp_bpdu *bpdu);

/**
 * @brief Set whether a port can send and receive frames (13.27.45)
 *
 * @param port Port
 * @param enabled true if the link is up and the port is administratively up
 */
void eth_rstp_port_set_enabled(struct eth_rstp_port *port, bool enabled);

/**
 * @brief Set whether a port is attached to a point-to-point LAN (13.28.16)
 *
 * @param port Port
 * @param p2p true for a point-to-point link, false for a shared LAN
 */
void eth_rstp_port_set_p2p(struct eth_rstp_port *port, bool p2p);

/**
 * @brief Set the AdminEdge and AutoEdge parameters of a port (13.27.1, 13.27.18)
 *
 * @param port Port
 * @param admin_edge The port is an Edge Port from the start
 * @param auto_edge Detect an Edge Port from the absence of BPDUs
 */
void eth_rstp_port_set_edge(struct eth_rstp_port *port, bool admin_edge, bool auto_edge);

/**
 * @brief Set the Port Path Cost of a port (13.27.25)
 *
 * @param port Port
 * @param cost Port Path Cost, 1 to ETH_RSTP_MAX_PATH_COST
 *
 * @retval 0 The cost was set.
 * @retval -EINVAL The cost is out of range.
 */
int eth_rstp_port_set_path_cost(struct eth_rstp_port *port, uint32_t cost);

/**
 * @brief Set the priority component of a Port Identifier (13.27.46)
 *
 * @param port Port
 * @param priority Port Priority, 0 to 240 in steps of 16
 *
 * @retval 0 The priority was set.
 * @retval -EINVAL The priority is not a valid value.
 */
int eth_rstp_port_set_priority(struct eth_rstp_port *port, uint8_t priority);

/**
 * @brief Make a port test whether STP bridges are still attached (13.27.38)
 *
 * @param port Port
 */
void eth_rstp_port_mcheck(struct eth_rstp_port *port);

/**
 * @brief Set the priority component of the Bridge Identifier (13.26.2)
 *
 * @param br Bridge
 * @param priority Bridge Priority, 0 to 61440 in steps of 4096
 *
 * @retval 0 The priority was set.
 * @retval -EINVAL The priority is not a valid value.
 */
int eth_rstp_set_bridge_priority(struct eth_rstp_bridge *br, uint16_t priority);

/**
 * @brief Set the Bridge Address, the MAC address part of the Bridge Identifier
 *
 * @param br Bridge
 * @param mac Bridge Address
 */
void eth_rstp_set_bridge_address(struct eth_rstp_bridge *br, const uint8_t mac[6]);

/**
 * @brief Set the Bridge Max Age and Bridge Forward Delay (13.25)
 *
 * @param br Bridge
 * @param max_age Bridge Max Age in seconds, 6 to 40
 * @param forward_delay Bridge Forward Delay in seconds, 4 to 30
 *
 * @retval 0 The times were set.
 * @retval -EINVAL A value is out of range, or 2 * (forward_delay - 1) is less
 *         than max_age, or max_age is less than 2 * (Hello Time + 1).
 */
int eth_rstp_set_times(struct eth_rstp_bridge *br, uint16_t max_age, uint16_t forward_delay);

/**
 * @brief Set the Transmit Hold Count (13.26.12)
 *
 * @param br Bridge
 * @param count Transmit Hold Count, 1 to 10
 *
 * @retval 0 The count was set.
 * @retval -EINVAL The count is out of range.
 */
int eth_rstp_set_tx_hold_count(struct eth_rstp_bridge *br, uint8_t count);

/**
 * @brief Set the Force Protocol Version (13.7.2)
 *
 * Restarts the protocol on all ports.
 *
 * @param br Bridge
 * @param version 0 for STP compatibility, 2 for RSTP
 *
 * @retval 0 The version was set.
 * @retval -EINVAL The version is neither 0 nor 2.
 */
int eth_rstp_set_force_version(struct eth_rstp_bridge *br, uint8_t version);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SUBSYS_NET_L2_ETHERNET_BRIDGE_STP_RSTP_INTERNAL_H_ */
