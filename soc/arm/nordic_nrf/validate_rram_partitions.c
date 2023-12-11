/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/toolchain.h>

#define PAIR__(f, sep, arg_first, ...) FOR_EACH_FIXED_ARG(f, sep, arg_first, __VA_ARGS__)
#define PAIR_(f, sep, args_to_expand)  PAIR__(f, sep, args_to_expand)
#define PAIR(n, f, sep, ...)	       PAIR_(f, sep, GET_ARGS_LESS_N(n, __VA_ARGS__))

/**
 * @brief Call a macro on every unique pair of the given variadic arguments.
 *
 * For example, FOR_EACH_PAIR(f, (,), 1, 2, 3, 4) should expand to:
 *
 *     f(2, 1) , f(3, 1) , f(4, 1) , f(3, 2) , f(4, 2) , f(4, 3)
 *
 * @param f   Macro to call. Must accept two arguments.
 * @param sep Separator between macro calls. Must be in parentheses.
 *
 * @see FOR_EACH
 */
#define FOR_EACH_PAIR(f, sep, ...)                                                                 \
	LISTIFY(NUM_VA_ARGS_LESS_1(__VA_ARGS__), PAIR, sep, f, sep, __VA_ARGS__)

/**
 * @brief Get a node's non-secure register block start address.
 *
 * @param node_id Node identifier.
 */
#define REG_ADDR_NS(node_id) (DT_REG_ADDR(node_id) & 0xEFFFFFFFUL)

/**
 * @brief Get a node's non-secure register block end address.
 *
 * @param node_id Node identifier.
 */
#define REG_END_NS(node_id) (REG_ADDR_NS(node_id) + DT_REG_SIZE(node_id))

/* clang-format off */

#define RRAM_BASE REG_ADDR_NS(DT_NODELABEL(rram0))
#define RRAM_CONTROLLER DT_NODELABEL(rram_controller)

#if !DT_NODE_EXISTS(RRAM_CONTROLLER)
#error "Missing \"rram-controller\" node"
#endif

#define CHECK_RRAM_NODE_COMPATIBLE(node_id)                                         \
	BUILD_ASSERT(DT_NODE_HAS_COMPAT(node_id, soc_nv_flash),                         \
			"Missing compatible \"soc-nv-flash\" from " DT_NODE_FULL_NAME(node_id)  \
			" (required for all children of " DT_NODE_PATH(RRAM_CONTROLLER) ")")

#define CHECK_RRAM_PARTITION_WITHIN_PARENT(node_id)                                            \
	BUILD_ASSERT(RRAM_BASE + REG_ADDR_NS(node_id) >= REG_ADDR_NS(DT_GPARENT(node_id)) && \
			RRAM_BASE + REG_END_NS(node_id) <= REG_END_NS(DT_GPARENT(node_id)),      \
			DT_NODE_FULL_NAME(node_id) " is not fully contained within its parent "  \
			DT_NODE_PATH(DT_GPARENT(node_id)))

#define CHECK_NODES_NON_OVERLAPPING(node_id_1, node_id_2)                                \
	BUILD_ASSERT(REG_ADDR_NS(node_id_1) >= REG_END_NS(node_id_2) ||                \
			REG_ADDR_NS(node_id_2) >= REG_END_NS(node_id_1),                       \
			DT_NODE_PATH(node_id_1) " and " DT_NODE_PATH(node_id_2) " are overlapping")

/* Retrieve all RRAM nodes that are children of "rram-controller". */
#define COMMA(x) x,
#define RRAM_NODES_LIST LIST_DROP_EMPTY(DT_FOREACH_CHILD(RRAM_CONTROLLER, COMMA))

#if !IS_EMPTY(RRAM_NODES_LIST)

/* Check that every RRAM node matches the "soc-nv-flash" compatible. */
FOR_EACH(CHECK_RRAM_NODE_COMPATIBLE, (;), RRAM_NODES_LIST);

/* Check that no two RRAM nodes are overlapping. */
FOR_EACH_PAIR(CHECK_NODES_NON_OVERLAPPING, (;), RRAM_NODES_LIST);

#endif

/* Retrieve all RRAM partitions by looking for "fixed-partitions" compatibles in each RRAM node. */
#define PARTITION_(x)                                                                              \
	COND_CODE_1(DT_NODE_HAS_COMPAT(x, fixed_partitions), (DT_FOREACH_CHILD(x, COMMA)), ())
#define PARTITION(x, ...) DT_FOREACH_CHILD_STATUS_OKAY(x, PARTITION_)
#define RRAM_PARTITION_LIST LIST_DROP_EMPTY(DT_FOREACH_CHILD_VARGS(RRAM_CONTROLLER, PARTITION))

#if !IS_EMPTY(RRAM_PARTITION_LIST)

/* Check that every RRAM partition is within the bounds of its parent RRAM node. */
FOR_EACH(CHECK_RRAM_PARTITION_WITHIN_PARENT, (;), RRAM_PARTITION_LIST);

/* Check that no two RRAM partitions are overlapping. */
FOR_EACH_PAIR(CHECK_NODES_NON_OVERLAPPING, (;), RRAM_PARTITION_LIST);

#endif
