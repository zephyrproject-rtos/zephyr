/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief SNMP (Simple Network Management Protocol) agent
 */

#ifndef ZEPHYR_INCLUDE_NET_SNMP_H_
#define ZEPHYR_INCLUDE_NET_SNMP_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/sys/iterable_sections.h>
#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief SNMP agent API
 * @defgroup snmp SNMP agent
 * @since 4.5
 * @version 0.1.0
 * @ingroup networking
 * @{
 */

/** Maximum number of sub-identifiers in an object identifier. */
#define SNMP_OID_MAX_LEN CONFIG_SNMP_OID_MAX_LEN

/** ASN.1 types a managed object can have. */
enum snmp_type {
	/** INTEGER, 32-bit signed. */
	SNMP_TYPE_INTEGER,
	/** OCTET STRING, arbitrary bytes. */
	SNMP_TYPE_OCTET_STRING,
	/** NULL, carries no value. Used in requests only. */
	SNMP_TYPE_NULL,
	/** OBJECT IDENTIFIER. */
	SNMP_TYPE_OID,
	/** IpAddress, four bytes in network order. */
	SNMP_TYPE_IPADDRESS,
	/** Counter32, monotonically increasing and wrapping. */
	SNMP_TYPE_COUNTER32,
	/** Gauge32, also used for Unsigned32. */
	SNMP_TYPE_GAUGE32,
	/** TimeTicks, hundredths of a second. */
	SNMP_TYPE_TIMETICKS,
	/** Counter64, SNMPv2c only. */
	SNMP_TYPE_COUNTER64,
};

/** Value of a managed object instance. */
struct snmp_value {
	/** Type that selects the union member. */
	enum snmp_type type;
	/** Value, in the member that @ref type selects. */
	union {
		/** @ref SNMP_TYPE_INTEGER */
		int32_t i32;
		/** @ref SNMP_TYPE_COUNTER32, @ref SNMP_TYPE_GAUGE32, @ref SNMP_TYPE_TIMETICKS */
		uint32_t u32;
		/** @ref SNMP_TYPE_COUNTER64 */
		uint64_t u64;
		/** @ref SNMP_TYPE_OCTET_STRING and @ref SNMP_TYPE_IPADDRESS */
		struct {
			/** Bytes of the string. Not NUL terminated. */
			const uint8_t *data;
			/** Number of bytes. */
			size_t len;
		} octets;
		/** @ref SNMP_TYPE_OID */
		struct {
			/** Sub-identifiers. */
			const uint32_t *arcs;
			/** Number of sub-identifiers. */
			size_t len;
		} oid;
	};
};

/** Phase of a SET request in which a setter is invoked. */
enum snmp_set_phase {
	/** Validate the value without changing anything. */
	SNMP_SET_CHECK,
	/** Apply a value that passed the check phase. */
	SNMP_SET_COMMIT,
};

/**
 * @brief Callback invoked once per existing table row.
 *
 * Called by a table's @ref snmp_table.foreach_row implementation. The row
 * handle is opaque to the agent and is only passed back to the same table's
 * callbacks.
 *
 * @param row Opaque handle of the row, as understood by the owning table.
 * @param user_data Pointer given to @ref snmp_table.foreach_row.
 */
typedef void (*snmp_row_cb_t)(void *row, void *user_data);

/**
 * @brief Row provider shared by all columns of one conceptual table.
 *
 * A table does not store rows. Each callback derives them on demand from the
 * data structure that owns them, for example the network interface list. The
 * agent walks the rows again on every request, so a row that appears or
 * disappears between two PDUs cannot corrupt a walk. All callbacks are
 * invoked from the agent's context and must not block for long.
 */
struct snmp_table {
	/**
	 * @brief Invoke @p cb for every row that currently exists.
	 *
	 * The order of invocation is irrelevant; the agent selects the
	 * lexicographically next row itself. A row handle passed to @p cb
	 * must stay valid until the current PDU has been answered.
	 *
	 * @param tbl The table being enumerated.
	 * @param cb Callback to invoke per row.
	 * @param user_data Passed unchanged to @p cb.
	 */
	void (*foreach_row)(const struct snmp_table *tbl, snmp_row_cb_t cb, void *user_data);

	/**
	 * @brief Encode the INDEX of a row as OID sub-identifiers.
	 *
	 * Produces the arcs that follow the column OID in an instance OID,
	 * for example a single arc holding ifIndex for ifTable.
	 *
	 * @param tbl The table the row belongs to.
	 * @param row Row handle obtained from @ref foreach_row.
	 * @param arcs Output buffer for the sub-identifiers.
	 * @param max_arcs Capacity of @p arcs in sub-identifiers.
	 *
	 * @return Number of sub-identifiers written, greater than zero.
	 * @retval -EMSGSIZE @p max_arcs is too small for this table's index.
	 * @retval -ENOENT The row no longer exists.
	 */
	int (*row_index)(const struct snmp_table *tbl, void *row, uint32_t *arcs, size_t max_arcs);

	/**
	 * @brief Find the row identified by an exact INDEX.
	 *
	 * Used for GET and SET, where the instance OID names one row. The
	 * arcs are exactly the suffix following the column OID; a suffix
	 * with the wrong length or an unknown index yields no row.
	 *
	 * @param tbl The table to search.
	 * @param arcs Sub-identifiers following the column OID.
	 * @param len Number of sub-identifiers in @p arcs.
	 *
	 * @return Row handle, or NULL when no row has this index. NULL is
	 *         reported to the manager as noSuchInstance.
	 */
	void *(*row_lookup)(const struct snmp_table *tbl, const uint32_t *arcs, size_t len);
};

/**
 * @brief One entry of the MIB registry.
 *
 * Entries are placed in an iterable section by SNMP_SCALAR_DEFINE() and
 * SNMP_COLUMN_DEFINE(); the agent never allocates them. An entry is either a
 * scalar object, whose only instance is @c oid.0, or one column of a table,
 * whose instances are @c oid followed by a row index. The section is not
 * ordered; the agent scans all entries on every GET, GETNEXT and GETBULK.
 * Two entries with the same OID are a configuration error.
 */
struct snmp_object {
	/** Object identifier of the scalar or column, without instance suffix. */
	const uint32_t *oid;
	/** Number of sub-identifiers in @ref oid. */
	uint8_t oid_len;
	/** Row provider for a column, or NULL for a scalar. */
	const struct snmp_table *table;

	/**
	 * @brief Read the value of one instance.
	 *
	 * For a scalar @p row is NULL. For a column it is the handle returned
	 * by @ref snmp_table.row_lookup or @ref snmp_table.foreach_row.
	 * Any buffer referenced from @p val must stay valid until the callback
	 * that produced it returns to the agent and the agent has encoded it,
	 * which happens before any other registry callback is invoked.
	 *
	 * @param obj The registry entry being read.
	 * @param row Row handle, or NULL for a scalar.
	 * @param val Output value.
	 *
	 * @retval 0 Success.
	 * @retval -ENOENT No value for this instance, reported as noSuchInstance.
	 * @retval -EIO Value could not be retrieved, reported as genErr.
	 */
	int (*get)(const struct snmp_object *obj, void *row, struct snmp_value *val);

	/**
	 * @brief Write one instance, or NULL for a read-only object.
	 *
	 * A SET request is processed in two phases. First the callback is
	 * invoked with @ref SNMP_SET_CHECK for every binding of the request;
	 * it validates length and range and changes nothing. Only when every
	 * check succeeded is it invoked again with @ref SNMP_SET_COMMIT for
	 * each binding, in request order. The agent has already verified
	 * that the community grants write access and that the type matches
	 * the one reported by @ref get. A commit must not fail for a reason
	 * the check could have detected: nothing is rolled back, so a commit
	 * failure after another binding was committed is reported to the
	 * manager as undoFailed.
	 *
	 * @param obj The registry entry being written.
	 * @param row Row handle, or NULL for a scalar.
	 * @param val Value to store.
	 * @param phase @ref SNMP_SET_CHECK or @ref SNMP_SET_COMMIT.
	 *
	 * @retval 0 Success.
	 * @retval -EINVAL Wrong type for this object, reported as wrongType.
	 * @retval -EMSGSIZE Value too long, reported as wrongLength.
	 * @retval -ERANGE Value outside the permitted range, reported as wrongValue.
	 * @retval -EACCES Instance is read-only, reported as notWritable.
	 * @retval -EIO Commit failed, reported as commitFailed or undoFailed.
	 */
	int (*set)(const struct snmp_object *obj, void *row, const struct snmp_value *val,
		   enum snmp_set_phase phase);

	/** Opaque pointer for the implementer, not interpreted by the agent. */
	void *user_data;
};

/** @cond INTERNAL_HIDDEN */
#define Z_SNMP_OBJECT_DEFINE(_name, _table, _get, _set, _user_data, ...)                           \
	static const uint32_t _name##_oid[] = {__VA_ARGS__};                                       \
	const STRUCT_SECTION_ITERABLE(snmp_object, _name) = {                                      \
		.oid = _name##_oid,                                                                \
		.oid_len = ARRAY_SIZE(_name##_oid),                                                \
		.table = _table,                                                                   \
		.get = _get,                                                                       \
		.set = _set,                                                                       \
		.user_data = _user_data,                                                           \
	}
/** @endcond */

/**
 * @brief Define a scalar managed object.
 *
 * The object's only instance is the given OID followed by 0.
 *
 * @param _name C identifier of the registry entry, unique within the build.
 * @param _get Getter callback, see @ref snmp_object.get.
 * @param _set Setter callback, or NULL for a read-only object.
 * @param _user_data Value stored in @ref snmp_object.user_data.
 * @param ... Sub-identifiers of the object, for example 1, 3, 6, 1, 2, 1, 1, 1.
 */
#define SNMP_SCALAR_DEFINE(_name, _get, _set, _user_data, ...)                                     \
	Z_SNMP_OBJECT_DEFINE(_name, NULL, _get, _set, _user_data, __VA_ARGS__)

/**
 * @brief Define one column of a conceptual table.
 *
 * Instances are the given OID followed by the index of each row that the
 * table's row provider reports.
 *
 * @param _name C identifier of the registry entry, unique within the build.
 * @param _table The @ref snmp_table shared by all columns of this table.
 * @param _get Getter callback, see @ref snmp_object.get.
 * @param _set Setter callback, or NULL for a read-only column.
 * @param _user_data Value stored in @ref snmp_object.user_data.
 * @param ... Sub-identifiers of the column, for example 1, 3, 6, 1, 2, 1, 2, 2, 1, 6.
 */
#define SNMP_COLUMN_DEFINE(_name, _table, _get, _set, _user_data, ...)                             \
	Z_SNMP_OBJECT_DEFINE(_name, &(_table), _get, _set, _user_data, __VA_ARGS__)

/**
 * @brief Compare two object identifiers lexicographically.
 *
 * A proper prefix sorts before any of its extensions, as SNMP requires.
 *
 * @param a First identifier.
 * @param a_len Number of sub-identifiers in @p a.
 * @param b Second identifier.
 * @param b_len Number of sub-identifiers in @p b.
 *
 * @return Negative when @p a sorts before @p b, zero when equal, positive otherwise.
 */
int snmp_oid_cmp(const uint32_t *a, size_t a_len, const uint32_t *b, size_t b_len);

/**
 * @brief Check whether an object identifier starts with a prefix.
 *
 * @param oid Identifier to test.
 * @param oid_len Number of sub-identifiers in @p oid.
 * @param prefix Prefix to look for.
 * @param prefix_len Number of sub-identifiers in @p prefix.
 *
 * @return true when the first @p prefix_len sub-identifiers of @p oid equal @p prefix.
 */
bool snmp_oid_has_prefix(const uint32_t *oid, size_t oid_len, const uint32_t *prefix,
			 size_t prefix_len);

/** Selects one of the agent's community strings. */
enum snmp_community {
	/** Community that grants GET, GETNEXT and GETBULK. */
	SNMP_COMMUNITY_READ,
	/** Community that additionally grants SET. */
	SNMP_COMMUNITY_WRITE,
};

/**
 * @brief Set a community string.
 *
 * Replaces the default from CONFIG_SNMP_COMMUNITY_READ or
 * CONFIG_SNMP_COMMUNITY_WRITE. An empty string disables the operations that
 * community grants.
 *
 * @param which Community to change.
 * @param community NUL terminated string of at most CONFIG_SNMP_COMMUNITY_MAX_LEN bytes.
 *
 * @retval 0 Success.
 * @retval -EINVAL @p community is NULL or @p which is unknown.
 * @retval -EMSGSIZE @p community is too long.
 */
int snmp_agent_set_community(enum snmp_community which, const char *community);

/**
 * @brief Start the agent.
 *
 * Opens a UDP socket bound to CONFIG_SNMP_PORT on every enabled address
 * family. Requests are answered from the socket service thread.
 *
 * @retval 0 Success.
 * @retval -EALREADY The agent is already running.
 * @retval -errno A socket call failed; the reported errno.
 */
int snmp_agent_start(void);

/**
 * @brief Stop the agent and close its sockets.
 *
 * @retval 0 Success.
 * @retval -EALREADY The agent is not running.
 */
int snmp_agent_stop(void);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_NET_SNMP_H_ */
