/*
 * Copyright (c) 2026 Caleb Perkinson
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Devicetree C bindings for zephyr,j1939-node
 *
 * Provides compile-time accessors and initialiser macros for J1939 virtual
 * nodes declared in the devicetree as children of a CAN controller using the
 * "zephyr,j1939-node" compatible.
 *
 * Typical usage
 * -------------
 *
 * DTS overlay:
 * @code{.dts}
 *   &can0 {
 *       status = "okay";
 *
 *       j1939_node0: j1939-node@80 {
 *           compatible = "zephyr,j1939-node";
 *           reg = <0x80>;
 *           industry-group    = <2>;
 *           arbitrary-address-capable;
 *       };
 *   };
 * @endcode
 *
 * C code:
 * @code{.c}
 *   #include <zephyr/canbus/j1939_dt.h>
 *
 *   // Build a table of all enabled J1939 node configs at compile time.
 *   static const struct j1939_dt_node_cfg nodes[] = {
 *       J1939_DT_NODE_CFG_ALL_DEFINE
 *   };
 *
 *   // Total number of entries in the table.
 *   #define NUM_J1939_NODES  J1939_DT_NUM_NODES
 * @endcode
 */

#ifndef ZEPHYR_INCLUDE_CANBUS_J1939_DT_H_
#define ZEPHYR_INCLUDE_CANBUS_J1939_DT_H_



// TODO - definitely not a great way to do this but not sure where to put this now
typedef enum J1939Ac_State_E
{
   J1939Ac_State_WaitingStartupInit,
   J1939Ac_State_Start,
   J1939Ac_State_Waiting,
   J1939Ac_State_Claimed,
   J1939Ac_State_LostContention,
   J1939Ac_State_WaitingCannotClaim,
   J1939Ac_State_CannotClaim
} J1939Ac_State_T;

typedef struct J1939Ac_RecordBusInfo_S
{
   uint8_t source; // Source address associated with the name pointer below
#if defined(J1939_RECORD_ADDRESS_CLAIMED_NAMES)
   J1939_Name_T name;
#endif
} J1939Ac_RecordBusInfo_T;

// Structure definition for items which may be requested by another module on the bus.
typedef struct J1939_PgnRequest_S
{
   uint16_t pgn;              // PGN registered and requested
   uint8_t source; // Source address of registered or requested PGN
   bool isRequested;     // True if PGN has been requested
   bool isUsed;          // True if PGN is used
} J1939_PgnRequest_T;

typedef struct j1939_dt_node_cfg * J1939_Node_T;

/// Callback function pointer for transport sessions
typedef bool (*J1939Tp_Callback_T)(uint16_t pgn, uint8_t *data,
                                           uint32_t length, uint8_t sender,
                                           J1939_Node_T node);

typedef struct J1939Tp_PgnParams_S
{
   uint16_t pgn;
   J1939Tp_Callback_T callback;
} J1939Tp_PgnParams_T;

#include <zephyr/device.h>
#include <zephyr/devicetree.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup j1939_dt J1939 Devicetree bindings
 *  @{
 */

/* -------------------------------------------------------------------------
 * Node-identifier-based accessors  (node_id = DT_NODELABEL(...) etc.)
 * ---------------------------------------------------------------------- */

/**
 * @brief J1939 source address of a node (value of the @c reg property).
 * @param node_id Devicetree node identifier.
 */
#define J1939_DT_NODE_SOURCE_ADDRESS(node_id)   DT_REG_ADDR(node_id)

/**
 * @brief Pointer to the CAN controller device the node sits on.
 * @param node_id Devicetree node identifier.
 */
#define J1939_DT_NODE_CAN_DEV(node_id)          DEVICE_DT_GET(DT_BUS(node_id))

/**
 * @brief 21-bit NAME identity number.
 * @param node_id Devicetree node identifier.
 */
#define J1939_DT_NODE_ID_NUMBER(node_id)        DT_PROP(node_id, id_number)

/**
 * @brief 11-bit SAE manufacturer code.
 * @param node_id Devicetree node identifier.
 */
#define J1939_DT_NODE_MFG_CODE(node_id)         DT_PROP(node_id, manufacturer_code)

/**
 * @brief 3-bit ECU instance (0–7).
 * @param node_id Devicetree node identifier.
 */
#define J1939_DT_NODE_ECU_INSTANCE(node_id)     DT_PROP(node_id, ecu_instance)

/**
 * @brief 5-bit function instance (0–31).
 * @param node_id Devicetree node identifier.
 */
#define J1939_DT_NODE_FUNC_INSTANCE(node_id)    DT_PROP(node_id, function_instance)

/**
 * @brief 8-bit SAE function code.
 * @param node_id Devicetree node identifier.
 */
#define J1939_DT_NODE_FUNCTION(node_id)         DT_PROP(node_id, function)

/**
 * @brief 7-bit vehicle system code.
 * @param node_id Devicetree node identifier.
 */
#define J1939_DT_NODE_VEHICLE_SYSTEM(node_id)   DT_PROP(node_id, vehicle_system)

/**
 * @brief 4-bit vehicle system instance (0–15).
 * @param node_id Devicetree node identifier.
 */
#define J1939_DT_NODE_VS_INSTANCE(node_id)      DT_PROP(node_id, vehicle_system_instance)

/**
 * @brief 3-bit SAE industry group.
 * @param node_id Devicetree node identifier.
 */
#define J1939_DT_NODE_INDUSTRY_GROUP(node_id)   DT_PROP(node_id, industry_group)

/**
 * @brief 1 if the node has the @c arbitrary-address-capable property, 0 otherwise.
 * @param node_id Devicetree node identifier.
 */
#define J1939_DT_NODE_ARBITRARY_ADDR(node_id) \
	DT_PROP(node_id, arbitrary_address_capable)

/* -------------------------------------------------------------------------
 * Instance-number-based accessors  (inst = integer assigned by Zephyr DT)
 * These mirror the node-id macros but use DT_INST_* equivalents.
 * ---------------------------------------------------------------------- */

/** @brief Source address of instance @p inst. */
#define J1939_DT_INST_SOURCE_ADDRESS(inst)      DT_INST_REG_ADDR(inst)

/** @brief CAN device pointer for instance @p inst. */
#define J1939_DT_INST_CAN_DEV(inst)             DEVICE_DT_GET(DT_INST_BUS(inst))

/** @brief 21-bit NAME identity number for instance @p inst. */
#define J1939_DT_INST_ID_NUMBER(inst)           DT_INST_PROP(inst, id_number)

/** @brief Manufacturer code for instance @p inst. */
#define J1939_DT_INST_MFG_CODE(inst)            DT_INST_PROP(inst, manufacturer_code)

/** @brief ECU instance for instance @p inst. */
#define J1939_DT_INST_ECU_INSTANCE(inst)        DT_INST_PROP(inst, ecu_instance)

/** @brief Function instance for instance @p inst. */
#define J1939_DT_INST_FUNC_INSTANCE(inst)       DT_INST_PROP(inst, function_instance)

/** @brief Function code for instance @p inst. */
#define J1939_DT_INST_FUNCTION(inst)            DT_INST_PROP(inst, function)

/** @brief Vehicle system for instance @p inst. */
#define J1939_DT_INST_VEHICLE_SYSTEM(inst)      DT_INST_PROP(inst, vehicle_system)

/** @brief Vehicle system instance for instance @p inst. */
#define J1939_DT_INST_VS_INSTANCE(inst)         DT_INST_PROP(inst, vehicle_system_instance)

/** @brief Industry group for instance @p inst. */
#define J1939_DT_INST_INDUSTRY_GROUP(inst)      DT_INST_PROP(inst, industry_group)

/** @brief Arbitrary-address-capable flag for instance @p inst (0 or 1). */
#define J1939_DT_INST_ARBITRARY_ADDR(inst) \
	DT_INST_PROP(inst, arbitrary_address_capable)

/* -------------------------------------------------------------------------
 * Count
 * ---------------------------------------------------------------------- */

/**
 * @brief Number of enabled zephyr,j1939-node instances in the devicetree.
 */
#define J1939_DT_NUM_NODES \
	DT_NUM_INST_STATUS_OKAY(zephyr_j1939_node)

/* -------------------------------------------------------------------------
 * Config struct and table initialiser
 * ---------------------------------------------------------------------- */

/**
 * @brief Compile-time configuration for one J1939 virtual node.
 *
 * Populated entirely from devicetree; no runtime init required.
 */
struct j1939_dt_node_cfg {
	/** Pointer to the underlying CAN controller device. */
	const struct device *can_dev;

	/** J1939 source address (0x00–0xFD). */
	uint8_t source_address;

	/** J1939 NAME: 21-bit identity number. */
	uint32_t id_number;

	/** J1939 NAME: 11-bit manufacturer code. */
	uint16_t manufacturer_code;

	/** J1939 NAME: 3-bit ECU instance. */
	uint8_t ecu_instance;

	/** J1939 NAME: 5-bit function instance. */
	uint8_t function_instance;

	/** J1939 NAME: 8-bit function code. */
	uint8_t function;

	/** J1939 NAME: 7-bit vehicle system. */
	uint8_t vehicle_system;

	/** J1939 NAME: 4-bit vehicle system instance. */
	uint8_t vehicle_system_instance;

	/** J1939 NAME: 3-bit industry group. */
	uint8_t industry_group;

	/** J1939 NAME: arbitrary address capable (self-configurable). */
	bool arbitrary_address_capable;

    /** State of the address claim process for this node. */
    J1939Ac_State_T node_state;

    /** Timestamp of when we first claimed an address, used for tie-breaking in address contention. */
    uint32_t ac_timestamp;

    /** Recorded bus information for this node. */
    J1939Ac_RecordBusInfo_T recorded_bus_info[CONFIG_J1939_MAX_NODES_IN_SYSTEM];

    /** Number of recorded bus information entries for this node. */
    uint8_t recorded_bus_info_count;

    /** Transmission is enabled */
    bool transmission_enabled;

    /** PGN Request list */
    J1939_PgnRequest_T J1939_PgnRequestList[CONFIG_J1939_MAX_PGN_REQUEST_MESSAGES];

    /** Number of PGNs requested */
    uint8_t J1939_RequestedPgnCount;

    /** BAM transmission status for this node. */
    bool J1939Tp_TransmitBam;

    /** List of all the PGNs that the module accepts */
    J1939Tp_PgnParams_T J1939Tp_RegisterPgnList[CONFIG_J1939TP_NUM_ALLOWED_RECEIVE_PGN];

    /** Provides count of # of registered PGNs and indexes where the next registered PGN value should go */
    uint8_t J1939Tp_RegisterPgnIndex;
};

/**
 * @brief Initialise a @ref j1939_dt_node_cfg from a DT instance number.
 *
 * @param inst DT instance number for a zephyr,j1939-node.
 *
 * Example:
 * @code{.c}
 *   #define DT_DRV_COMPAT zephyr_j1939_node
 *   static const struct j1939_dt_node_cfg my_nodes[] = {
 *       DT_INST_FOREACH_STATUS_OKAY(J1939_DT_INST_CFG_DEFINE)
 *   };
 * @endcode
 */
#define J1939_DT_INST_CFG_DEFINE(inst)                                 \
	{                                                              \
		.can_dev                 = J1939_DT_INST_CAN_DEV(inst),        \
		.source_address          = J1939_DT_INST_SOURCE_ADDRESS(inst), \
		.id_number               = J1939_DT_INST_ID_NUMBER(inst),      \
		.manufacturer_code       = J1939_DT_INST_MFG_CODE(inst),       \
		.ecu_instance            = J1939_DT_INST_ECU_INSTANCE(inst),   \
		.function_instance       = J1939_DT_INST_FUNC_INSTANCE(inst),  \
		.function                = J1939_DT_INST_FUNCTION(inst),       \
		.vehicle_system          = J1939_DT_INST_VEHICLE_SYSTEM(inst), \
		.vehicle_system_instance = J1939_DT_INST_VS_INSTANCE(inst),    \
		.industry_group          = J1939_DT_INST_INDUSTRY_GROUP(inst), \
		.arbitrary_address_capable = J1939_DT_INST_ARBITRARY_ADDR(inst), \
		.node_state              = J1939Ac_State_WaitingStartupInit, \
		.ac_timestamp            = 0, \
		.recorded_bus_info_count = 0, \
		.transmission_enabled    = false, \
        .J1939Tp_TransmitBam         = false, \
	},

/**
 * @brief Expands to a comma-separated list of @ref j1939_dt_node_cfg
 *        initialisers for every enabled zephyr,j1939-node in the DTS.
 *
 * Use this as the body of a static const array declaration:
 *
 * @code{.c}
 *   static const struct j1939_dt_node_cfg nodes[] = {
 *       J1939_DT_NODE_CFG_ALL_DEFINE
 *   };
 * @endcode
 */
#define J1939_DT_NODE_CFG_ALL_DEFINE \
	DT_FOREACH_STATUS_OKAY(zephyr_j1939_node, J1939_DT_NODE_CFG_INIT_FROM_NODE)

/* internal helper – node_id variant used by J1939_DT_NODE_CFG_ALL_DEFINE */
#define J1939_DT_NODE_CFG_INIT_FROM_NODE(node_id)                              \
	{                                                                       \
		.can_dev                 = J1939_DT_NODE_CAN_DEV(node_id),             \
		.source_address          = J1939_DT_NODE_SOURCE_ADDRESS(node_id),      \
		.id_number               = J1939_DT_NODE_ID_NUMBER(node_id),           \
		.manufacturer_code       = J1939_DT_NODE_MFG_CODE(node_id),            \
		.ecu_instance            = J1939_DT_NODE_ECU_INSTANCE(node_id),        \
		.function_instance       = J1939_DT_NODE_FUNC_INSTANCE(node_id),       \
		.function                = J1939_DT_NODE_FUNCTION(node_id),            \
		.vehicle_system          = J1939_DT_NODE_VEHICLE_SYSTEM(node_id),      \
		.vehicle_system_instance = J1939_DT_NODE_VS_INSTANCE(node_id),         \
		.industry_group          = J1939_DT_NODE_INDUSTRY_GROUP(node_id),      \
		.arbitrary_address_capable = J1939_DT_NODE_ARBITRARY_ADDR(node_id),    \
        .node_state              = J1939Ac_State_WaitingStartupInit, \
        .ac_timestamp            = 0, \
        .recorded_bus_info_count = 0, \
        .transmission_enabled    = false, \
        .J1939Tp_TransmitBam         = false, \
	},

/**
 * @brief Static initializer for @p j1939_dt_node_cfg struct
 *
 * @param node_id Devicetree node identifier
 */
#define J1939_DT_DRIVER_CONFIG_GET(node_id)				\
	{                                                                       \
		.can_dev                 = J1939_DT_NODE_CAN_DEV(node_id),             \
		.source_address          = J1939_DT_NODE_SOURCE_ADDRESS(node_id),      \
		.id_number               = J1939_DT_NODE_ID_NUMBER(node_id),           \
		.manufacturer_code       = J1939_DT_NODE_MFG_CODE(node_id),            \
		.ecu_instance            = J1939_DT_NODE_ECU_INSTANCE(node_id),        \
		.function_instance       = J1939_DT_NODE_FUNC_INSTANCE(node_id),       \
		.function                = J1939_DT_NODE_FUNCTION(node_id),            \
		.vehicle_system          = J1939_DT_NODE_VEHICLE_SYSTEM(node_id),      \
		.vehicle_system_instance = J1939_DT_NODE_VS_INSTANCE(node_id),         \
		.industry_group          = J1939_DT_NODE_INDUSTRY_GROUP(node_id),      \
		.arbitrary_address_capable = J1939_DT_NODE_ARBITRARY_ADDR(node_id),    \
		.node_state              = J1939Ac_State_WaitingStartupInit, \
        .ac_timestamp            = 0, \
        .recorded_bus_info_count = 0, \
        .transmission_enabled    = false, \
        .J1939Tp_TransmitBam         = false, \
	}

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_CANBUS_J1939_DT_H_ */
