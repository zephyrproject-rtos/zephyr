/** @file
 @brief RPL data handler

 This is not to be included by the application.
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __RPL_H
#define __RPL_H

#include <kernel.h>
#include <stdbool.h>

#include <net/net_ip.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CONFIG_NET_RPL)

#include "route.h"

#define NET_ICMPV6_RPL			155  /* RPL Control Message */

#define NET_RPL_DODAG_SOLICIT		0x00
#define NET_RPL_DODAG_INFO_OBJ		0x01
#define NET_RPL_DEST_ADV_OBJ		0x02
#define NET_RPL_DEST_ADV_OBJ_ACK	0x03
#define NET_RPL_SEC_DODAG_SOLICIT	0x80
#define NET_RPL_SEC_DODAG_INFO_OBJ	0x81
#define NET_RPL_SEC_DEST_ADV_OBJ	0x82
#define NET_RPL_SEC_DEST_ADV_OBJ_ACK	0x83
#define NET_RPL_CONSISTENCY_CHECK	0x8A

/* Routing Metric/Constraint Type, RFC 6551 ch 6.1 */
#define NET_RPL_MC_NONE			0  /* Local identifier for empty MC */
#define NET_RPL_MC_NSA                  1  /* Node State and Attributes     */
#define NET_RPL_MC_ENERGY               2  /* Node Energy                   */
#define NET_RPL_MC_HOPCOUNT             3  /* Hop Count                     */
#define NET_RPL_MC_THROUGHPUT           4  /* Throughput                    */
#define NET_RPL_MC_LATENCY              5  /* Latency                       */
#define NET_RPL_MC_LQL                  6  /* Link Quality Level            */
#define NET_RPL_MC_ETX                  7  /* Expected Transmission Count   */
#define NET_RPL_MC_LC                   8  /* Link Color                    */

/* Routing Metric/Constraint Common Header Flag Field, RFC 6551, ch 6.3 */
#define NET_RPL_MC_FLAG_P               BIT(5)
#define NET_RPL_MC_FLAG_C               BIT(6)
#define NET_RPL_MC_FLAG_O               BIT(7)
#define NET_RPL_MC_FLAG_R               BIT(8)

/* Routing Metric/Constraint Common Header A Field, RFC 6551, ch 6.4 */
#define NET_RPL_MC_A_ADDITIVE		0
#define NET_RPL_MC_A_MAXIMUM		1
#define NET_RPL_MC_A_MINIMUM		2
#define NET_RPL_MC_A_MULTIPLICATIVE	3

/* Node type field, RFC 6551, ch 6.7 */
#define NET_RPL_MC_NODE_TYPE_MAINS	0
#define NET_RPL_MC_NODE_TYPE_BATTERY	1
#define NET_RPL_MC_NODE_TYPE_SCAVENGING	2

/* The bit index within the flags field of the RPL metric object energy
 * structure.
 */
#define NET_RPL_MC_ENERGY_INCLUDED	3
#define NET_RPL_MC_ENERGY_TYPE		1
#define NET_RPL_MC_ENERGY_ESTIMATION	0

/* RPL control message options. */
#define NET_RPL_OPTION_PAD1                 0
#define NET_RPL_OPTION_PADN                 1
#define NET_RPL_OPTION_DAG_METRIC_CONTAINER 2
#define NET_RPL_OPTION_ROUTE_INFO           3
#define NET_RPL_OPTION_DAG_CONF             4
#define NET_RPL_OPTION_TARGET               5
#define NET_RPL_OPTION_TRANSIT              6
#define NET_RPL_OPTION_SOLICITED_INFO       7
#define NET_RPL_OPTION_PREFIX_INFO          8
#define NET_RPL_OPTION_TARGET_DESC          9

#define NET_RPL_MAX_RANK_INC		(7 * CONFIG_NET_RPL_MIN_HOP_RANK_INC)
#define NET_RPL_INFINITE_RANK		0xffff

/* Rank of a root node. */
#define NET_RPL_ROOT_RANK(instance)	((instance)->min_hop_rank_inc)

#define NET_RPL_DAG_RANK(rank, instance) ((rank) / \
					  (instance)->min_hop_rank_inc)

/*
 * The ETX in the metric container is expressed as a fixed-point value
 * whose integer part can be obtained by dividing the value by
 * NET_RPL_MC_ETX_DIVISOR.
 */
#define NET_RPL_MC_ETX_DIVISOR		256

/* DAG Mode of Operation */
#define NET_RPL_MOP_NO_DOWNWARD_ROUTES      0
#define NET_RPL_MOP_NON_STORING             1
#define NET_RPL_MOP_STORING_NO_MULTICAST    2
#define NET_RPL_MOP_STORING_MULTICAST       3

#if defined(CONFIG_NET_RPL_DEFAULT_INSTANCE)
#define NET_RPL_DEFAULT_INSTANCE CONFIG_NET_RPL_DEFAULT_INSTANCE
#else
#define NET_RPL_DEFAULT_INSTANCE 0x1e
#endif

#define NET_RPL_PARENT_FLAG_UPDATED           0x1
#define NET_RPL_PARENT_FLAG_LINK_METRIC_VALID 0x2

/* RPL IPv6 extension header option. */
#define NET_RPL_HDR_OPT_LEN		4
#define NET_RPL_HOP_BY_HOP_LEN		(NET_RPL_HDR_OPT_LEN + 2 + 2)
#define NET_RPL_HDR_OPT_DOWN		0x80
#define NET_RPL_HDR_OPT_DOWN_SHIFT	7
#define NET_RPL_HDR_OPT_RANK_ERR	0x40
#define NET_RPL_HDR_OPT_RANK_ERR_SHIFT	6
#define NET_RPL_HDR_OPT_FWD_ERR		0x20
#define NET_RPL_HDR_OPT_FWD_ERR_SHIFT	5

/**
 * RPL modes
 *
 * The RPL module can be in either of three modes: mesh mode
 * (NET_RPL_MODE_MESH), feather mode (NET_RPL_MODE_FEATHER), and leaf mode
 * (NET_RPL_MODE_LEAF). In mesh mode, nodes forward data for other nodes,
 * and are reachable by others. In feather mode, nodes can forward
 * data for other nodes, but are not reachable themselves. In leaf
 * mode, nodes do not forward data for others, but are reachable by
 * others.
 */
enum net_rpl_mode {
	NET_RPL_MODE_MESH,
	NET_RPL_MODE_FEATHER,
	NET_RPL_MODE_LEAF,
};

/**
 * Flag values in DAO message.
 */
#define NET_RPL_DAO_K_FLAG                0x80  /* DAO ACK requested */
#define NET_RPL_DAO_D_FLAG                0x40  /* DODAG ID present */

#define NET_RPL_LOLLIPOP_MAX_VALUE        255
#define NET_RPL_LOLLIPOP_CIRCULAR_REGION  127
#define NET_RPL_LOLLIPOP_SEQUENCE_WINDOWS 16

static inline u8_t net_rpl_lollipop_init(void)
{
	return NET_RPL_LOLLIPOP_MAX_VALUE -
		NET_RPL_LOLLIPOP_SEQUENCE_WINDOWS + 1;
}

static inline void net_rpl_lollipop_increment(u8_t *counter)
{
	if (*counter > NET_RPL_LOLLIPOP_CIRCULAR_REGION) {
		*counter = (*counter + 1) & NET_RPL_LOLLIPOP_MAX_VALUE;
	} else {
		*counter = (*counter + 1) & NET_RPL_LOLLIPOP_CIRCULAR_REGION;
	}
}

static inline bool net_rpl_lollipop_is_init(u8_t counter)
{
	return counter > NET_RPL_LOLLIPOP_CIRCULAR_REGION;
}

struct net_rpl_instance;
struct net_rpl_dag;

/**
 * @brief DIO prefix suboption.
 */
struct net_rpl_prefix {
	/** IPv6 address prefix */
	struct in6_addr prefix;

	/** Lifetime of the prefix */
	u32_t lifetime;

	/** Length of the prefix */
	u8_t length;

	/** Prefix flags */
	u8_t flags;
};

/**
 * @brief Node energy object, RFC 6551, ch 3.2
 */
struct net_rpl_node_energy_object {
	/** Energy node flags */
	u8_t flags;

	/** Energy estimation */
	u8_t estimation;
};

/**
 * @brief DAG Metric Container. RFC 6551, ch 2.1
 */
struct net_rpl_metric_container {
	/** Type of the container */
	u8_t type;

	/** Container flags */
	u8_t flags;

	/** Aggregated value (A field) */
	u8_t aggregated;

	/**
	 * Precedence of this Routing Metric/Constraint object relative
	 * to other objects in the container.
	 */
	u8_t precedence;

	/** Length of the object body */
	u8_t length;

	/** Metric container information */
	union metric_object {
		struct net_rpl_node_energy_object energy;
		u16_t etx;
	} obj;
};

/**
 * @brief Parent information.
 */
struct net_rpl_parent {
	/** Used DAG */
	struct net_rpl_dag *dag;

	/** Used metric container */
	struct net_rpl_metric_container mc;

	/** When last transmit happened */
	u32_t last_tx_time;

	/** Rank of the parent */
	u16_t rank;

	/** Destination Advertisement Trigger Sequence Number */
	u8_t dtsn;

	/** Parent flags */
	u8_t flags;
};

/**
 * @brief Directed Acyclic Graph (DAG)
 */
struct net_rpl_dag {
	/** DAG id */
	struct in6_addr dag_id;

	/** What is the preferred parent. */
	struct net_rpl_parent *preferred_parent;

	/** IPv6 prefix information */
	struct net_rpl_prefix prefix_info;

	/** Used RPL instance */
	struct net_rpl_instance *instance;

	/** Minimum rank */
	u16_t min_rank;

	/** DAG rank */
	u16_t rank;

	/** DAG version. */
	u8_t version;

	/** DODAG preference. */
	u8_t preference : 3;

	/** Is this DAG used or not. */
	u8_t is_used : 1;

	/** Is DAG grounded or floating. */
	u8_t is_grounded : 1;

	/** Is DAG joined or not. */
	u8_t is_joined : 1;

	u8_t _unused : 2;
};

/**
 * @brief Get related neighbor information from parent pointer.
 *
 * @param data Pointer to parent.
 *
 * @return Neighbor pointer if found, NULL if neighbor is not found.
 */
struct net_nbr *net_rpl_get_nbr(struct net_rpl_parent *data);

/**
 * @brief Get related neighbor data from parent pointer.
 *
 * @param data Pointer to parent.
 *
 * @return Neighbor data pointer if found, NULL if neighbor is not found.
 */
struct net_ipv6_nbr_data *
net_rpl_get_ipv6_nbr_data(struct net_rpl_parent *parent);

/**
 * @brief RPL object function (OF) reset.
 *
 * @details Reset the OF state for a specific DAG. This function is called when
 * doing a global repair on the DAG.
 *
 * @param dag Pointer to DAG object
 */
extern void net_rpl_of_reset(struct net_rpl_dag *dag);

/**
 * @brief RPL object function (OF) neighbor link callback.
 *
 * @details Receives link-layer neighbor information. The etx parameter
 * specifies the current ETX(estimated transmissions) for the neighbor.
 *
 * @param iface Network interface
 * @param parent Parent of the neighbor
 * @param status Transmission status
 * @param ext Estimated transmissions value
 *
 * @return 0 if ok, < 0 if error
 */
extern int net_rpl_of_neighbor_link_cb(struct net_if *iface,
				       struct net_rpl_parent *parent,
				       int status, int etx);

/**
 * @brief RPL object function (OF) get best parent.
 *
 * @details Compares two parents and returns the best one.
 *
 * @param iface Network interface.
 * @param parentA First parent.
 * @param parentB Second parent.
 *
 * @return Best parent is returned.
 */
extern struct net_rpl_parent *
net_rpl_of_best_parent(struct net_if *iface,
		       struct net_rpl_parent *parentA,
		       struct net_rpl_parent *parentB);

/**
 * @brief RPL object function (OF) get best DAG.
 *
 * @details Compares two DAGs and returns the best one.
 *
 * @param dagA First DAG.
 * @param dagB Second DAG.
 *
 * @return Best DAG.
 */
extern struct net_rpl_dag *net_rpl_of_best_dag(struct net_rpl_dag *dagA,
					       struct net_rpl_dag *dagB);

/**
 * @brief RPL object function (OF) calculate rank.
 *
 * @details Calculates a rank value using the parent rank and a base rank.
 * If parent is not set, the OF selects a default increment that is
 * added to the base rank. Otherwise, the OF uses information known
 * about parent to select an increment to the base rank.
 */
extern u16_t net_rpl_of_calc_rank(struct net_rpl_parent *parent,
				  u16_t rank);

/**
 * @brief RPL object function (OF) update metric container.
 *
 * @details Updates the metric container for outgoing DIOs in a certain DAG.
 * If the OF of the DAG does not use metric containers, the function
 * should set the object type to NET_RPL_MC_NONE.
 *
 * @param instance Pointer to RPL instance.
 */
extern int net_rpl_of_update_mc(struct net_rpl_instance *instance);

/**
 * @brief RPL object function (OF) objective code point used.
 *
 * @details Check if we support desired objective function.
 *
 * @param ocp Objective Code Point value
 *
 * @return true if OF is supported, false otherwise.
 */
extern bool net_rpl_of_find(u16_t ocp);

/**
 * @brief Return RPL object function (OF) objective code point used.
 *
 * @return OCP (Objective Code Point) value used.
 */
extern u16_t net_rpl_of_get(void);

/**
 * @brief RPL instance structure
 *
 * Describe RPL instance.
 */
struct net_rpl_instance {
	/** Routing metric information */
	struct net_rpl_metric_container mc;

	/** All the DAGs for this RPL instance */
	struct net_rpl_dag dags[CONFIG_NET_RPL_MAX_DAG_PER_INSTANCE];

#if defined(CONFIG_NET_RPL_PROBING)
	/** When next probe message will be sent. */
	struct k_delayed_work probing_timer;
#endif /* CONFIG_NET_RPL_PROBING */

	/** DODAG Information Object timer. */
	struct k_delayed_work dio_timer;

	/** Destination Advertisement Object timer. */
	struct k_delayed_work dao_timer;

	/** DAO lifetime timer. */
	struct k_delayed_work dao_lifetime_timer;

#if defined(CONFIG_NET_RPL_DAO_ACK)
	/** DAO retransmit timer */
	struct k_delayed_work dao_retransmit_timer;

	/** DAO number of retransmissions */
	u8_t dao_transmissions;
#endif

	/** Network interface to send DAO */
	struct net_if *iface;

	/** Current DAG in use */
	struct net_rpl_dag *current_dag;

	/** Current default router information */
	struct net_if_router *default_route;

	/** Amount of time for completion of dio interval */
	u32_t dio_next_delay;

	/** Objective Code Point (Used objective function) */
	u16_t ocp;

	/** MaxRankIncrease, RFC 6550, ch 6.7.6 */
	u16_t max_rank_inc;

	/** MinHopRankIncrease, RFC 6550, ch 6.7.6 */
	u16_t min_hop_rank_inc;

	/**
	 * Provides the unit in seconds that is used to express route
	 * lifetimes in RPL.  For very stable networks, it can be hours
	 * to days. RFC 6550, ch 6.7.6
	 */
	u16_t lifetime_unit;

#if defined(CONFIG_NET_STATISTICS_RPL)
	/** Number of DIO intervals for this RPL instance. */
	u16_t dio_intervals;

	/** Number of DIOs sent for this RPL instance. */
	u16_t dio_send_pkt;

	/** Number of DIOs received for this RPL instance. */
	u16_t dio_recv_pkt;
#endif /* CONFIG_NET_STATISTICS_RPL */

	/**
	 * This is the lifetime that is used as default for all RPL routes.
	 * It is expressed in units of Lifetime Units, e.g., the default
	 * lifetime in seconds is (Default Lifetime) * (Lifetime Unit)
	 * RFC 6550, ch 6.7.6
	 */
	u8_t default_lifetime;

	/** Instance ID of this RPL instance */
	u8_t instance_id;

	/** Destination Advertisement Trigger Sequence Number */
	u8_t dtsn;

	/** Mode of operation */
	u8_t mop;

	/** DIOIntervalDoublings, RFC 6550, ch 6.7.6 */
	u8_t dio_interval_doublings;

	/** DIOIntervalMin, RFC 6550, ch 6.7.6 */
	u8_t dio_interval_min;

	/** Current DIO interval */
	u8_t dio_interval_current;

	/** DIORedundancyConstant, ch 6.7.6 */
	u8_t dio_redundancy;

	/** Current number of DIOs received (temp var) */
	u8_t dio_counter;

	/** Keep track of whether we can send DIOs or not (true if we can send)
	 */
	bool dio_send;

	/** Is this RPL instance used or not */
	bool is_used;

	/** Is DAO timer active or not. */
	bool dao_timer_active;

	/** Is DAO lifetime timer active or not. */
	bool dao_lifetime_timer_active;
};

/**
 * @brief RPL DIO structure
 *
 * Describe RPL DAG Information Object.
 */
struct net_rpl_dio {
	/** Routing metric information */
	struct net_rpl_metric_container mc;

	/** DAG id */
	struct in6_addr dag_id;

	/** IPv6 prefix information */
	struct net_rpl_prefix prefix_info;

	/** IPv6 destination prefix */
	struct net_rpl_prefix destination_prefix;

	/** Objective Code Point (OF being used) */
	u16_t ocp;

	/** Current rank */
	u16_t rank;

	/** MaxRankIncrease, RFC 6550, ch 6.7.6 */
	u16_t max_rank_inc;

	/** MinHopRankIncrease, RFC 6550, ch 6.7.6 */
	u16_t min_hop_rank_inc;
	/**
	 * Provides the unit in seconds that is used to express route
	 * lifetimes in RPL.  For very stable networks, it can be hours
	 * to days. RFC 6550, ch 6.7.6
	 */
	u16_t lifetime_unit;

	/**
	 * This is the lifetime that is used as default for all RPL routes.
	 * It is expressed in units of Lifetime Units, e.g., the default
	 * lifetime in seconds is (Default Lifetime) * (Lifetime Unit)
	 * RFC 6550, ch 6.7.6
	 */
	u8_t default_lifetime;

	/** Instance ID of this RPL instance */
	u8_t instance_id;

	/** Destination Advertisement Trigger Sequence Number */
	u8_t dtsn;

	/** Mode of operation */
	u8_t mop;

	/** DAG interval doublings */
	u8_t dag_interval_doublings;

	/** DAG interval min */
	u8_t dag_interval_min;

	/** DAG interval */
	u8_t dag_interval_current;

	/** DAG redundancy constant */
	u8_t dag_redundancy;

	/** Is this DAG grounded or floating */
	u8_t grounded;

	/** DODAG preference */
	u8_t preference;

	/** DODAG version number */
	u8_t version;
};

/**
 * @brief RPL route information source
 */
enum net_rpl_route_source {
	NET_RPL_ROUTE_INTERNAL,
	NET_RPL_ROUTE_UNICAST_DAO,
	NET_RPL_ROUTE_MULTICAST_DAO,
	NET_RPL_ROUTE_DIO,
};

/**
 * @brief RPL route entry
 *
 * Stores extra information for RPL route.
 */
struct net_rpl_route_entry {
	/** Dag info for this route */
	struct net_rpl_dag *dag;

	/** Lifetime for this route entry (in seconds) */
	u32_t lifetime;

	/** Where this route came from */
	enum net_rpl_route_source route_source;

	/** No-Path target option with lifetime 0 received (true) or (false) */
	bool no_path_received;
};

/**
 * The extra data size is used in route.h to determine how much extra
 * space to allocate for RPL specific data.
 */
#define NET_ROUTE_EXTRA_DATA_SIZE sizeof(struct net_rpl_route_entry)

/**
 * @brief Check if the IPv6 address is a RPL multicast address.
 *
 * @param addr IPv6 address
 *
 * @return True if address is RPL multicast address, False otherwise.
 */
static inline bool net_rpl_is_ipv6_addr_mcast(const struct in6_addr *addr)
{
	return UNALIGNED_GET(&addr->s6_addr32[0]) == htonl(0xff020000) &&
		UNALIGNED_GET(&addr->s6_addr32[1]) == 0x00000000 &&
		UNALIGNED_GET(&addr->s6_addr32[2]) == 0x00000000 &&
		UNALIGNED_GET(&addr->s6_addr32[3]) == htonl(0x0000001a);
}

/**
 *  @brief Create RPL IPv6 multicast address FF02::1a
 *
 *  @param addr IPv6 address.
 *
 *  @return Pointer to given IPv6 address.
 */
static inline
struct in6_addr *net_rpl_create_mcast_address(struct in6_addr *addr)
{
	addr->s6_addr[0]   = 0xFF;
	addr->s6_addr[1]   = 0x02;
	UNALIGNED_PUT(0, &addr->s6_addr16[1]);
	UNALIGNED_PUT(0, &addr->s6_addr16[2]);
	UNALIGNED_PUT(0, &addr->s6_addr16[3]);
	UNALIGNED_PUT(0, &addr->s6_addr16[4]);
	UNALIGNED_PUT(0, &addr->s6_addr16[5]);
	UNALIGNED_PUT(0, &addr->s6_addr16[6]);
	addr->s6_addr[14]  = 0;
	addr->s6_addr[15]  = 0x1a;

	return addr;
}

/**
 * @brief Return information whether the DAG is in use right now.
 *
 * @param dag Pointer to DAG.
 *
 * @return true if in use, false otherwise
 */
static inline bool net_rpl_dag_is_used(struct net_rpl_dag *dag)
{
	NET_ASSERT(dag);

	return !!dag->is_used;
}

/**
 * @brief Set the DAG as not in use.
 *
 * @param dag Pointer to DAG.
 */
static inline void net_rpl_dag_set_not_used(struct net_rpl_dag *dag)
{
	NET_ASSERT(dag);

	dag->is_used = 0;
}

/**
 * @brief Set the DAG as in use.
 *
 * @param dag Pointer to DAG.
 */
static inline void net_rpl_dag_set_used(struct net_rpl_dag *dag)
{
	NET_ASSERT(dag);

	dag->is_used = 1;
}

/**
 * @brief Return information whether the DAG is grounded or floating.
 *
 * @param dag Pointer to DAG.
 *
 * @return true if grounded, false if floating
 */
static inline bool net_rpl_dag_is_grounded(struct net_rpl_dag *dag)
{
	NET_ASSERT(dag);

	return !!dag->is_grounded;
}

/**
 * @brief Set DAG information whether it is grounded or floating.
 *
 * @param dag Pointer to DAG.
 */
static inline void net_rpl_dag_set_grounded_status(struct net_rpl_dag *dag,
						   bool grounded)
{
	NET_ASSERT(dag);

	dag->is_grounded = grounded;
}

/**
 * @brief Return information whether the DAG is joined or not.
 *
 * @param dag Pointer to DAG.
 *
 * @return true if joined, false otherwise
 */
static inline bool net_rpl_dag_is_joined(struct net_rpl_dag *dag)
{
	NET_ASSERT(dag);

	return !!dag->is_joined;
}

/**
 * @brief Mark DAG as joined.
 *
 * @param dag Pointer to DAG.
 */
static inline void net_rpl_dag_join(struct net_rpl_dag *dag)
{
	NET_ASSERT(dag);

	dag->is_joined = 1;
}

/**
 * @brief Mark DAG as joined.
 *
 * @param dag Pointer to DAG.
 */
static inline void net_rpl_dag_unjoin(struct net_rpl_dag *dag)
{
	NET_ASSERT(dag);

	dag->is_joined = 0;
}

/**
 * @brief Get preference for this DAG.
 *
 * @details This function returns the preference of the dag.
 *
 * @param dag Pointer to DAG.
 *
 * @return preference value
 */
static inline
u8_t net_rpl_dag_get_preference(struct net_rpl_dag *dag)
{
	NET_ASSERT(dag);

	return dag->preference;
}

/**
 * @brief Set preference for this DAG.
 *
 * @details This function sets the preference of the dag.
 *
 * @param dag Pointer to DAG.
 * @param preference New preference value.
 */
static inline
void net_rpl_dag_set_preference(struct net_rpl_dag *dag,
				u8_t preference)
{
	NET_ASSERT(dag && preference <= 8);

	dag->preference = preference;
}

/**
 * @typedef net_rpl_join_callback_t
 * @brief Callback for evaluating if this is a network to join or not.
 * @param dio Pointer to DIO.
 * @return True if the network is to be joined, false otherwise.
 */
typedef bool (*net_rpl_join_callback_t)(struct net_rpl_dio *dio);

/**
 * @brief Register a callback that determines if the network is to be
 * joined or not.
 *
 * @param cb Callback function
 */
void net_rpl_set_join_callback(net_rpl_join_callback_t cb);

/**
 * @brief Send a DODAG Information Solicitation message.
 *
 * @param dst Destination IPv6 address.
 * @param iface Interface to send the message to.
 *
 * @return 0 message was sent ok, <0 otherwise
 */
int net_rpl_dis_send(struct in6_addr *dst, struct net_if *iface);

/**
 * @brief Send a Destination Advertisement Object message.
 *
 * @param iface Network interface to use, this can be set to NULL in which
 * case the function can try to figure it out itself.
 * @param parent Pointer to parent information.
 * @param prefix IPv6 address
 * @param lifetime Lifetime of the advertisement.
 *
 * @return 0 message was sent ok, <0 otherwise
 */
int net_rpl_dao_send(struct net_if *iface,
		     struct net_rpl_parent *parent,
		     struct in6_addr *prefix,
		     u8_t lifetime);

/**
 * @brief Send a DODAG Information Object message.
 *
 * @param iface Network interface to use, this can be set to NULL in which
 * case the function can try to figure it out itself.
 * @param instance Pointer to instance object.
 * @param src IPv6 source address.
 * @param dst IPv6 destination address.
 *
 * @return 0 message was sent ok, <0 otherwise
 */
int net_rpl_dio_send(struct net_if *iface,
		     struct net_rpl_instance *instance,
		     struct in6_addr *src,
		     struct in6_addr *dst);

/**
 * @brief Set the root DAG.
 *
 * @param iface Network interface to use.
 * @param instance Pointer to instance object.
 * @param dag_id IPv6 address of the DAG.
 *
 * @return DAG object or NULL if creation failed.
 */
struct net_rpl_dag *net_rpl_set_root(struct net_if *iface,
				     u8_t instance_id,
				     struct in6_addr *dag_id);

/**
 * @brief Set the root DAG with version.
 *
 * @param iface Network interface to use.
 * @param instance Pointer to instance object.
 * @param dag_id IPv6 address of the DAG.
 * @param version Version number to use.
 *
 * @return DAG object or NULL if creation failed.
 */
struct net_rpl_dag *net_rpl_set_root_with_version(struct net_if *iface,
						  u8_t instance_id,
						  struct in6_addr *dag_id,
						  u8_t version);

/**
 * @brief Get first available DAG.
 *
 * @return First available DAG or NULL if none found.
 */
struct net_rpl_dag *net_rpl_get_any_dag(void);

/**
 * @brief Set IPv6 prefix we are using.
 *
 * @param iface Network interface in use.
 * @param dag DAG in use.
 * @param prefix IPv6 prefix we are using.
 * @param prefix_len IPv6 prefix length.
 *
 * @return True if prefix could be set, false otherwise.
 */
bool net_rpl_set_prefix(struct net_if *iface,
			struct net_rpl_dag *dag,
			struct in6_addr *prefix,
			u8_t prefix_len);

/**
 * @brief Do global repair for this route.
 *
 * @param route IPv6 route entry.
 */
void net_rpl_global_repair(struct net_route_entry *route);

/**
 * @brief Do global repair for this instance.
 *
 * @param instance RPL instance id.
 */
bool net_rpl_repair_root(u8_t instance_id);

/**
 * @brief Update RPL headers in IPv6 packet.
 *
 * @param pkt Network packet.
 * @param addr IPv6 address of next hop host.
 *
 * @return 0 if ok, < 0 if error
 */
int net_rpl_update_header(struct net_pkt *pkt, struct in6_addr *addr);

/**
 * @brief Verify RPL header in IPv6 packet.
 *
 * @param pkt Network packet fragment list.
 * @param frag Current network buffer fragment.
 * @param offset Where the RPL header starts in the packet
 * @param pos How long the RPL header was, this is returned to the caller.
 * @param out result True if ok, false if error
 *
 * @return frag Returns the fragment where this call finished reading.
 */
struct net_buf *net_rpl_verify_header(struct net_pkt *pkt, struct net_buf *frag,
				      u16_t offset, u16_t *pos,
				      bool *result);

/**
 * @brief Insert RPL extension header to IPv6 packet.
 *
 * @param pkt Network packet.
 *
 * @return 0 if ok, <0 if error.
 */
int net_rpl_insert_header(struct net_pkt *pkt);

/**
 * @brief Revert RPL extension header to IPv6 packet.
 *        Revert flags, instance ID and sender rank in the packet.
 *
 * @param pkt Network packet.
 * @param offset Where the HBH header starts in the packet
 * @param pos How long the RPL header was, this is returned to the caller.
 *
 * @return 0 if ok, <0 if error.
 */
int net_rpl_revert_header(struct net_pkt *pkt, u16_t offset, u16_t *pos);

/**
 * @brief Get parent IPv6 address.
 *
 * @param iface Network interface
 * @param parent Parent pointer
 *
 * @return Parent IPv6 address, or NULL if no such parent.
 */
struct in6_addr *net_rpl_get_parent_addr(struct net_if *iface,
					 struct net_rpl_parent *parent);

typedef void (*net_rpl_parent_cb_t)(struct net_rpl_parent *parent,
				    void *user_data);

/**
 * @brief Go through all the parents entries and call callback
 * for each entry that is in use.
 *
 * @param cb User supplied callback function to call.
 * @param user_data User specified data.
 *
 * @return Total number of parents found.
 */
int net_rpl_foreach_parent(net_rpl_parent_cb_t cb, void *user_data);

/**
 * @brief Set the RPL mode (mesh or leaf) of this node.
 *
 * @param new_mode New RPL mode. Value is either NET_RPL_MODE_MESH,
 * NET_RPL_MODE_FEATHER or NET_RPL_MODE_LEAF. The NET_RPL_MODE_MESH is
 * the default mode.
 */
void net_rpl_set_mode(enum net_rpl_mode new_mode);

/**
 * @brief Get the RPL mode (mesh or leaf) of this node.
 *
 * @return Current RPL mode.
 */
enum net_rpl_mode net_rpl_get_mode(void);

/**
 * @brief Get the default RPL instance.
 *
 * @return Current default RPL instance.
 */
struct net_rpl_instance *net_rpl_get_default_instance(void);

void net_rpl_init(void);
#else
#define net_rpl_init(...)
#define net_rpl_global_repair(...)
#define net_rpl_update_header(...) 0
#endif /* CONFIG_NET_RPL */

#ifdef __cplusplus
}
#endif

#endif /* __RPL_H */
