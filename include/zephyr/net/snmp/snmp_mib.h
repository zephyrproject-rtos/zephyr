/*
 * Copyright (c) 2026 StructCore Embedded
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NET_SNMP_SNMP_MIB_H_
#define ZEPHYR_INCLUDE_NET_SNMP_SNMP_MIB_H_

#include <zephyr/kernel.h>
#include <zephyr/net/snmp/snmp_ber.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup snmp_mib MIB Tree Engine & Two-Phase Commit (2PC)
 * @ingroup snmp
 * @{
 * @brief MIB tree storage, O(log N) lookup, table walk, and transactional SET engine.
 */

#ifndef CONFIG_SNMP_MAX_MIB_NODES
#define CONFIG_SNMP_MAX_MIB_NODES 128
#endif

#ifndef CONFIG_SNMP_MAX_TABLES
#define CONFIG_SNMP_MAX_TABLES 8
#endif

struct snmp_mib_node;
struct snmp_agent;

/**
 * @brief Two-Phase Commit (2PC) SET transaction action phases per RFC 3416 §4.2.5.
 */
enum snmp_set_action {
	SNMP_SET_RESERVE, /**< Phase 1: validate types, values, and reserve resources */
	SNMP_SET_COMMIT,  /**< Phase 2: apply modifications */
	SNMP_SET_UNDO,    /**< Phase 3: rollback on partial failure */
	SNMP_SET_CLEANUP  /**< Phase 4: release transaction resources */
};

/* RFC 3416 Protocol Error Status Codes */
#define SNMP_ERR_NO_ERROR             0
#define SNMP_ERR_TOO_BIG              1
#define SNMP_ERR_NO_SUCH_NAME         2
#define SNMP_ERR_BAD_VALUE            3
#define SNMP_ERR_READ_ONLY            4
#define SNMP_ERR_GEN_ERR              5
#define SNMP_ERR_NO_ACCESS            6
#define SNMP_ERR_WRONG_TYPE           7
#define SNMP_ERR_WRONG_LENGTH         8
#define SNMP_ERR_WRONG_ENCODING       9
#define SNMP_ERR_WRONG_VALUE          10
#define SNMP_ERR_NO_CREATION          11
#define SNMP_ERR_INCONSISTENT_VALUE   12
#define SNMP_ERR_RESOURCE_UNAVAILABLE 13
#define SNMP_ERR_COMMIT_FAILED        14
#define SNMP_ERR_UNDO_FAILED          15
#define SNMP_ERR_AUTHORIZATION_ERR    16
#define SNMP_ERR_NOT_WRITABLE         17
#define SNMP_ERR_INCONSISTENT_NAME    18

/**
 * @brief Read callback for a scalar MIB node.
 *
 * @param agent The instance that received the request. The agent is always the object
 *              owning @p node's registration, so per-agent state (identity strings,
 *              counters, scratch buffers) is reached through it.
 * @param node  The node being read.
 * @param vb    Variable binding to fill in.
 * @return 0 on success, negative errno or a protocol error number otherwise.
 */
typedef int (*snmp_mib_get_cb_t)(struct snmp_agent *agent, const struct snmp_mib_node *node,
				 struct snmp_varbind *vb);

/** @brief Write callback for a scalar MIB node. See snmp_mib_get_cb_t for @p agent. */
typedef int (*snmp_mib_set_cb_t)(struct snmp_agent *agent, struct snmp_mib_node *node,
				 const struct snmp_varbind *vb);

/**
 * @brief Two-phase-commit write callback for a scalar MIB node (RFC 3416 section 4.2.5).
 * @param agent   Instance that received the SET.
 * @param node    Node being written.
 * @param vb      Proposed value.
 * @param action  Phase of the transaction.
 * @param tx_ctx  Per-transaction context, private to the callback.
 */
typedef int (*snmp_mib_set_tx_cb_t)(struct snmp_agent *agent, struct snmp_mib_node *node,
				    const struct snmp_varbind *vb, enum snmp_set_action action,
				    void **tx_ctx);

struct snmp_mib_node {
	struct snmp_oid oid;
	uint8_t type;
	snmp_mib_get_cb_t get_cb;
	snmp_mib_set_cb_t set_cb;
	snmp_mib_set_tx_cb_t set_tx_cb;
	/**
	 * @brief Application context handed back to the callbacks.
	 *
	 * MUST point only at application-owned state (sensor records, device structs) or
	 * at compile-time constants. It must NEVER point at a @ref snmp_agent or at a field
	 * inside one: the owning agent is delivered as the first callback argument, and
	 * instance recovery through the node would defeat the multi-instance model.
	 */
	void *user_data;
};

/* --- Global SNMP MIB-II Group (1.3.6.1.2.1.11) Counters --- */
struct snmp_mib2_counters {
	uint32_t snmpInPkts;              /* 11.1.0 */
	uint32_t snmpOutPkts;             /* 11.2.0 */
	uint32_t snmpInBadVersions;       /* 11.3.0 */
	uint32_t snmpInBadCommunityNames; /* 11.4.0 */
	uint32_t snmpInBadCommunityUses;  /* 11.5.0 */
	uint32_t snmpInASNParseErrs;      /* 11.6.0 */
	uint32_t snmpInTooBigs;           /* 11.8.0 */
	uint32_t snmpInNoSuchNames;       /* 11.9.0 */
	uint32_t snmpInBadValues;         /* 11.10.0 */
	uint32_t snmpInReadOnlys;         /* 11.11.0 */
	uint32_t snmpInGenErrs;           /* 11.12.0 */
	uint32_t snmpInTotalReqVars;      /* 11.13.0 */
	uint32_t snmpInTotalSetVars;      /* 11.14.0 */
	uint32_t snmpInGetRequests;       /* 11.15.0 */
	uint32_t snmpInGetNexts;          /* 11.16.0 */
	uint32_t snmpInSetRequests;       /* 11.17.0 */
	uint32_t snmpInGetResponses;      /* 11.18.0 */
	uint32_t snmpInTraps;             /* 11.19.0 */
	uint32_t snmpOutTooBigs;          /* 11.20.0 */
	uint32_t snmpOutNoSuchNames;      /* 11.21.0 */
	uint32_t snmpOutBadValues;        /* 11.22.0 */
	uint32_t snmpOutGenErrs;          /* 11.24.0 */
	uint32_t snmpOutGetRequests;      /* 11.25.0 */
	uint32_t snmpOutGetNexts;         /* 11.26.0 */
	uint32_t snmpOutSetRequests;      /* 11.27.0 */
	uint32_t snmpOutGetResponses;     /* 11.28.0 */
	uint32_t snmpOutTraps;            /* 11.29.0 */
	int32_t snmpEnableAuthenTraps;    /* 11.30.0 (1=enabled, 2=disabled) */
	uint32_t snmpSilentDrops;         /* 11.31.0 */
	uint32_t snmpProxyDrops;          /* 11.32.0 */
};

/**
 * @brief Snapshot an agent's MIB-II snmp-group counters.
 *
 * Safe to call from any context (plain/atomic word reads) and independent of the
 * 1.3.6.1.2.1.11 MIB nodes, which read the same fields of the receiving agent.
 *
 * @param agent Agent to query; NULL is ignored.
 * @param out   Destination structure.
 */
void snmp_agent_get_counters(const struct snmp_agent *agent, struct snmp_mib2_counters *out);

/**
 * @brief Zero an agent's counters, restoring snmpEnableAuthenTraps to the RFC 1907
 *        default (enabled).
 *
 * @param agent Agent to clear; NULL is ignored.
 */
void snmp_agent_zero_counters(struct snmp_agent *agent);

static inline void snmp_counter_inc(uint32_t *counter)
{
#if defined(__ZEPHYR__)
	__atomic_fetch_add(counter, 1U, __ATOMIC_RELAXED);
#else
	(*counter)++;
#endif
}

static inline void snmp_counter_add(uint32_t *counter, uint32_t value)
{
#if defined(__ZEPHYR__)
	__atomic_fetch_add(counter, value, __ATOMIC_RELAXED);
#else
	*counter += value;
#endif
}

/* --- MIB Table Support --- */

struct snmp_table_index {
	uint8_t len;
	uint32_t subids[CONFIG_SNMP_MAX_OID_LEN];
};

struct snmp_table_row;

/**
 * @brief Read callback for one table cell.
 * @param agent  Instance that received the request.
 * @param row    Row holding the cell.
 * @param column Column index into struct snmp_mib_table.
 * @param vb     Variable binding to fill in.
 */
typedef int (*snmp_table_get_cb_t)(struct snmp_agent *agent, const struct snmp_table_row *row,
				   uint8_t column, struct snmp_varbind *vb);

/** @brief Write callback for one table cell. See snmp_table_get_cb_t. */
typedef int (*snmp_table_set_cb_t)(struct snmp_agent *agent, struct snmp_table_row *row,
				   uint8_t column, const struct snmp_varbind *vb);

struct snmp_table_row {
	uint32_t index;                    /* Legacy single index (backward compatible) */
	struct snmp_table_index index_oid; /* Generalized / Compound index suffix */
	struct snmp_table_row *next;
	/**
	 * @brief Application-owned row context.
	 *
	 * Same rule as struct snmp_mib_node::user_data: never a @ref snmp_agent or a
	 * pointer into one.
	 */
	void *user_data;
};

struct snmp_mib_table {
	struct snmp_oid table_oid;
	uint8_t column_cnt;
	const uint8_t *column_subids;
	const uint8_t *column_types;
	snmp_table_get_cb_t *get_cbs;
	snmp_table_set_cb_t *set_cbs;
	struct snmp_table_row *rows;
	void *user_data;
};

/**
 * @brief Internal transaction item tracking individual variable state during Two-Phase Commit (2PC)
 * SET.
 *
 * Encapsulates the target MIB node, table, column, backup of previous value for rollback,
 * and reservation status during atomic multi-varbind SET operations.
 */
struct snmp_set_tx_item {
	struct snmp_mib_node *node;     /**< Target scalar node (if !is_table). */
	struct snmp_mib_table *tbl;     /**< Target table descriptor (if is_table). */
	struct snmp_table_row *tbl_row; /**< Target table row (if is_table). */
	uint8_t tbl_col;                /**< Target column index within the table. */
	bool is_table;   /**< True if the item targets a table column rather than a scalar. */
	bool reserved;   /**< True if Phase 1 (Test/Reserve) succeeded for this item. */
	bool has_old_vb; /**< True if old_vb contains a valid backup for rollback. */
	struct snmp_varbind
		old_vb; /**< Backup of prior value used during Phase 2 rollback on failure. */
	void *tx_ctx;   /**< Opaque application transaction context. */
};

/**
 * @brief Prepend a row to an application-owned table's row list.
 *
 * The row list is application state; registration only stores the table pointer in
 * the agent. The update is serialised against @p inst's walks with the agent lock,
 * so pass the agent this table is registered with (a table shared by several agents
 * must be mutated per that contract's snmp_mib_lock()/unlock() discipline).
 *
 * @param inst Agent whose MIB walks must not observe a torn row list.
 * @param table Application-owned table descriptor.
 * @param row Application-owned row; its @c next link is managed by this API.
 * @return 0 on success, -EINVAL on bad arguments.
 */
int snmp_mib_table_add_row(struct snmp_agent *inst, struct snmp_mib_table *table,
			   struct snmp_table_row *row);

/**
 * @brief Unlink the row with @p index from an application-owned table's row list.
 * @param inst Agent whose MIB walks must not observe a torn row list.
 * @param table Application-owned table descriptor.
 * @param index Row index to remove.
 * @return 0 on success, -EINVAL on bad arguments, -ENOENT when no row matches.
 */
int snmp_mib_table_remove_row(struct snmp_agent *inst, struct snmp_mib_table *table,
			      uint32_t index);

/**
 * @brief Unlink the row with @p idx from an application-owned table's row list.
 * @param inst Agent whose MIB walks must not observe a torn row list.
 * @param table Application-owned table descriptor.
 * @param idx Generalized row index suffix to remove.
 * @return 0 on success, -EINVAL on bad arguments, -ENOENT when no row matches.
 */
int snmp_mib_table_remove_row_by_index(struct snmp_agent *inst, struct snmp_mib_table *table,
				       const struct snmp_table_index *idx);

/* --- Instance-based MIB APIs --- */

/**
 * @brief Reset the agent's MIB tree and table registry.
 * @param inst Target agent instance.
 * @return 0 on success, -EINVAL if @p inst is NULL.
 */
int snmp_mib_init(struct snmp_agent *inst);

/**
 * @brief Register the MIB-II system group and snmp (1.3.6.1.2.1.11) counter group
 *        into the agent's own tree.
 * @param inst Target agent instance.
 * @return 0 on success, -EINVAL if @p inst is NULL, -ENOMEM if the tree is full.
 */
int snmp_mib_register_defaults(struct snmp_agent *inst);

/**
 * @brief Register a single MIB node in the agent's tree (lexicographically sorted).
 * @param inst Target agent instance.
 * @param node Node definition to copy in.
 * @return 0 on success, -EINVAL on bad arguments, -ENOMEM if the tree is full.
 */
int snmp_mib_register(struct snmp_agent *inst, const struct snmp_mib_node *node);

/**
 * @brief Register an array of MIB nodes in bulk.
 * @param inst Target agent instance.
 * @param nodes Node array.
 * @param count Number of nodes.
 * @return 0 on success, -EINVAL on bad arguments, -ENOMEM if the tree is full.
 */
int snmp_mib_register_nodes(struct snmp_agent *inst, const struct snmp_mib_node *nodes,
			    size_t count);

/**
 * @brief Look up one OID in the agent's tree (scalar node or table cell).
 * @param inst Target agent instance.
 * @param oid OID to resolve.
 * @param vb Output variable binding.
 * @return 0 on success, -EINVAL on bad arguments, negative errno from the node callback.
 */
int snmp_mib_get(struct snmp_agent *inst, const struct snmp_oid *oid, struct snmp_varbind *vb);

/**
 * @brief Return the lexicographically next OID present in the agent's tree.
 * @param inst Target agent instance.
 * @param oid Reference OID.
 * @param vb Output variable binding (endOfMibView when nothing is larger).
 * @return 0 on success, -EINVAL on bad arguments.
 */
int snmp_mib_get_next(struct snmp_agent *inst, const struct snmp_oid *oid, struct snmp_varbind *vb);

/**
 * @brief Write one variable binding (reserve/commit/cleanup runs inline).
 * @param inst Target agent instance.
 * @param oid Target OID.
 * @param vb Value to write.
 * @return 0 on success, -EINVAL on bad arguments, -ENOENT/-EACCES per RFC 3416.
 */
int snmp_mib_set(struct snmp_agent *inst, const struct snmp_oid *oid,
		 const struct snmp_varbind *vb);

/**
 * @brief Execute a PDU (GET/GETNEXT/GETBULK/SET) against the agent's tree.
 *
 * Runs the two-phase-commit SET pipeline of RFC 3416 section 4.2.5 atomically under
 * the agent lock. @p inst is mandatory: pass the agent that received the message.
 *
 * @param inst Target agent instance.
 * @param pdu PDU to process in place (error_status/error_index and response type set).
 * @return 0 on success, -EINVAL if @p inst or @p pdu is NULL.
 */
int snmp_mib_dispatch(struct snmp_agent *inst, struct snmp_pdu *pdu);

/**
 * @brief Register an SNMP table in the agent's tree.
 * @param inst Target agent instance.
 * @param table Application-owned table descriptor (pointer is stored, not copied).
 * @return 0 on success, -EINVAL on bad arguments, -ENOMEM if the table registry is full.
 */
int snmp_mib_register_table(struct snmp_agent *inst, struct snmp_mib_table *table);

/**
 * @brief Lock the agent's shared state (MIB tree, tables, sys strings, communities,
 *        engine config and rx/tx/current_pdu scratch). Same mutex as @c inst->lock.
 * @param inst Target agent instance.
 */
void snmp_mib_lock(struct snmp_agent *inst);

/** @brief Release the lock taken by snmp_mib_lock(). @param inst Target agent. */
void snmp_mib_unlock(struct snmp_agent *inst);

int snmp_set_device_enterprise_oid(struct snmp_agent *inst, const struct snmp_oid *oid);
int snmp_mib2_set_sysdescr(struct snmp_agent *inst, const char *descr);
int snmp_mib2_set_syscontact(struct snmp_agent *inst, const char *contact);
int snmp_mib2_set_sysname(struct snmp_agent *inst, const char *name);
int snmp_mib2_set_syslocation(struct snmp_agent *inst, const char *location);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_NET_SNMP_SNMP_MIB_H_ */
