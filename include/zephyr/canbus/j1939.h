/*
 * Copyright (c) 2026 Deere & Company
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for J1939
 *
 * J1939 is a higher-layer protocol for CAN (Controller Area Network)
 */

#ifndef ZEPHYR_INCLUDE_CANBUS_J1939_H_
#define ZEPHYR_INCLUDE_CANBUS_J1939_H_

/**
 * @brief CAN J1939 Protocol
 * @defgroup can_j1939 CAN J1939 Protocol
 * @ingroup connectivity
 * @since 4.4
 * @version 0.1.0
 * @{
 */

#include <zephyr/drivers/can.h>

/** Extract the low byte from a 16-bit value. */
#define LOBYTE(w) ((uint8_t)((uint16_t)(w) & 0xFFU))
/** Extract the high byte from a 16-bit value. */
#define HIBYTE(w) ((uint8_t)(((uint16_t)(w) >> 8) & 0xFFU))
/** Extract the low word from a 32-bit value. */
#define LOWORD(l) ((uint16_t)((uint32_t)(l) & 0xFFFFU))
/** Extract the high word from a 32-bit value. */
#define HIWORD(l) ((uint16_t)(((uint32_t)(l) >> 16) & 0xFFFFU))
/** Combine low and high bytes into a 16-bit word. */
#define MAKEWORD(lo, hi) ((uint16_t)(((uint16_t)(uint8_t)(lo)) | ((uint16_t)(uint8_t)(hi) << 8)))
/** Combine low and high words into a 32-bit double word. */
#define MAKEDWORD(lo, hi) \
	((uint32_t)(((uint32_t)(uint16_t)(lo)) | ((uint32_t)(uint16_t)(hi) << 16)))
/** Count elements in a fixed-size array. */
#define ELEMENTS(a) (sizeof(a) / sizeof((a)[0]))

/** 8-bit J1939 node address type. */
typedef uint8_t j1939_address_t;

/** J1939 source address (SA) field type. */
typedef j1939_address_t j1939_source_address_t;

/** J1939 destination address (DA) field type. */
typedef j1939_address_t j1939_destination_address_t;

/** J1939 memory address type. */
typedef uint32_t j1939_memory_address_t;

/** J1939 parameter group number (PGN) type. */
typedef uint32_t j1939_pgn_t;

/** J1939 PDU specific (PS) field type. */
typedef uint8_t j1939_pdu_specific_t;

/** J1939 PDU format (PF) field type. */
typedef uint8_t j1939_pdu_format_t;

/** J1939 security seed value type. */
typedef uint16_t j1939_security_seed_t;

/** J1939 security key value type. */
typedef uint16_t j1939_security_key_t;

/** J1939 timer tick/counter type. */
typedef uint32_t j1939_timer_t;

/** Encoded 29-bit J1939 arbitration identifier type. */
typedef uint32_t j1939_arbitration_t;

/** Generic J1939 counter type. */
typedef uint32_t j1939_counter_t;

/** Byte-sized J1939 counter type. */
typedef uint8_t j1939_byte_counter_t;

/** J1939 DTC counter type. */
typedef j1939_counter_t j1939_dtc_counter_t;

/** J1939 suspect parameter number (SPN) type. */
typedef uint32_t j1939_spn_t;

/** J1939 DTC conversion method type. */
typedef uint8_t j1939_dtc_conversion_method_t;

/** J1939 group extension field type. */
typedef uint8_t j1939_group_extension_t;


/** Failure mode identification codes as defined by J1939, appendix A, SAE J1939-71 Draft JUN98. */
typedef enum j1939_fmi_e {
	J1939_Fmi_0, /**< Valid, but above normal operating range. */
	J1939_Fmi_1, /**< Valid, but below normal operating range. */
	J1939_Fmi_2, /**< Data erratic, intermittent or incorrect. */
	J1939_Fmi_3, /**< Voltage above normal, or shorted to high source. */
	J1939_Fmi_4, /**< Voltage below normal, or shorted to low source. */
	J1939_Fmi_5, /**< Current below normal, or open circuit. */
	J1939_Fmi_6, /**< Current above normal, or grounded circuit. */
	J1939_Fmi_7, /**< Mechanical system not responding or out of adjustment. */
	J1939_Fmi_8, /**< Abnormal frequency, pulse width or period. */
	J1939_Fmi_9, /**< Abnormal update rate. */
	J1939_Fmi_10, /**< Abnormal rate of change. */
	J1939_Fmi_11, /**< Failure code not identifiable. */
	J1939_Fmi_12, /**< Bad intelligent device or component. */
	J1939_Fmi_13, /**< Out of calibration. */
	J1939_Fmi_14, /**< Special Instructions. */
	/** FMI_15 to FMI_30 Not defined. */
	J1939_Fmi_31 = 31, /**< Not available. */
	J1939_Fmi_255 = 255 /**< Not possible FMI value. */
} j1939_fmi_t;

/** Combined SPN/FMI packed value type. */
typedef uint32_t j1939_spn_fmi_t;

/**
 * @brief DM4 freeze-frame specific field types.
 *
 * These types are specifically for DM4 and therefore use the `j1939_dm4_` prefix.
 */
/** DM4 freeze-frame torque mode field type. */
typedef uint8_t j1939_dm4_freeze_frame_engine_torque_mode_t;
/** DM4 freeze-frame intake manifold boost pressure field type. */
typedef uint8_t j1939_dm4_freeze_frame_boost_t;
/** DM4 freeze-frame engine speed field type. */
typedef uint16_t j1939_dm4_freeze_frame_engine_speed_t;
/** DM4 freeze-frame engine load percentage field type. */
typedef uint8_t j1939_dm4_freeze_frame_engine_load_percent_t;
/** DM4 freeze-frame engine coolant temperature field type. */
typedef uint8_t j1939_dm4_freeze_frame_engine_coolant_temp_t;
/** DM4 freeze-frame vehicle speed field type. */
typedef uint16_t j1939_dm4_freeze_frame_vehicle_speed_t;

/** NULL (unclaimed) J1939 source address value. */
#define J1939_NULL_ADDRESS ((j1939_address_t)0xFE)
/** Global destination address (broadcast). */
#define J1939_GLOBAL_ADDRESS ((j1939_address_t)0xFF)

/** Byte value used by J1939 to indicate unavailable data. */
#define J1939_BYTE_UNAVAILABLE (0xFF)

/** PF value for group function 232 (0xE8). */
#define J1939_PDUF_232 ((j1939_pdu_format_t)0xE8)
/** PF value for transport protocol data transfer 235 (0xEB). */
#define J1939_PDUF_235 ((j1939_pdu_format_t)0xEB)
/** PF value for transport protocol connection management 236 (0xEC). */
#define J1939_PDUF_236 ((j1939_pdu_format_t)0xEC)
/** PF value for PDU2 messages at 240 (0xF0). */
#define J1939_PDUF_240 ((j1939_pdu_format_t)0xF0)

/** Diagnostic Message 1 PGN. */
#define J1939_DM1_PGN ((j1939_pgn_t)0xFECA)
/** Diagnostic Message 2 PGN. */
#define J1939_DM2_PGN ((j1939_pgn_t)0xFECB)
/** Diagnostic Message 3 PGN. */
#define J1939_DM3_PGN ((j1939_pgn_t)0xFECC)
/** Diagnostic Message 4 PGN. */
#define J1939_DM4_PGN ((j1939_pgn_t)0xFECD)
/** Diagnostic Message 5 PGN. */
#define J1939_DM5_PGN ((j1939_pgn_t)0xFECE)
/** Diagnostic Message 11 PGN. */
#define J1939_DM11_PGN ((j1939_pgn_t)0xFED3)

/** Address Claimed PGN. */
#define J1939_ADDRESS_CLAIMED_PGN ((j1939_pgn_t)0xEE00)
/** Request PGN. */
#define J1939_REQUEST_PGN ((j1939_pgn_t)0xEA00)
/** Software Identification PGN. */
#define J1939_SOFTWARE_ID_PGN ((j1939_pgn_t)0xFEDA)
/** ECU Identification Information PGN. */
#define J1939_ECU_ID_INFO_PGN ((j1939_pgn_t)0xFDC5)

/** PDU specific value for active DTC (DM1). */
#define J1939_ACTIVE_DTC_PS ((j1939_pdu_specific_t)202)      /* DM1 message */
/** PDU specific value for freeze frame (DM4). */
#define J1939_FREEZE_FRAME_PS ((j1939_pdu_specific_t)205)    /* DM4 message */
/** PDU specific value for previously active DTC (DM2). */
#define J1939_PREVIOUS_ACTIVE_PS ((j1939_pdu_specific_t)203) /* DM2 message */

/** Alias for PF value used by ACK/NACK responses. */
#define J1939_PGN_ACK_PF J1939_PDUF_232
/** Alias for PF value used by TP data transfer frames. */
#define J1939_TP_DATA_TRANSFER_PF J1939_PDUF_235
/** Alias for PF value used by TP connection management frames. */
#define J1939_TP_CONN_MANAGEMENT_PF J1939_PDUF_236

/** Priority field values for J1939 29-bit identifiers. */
typedef enum j1939_priority_e {
	J1939_Priority_0, /**< Highest priority level. */
	J1939_Priority_1, /**< Priority level 1. */
	J1939_Priority_2, /**< Priority level 2. */
	J1939_Priority_3, /**< Priority level 3. */
	J1939_Priority_4, /**< Priority level 4. */
	J1939_Priority_5, /**< Priority level 5. */
	J1939_Priority_6, /**< Priority level 6. */
	J1939_Priority_7, /**< Lowest priority level. */
} j1939_priority_t;

/** ACK/NACK response type. */
typedef enum j1939_response_e {
	J1939_Response_Ack, /**< Positive acknowledgment response. */
	J1939_Response_Nack /**< Negative acknowledgment response. */
} j1939_response_t;

/** 21-bit identity number field from J1939 NAME. */
typedef uint32_t j1939_id_number_t;
/** 11-bit manufacturer code field from J1939 NAME. */
typedef uint16_t j1939_manufacturer_code_t;
/** ECU instance field from J1939 NAME. */
typedef uint8_t j1939_ecu_instance_t;
/** Function instance field from J1939 NAME. */
typedef uint8_t j1939_function_instance_t;
/** Function field from J1939 NAME. */
typedef uint8_t j1939_function_t;
/** Vehicle system field from J1939 NAME. */
typedef uint8_t j1939_vehicle_system_t;
/** Vehicle system instance field from J1939 NAME. */
typedef uint8_t j1939_vehicle_system_instance_t;
/** Industry group field from J1939 NAME. */
typedef uint8_t j1939_industry_group_t;
/** Reserved bit field from J1939 NAME. */
typedef uint8_t j1939_reserved_t;

/** J1939 NAME fields used in Address Claimed messages. */
typedef struct j1939_name_s {
	j1939_id_number_t idNumber; /**< 21-bit identity number */
	j1939_manufacturer_code_t mfgCode; /**< 11-bit manufacturer code */
	j1939_ecu_instance_t ecuInstance; /**< ECU instance */
	j1939_function_instance_t functionInstance; /**< Function instance */
	j1939_function_t function; /**< Function */
	j1939_vehicle_system_t vehicleSystem; /**< Vehicle system */
	j1939_vehicle_system_instance_t vehicleSystemInstance; /**< Vehicle system instance */
	j1939_industry_group_t industryGroup; /**< Industry group */
	bool isSelfConfig; /**< Arbitrary-address-capable indicator. */
	j1939_reserved_t reservedBit; /**< Reserved bit */
} j1939_name_t;


/** Internal state machine values for J1939 address claiming. */
typedef enum j1939_ac_state_e {
	J1939_AC_STATE_WAITING_STARTUP_INIT, /**< Waiting for startup initialization delay. */
	J1939_AC_STATE_START, /**< Starting address claim procedure. */
	J1939_AC_STATE_WAITING, /**< Waiting for contention/claim timeout. */
	J1939_AC_STATE_CLAIMED, /**< Address successfully claimed. */
	J1939_AC_STATE_LOST_CONTENTION, /**< Lost contention for the selected address. */
	J1939_AC_STATE_WAITING_CANNOT_CLAIM, /**< Waiting before sending cannot-claim. */
	J1939_AC_STATE_CANNOT_CLAIM /**< Node cannot claim an address. */
} j1939_ac_state_t;

/** Address-claim bus record entry. */
typedef struct j1939_ac_record_bus_info_s {
	uint8_t source; /**< Source address associated with this record. */
#if defined(J1939_RECORD_ADDRESS_CLAIMED_NAMES)
	j1939_name_t name; /**< J1939 NAME recorded for the source address. */
#endif
} j1939_ac_record_bus_info_t;

/** Entry used to track PGNs that can be requested by peers. */
typedef struct J1939_PgnRequest_S {
	uint16_t pgn; /**< PGN registered and requested. */
	uint8_t source; /**< Source address of the registered/requested PGN. */
	bool isRequested; /**< True if the PGN has been requested. */
	bool isUsed; /**< True if this entry is in use. */
} j1939_pgn_request_t;

/** Source address and NAME tuple observed during address claim. */
typedef struct j1939_ac_bus_info_S {
	j1939_source_address_t source; /**< Source address associated with the NAME. */
	j1939_name_t name; /**< J1939 NAME associated with the source address. */
} j1939_ac_bus_info_t;

/** Opaque pointer type for a J1939 node configuration/instance. */
typedef struct j1939_node_cfg *j1939_node_t;

/** Callback used by PF routing table handlers. */
typedef bool (*j1939_routing_callback_t)(const struct can_frame *message, j1939_node_t node);

/**
 * Callback function pointer for transport sessions
 */
typedef bool (*j1939_tp_callback_t)(uint16_t pgn, uint8_t *data,
					 uint32_t length, uint8_t sender,
					 j1939_node_t node);

/** Mapping between a PGN and its registered transport callback. */
typedef struct j1939_tp_pgn_params_s {
	uint16_t pgn; /**< PGN to match for transport callbacks. */
	j1939_tp_callback_t callback; /**< Callback invoked for matching PGN data. */
} j1939_tp_pgn_params_t;

/**
 * @brief Compile-time configuration for one J1939 virtual node.
 *
 * Populated entirely from devicetree; no runtime init required.
 */
struct j1939_node_cfg {
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
	j1939_ac_state_t node_state;

	/**
	 * Timestamp of when we first claimed an address,
	 * used for tie-breaking in address contention.
	 */
	uint32_t ac_timestamp;

	/** Recorded bus information for this node. */
	j1939_ac_record_bus_info_t recorded_bus_info[CONFIG_J1939_MAX_NODES_IN_SYSTEM];

	/** Number of recorded bus information entries for this node. */
	uint8_t recorded_bus_info_count;

	/** Transmission is enabled. */
	bool transmission_enabled;

	/** PGN request list. */
	j1939_pgn_request_t j1939_pgn_request_list[CONFIG_J1939_MAX_PGN_REQUEST_MESSAGES];

	/** Number of PGNs requested. */
	uint8_t j1939_requested_pgn_count;

	/** BAM transmission status for this node. */
	bool j1939_tp_transmit_bam;

	/** List of all PGNs that the module accepts. */
	j1939_tp_pgn_params_t j1939_tp_register_pgn_list[CONFIG_J1939TP_NUM_ALLOWED_RECEIVE_PGN];

	/**
	 * Count of registered PGNs and the index where the next PGN
	 * registration should be stored.
	 */
	uint8_t j1939_tp_register_pgn_index;

	/** Address claim bus information for this node. */
	j1939_ac_bus_info_t j1939_ac_bus_info;

	/** Indicates if an address claim is requested for this node. */
	bool j1939_ac_is_requested;
};

/**
 * @brief Define a compile-time J1939 virtual node.
 *
 * @param node_name Symbol name for the generated node object.
 * @param _can_dev CAN controller device pointer.
 * @param _source_address J1939 source address (0x00-0xFD).
 * @param _id_number J1939 NAME identity number.
 * @param _manufacturer_code J1939 NAME manufacturer code.
 * @param _ecu_instance J1939 NAME ECU instance.
 * @param _function_instance J1939 NAME function instance.
 * @param _function J1939 NAME function.
 * @param _vehicle_system J1939 NAME vehicle system.
 * @param _vehicle_system_instance J1939 NAME vehicle system instance.
 * @param _industry_group J1939 NAME industry group.
 * @param _arbitrary_address_capable J1939 NAME arbitrary address capable flag.
 */
#define J1939_NODE_DEFINE(node_name, _can_dev, _source_address, _id_number, \
			  _manufacturer_code, _ecu_instance, _function_instance, _function, \
				  _vehicle_system, _vehicle_system_instance, _industry_group, \
				  _arbitrary_address_capable) \
	STRUCT_SECTION_ITERABLE(j1939_node_cfg, node_name) = { \
		.can_dev = _can_dev, \
		.source_address = _source_address, \
		.id_number = _id_number, \
		.manufacturer_code = _manufacturer_code, \
		.ecu_instance = _ecu_instance, \
		.function_instance = _function_instance, \
		.function = _function, \
		.vehicle_system = _vehicle_system, \
		.vehicle_system_instance = _vehicle_system_instance, \
		.industry_group = _industry_group, \
		.arbitrary_address_capable = _arbitrary_address_capable, \
		.node_state = J1939_AC_STATE_WAITING_STARTUP_INIT, \
		.ac_timestamp = 0, \
		.recorded_bus_info_count = 0, \
		.transmission_enabled = false, \
		.j1939_requested_pgn_count = 0, \
		.j1939_tp_transmit_bam = false, \
		.j1939_tp_register_pgn_index = 0, \
	}

/** @brief Get the J1939 priority field from an arbitration ID. */
static inline j1939_priority_t j1939_get_priority(j1939_arbitration_t arbitration)
{
	return (j1939_priority_t)((arbitration >> 26) & 0x07);
}

/** @brief Get the PDU format field from an arbitration ID. */
static inline j1939_pdu_format_t j1939_get_pdu_format(j1939_arbitration_t arbitration)
{
	return (j1939_pdu_format_t)(LOBYTE(HIWORD(arbitration)));
}

/** @brief Get the PDU specific field from an arbitration ID. */
static inline j1939_pdu_specific_t j1939_get_pdu_specific(j1939_arbitration_t arbitration)
{
	return (j1939_pdu_specific_t)(HIBYTE(LOWORD(arbitration)));
}

/** @brief Get the source address field from an arbitration ID. */
static inline j1939_source_address_t j1939_get_source_address(j1939_arbitration_t arbitration)
{
	return ((j1939_source_address_t)LOBYTE(LOWORD(arbitration)));
}

/** @brief Get the data page (DP) bit from an arbitration ID. */
static inline bool j1939_get_data_page(j1939_arbitration_t arbitration)
{
	return (bool)(LOBYTE(HIWORD(arbitration)) & 0x01);
}

/** @brief Get the data page (DP) bit from a PGN value. */
static inline bool j1939_get_data_page_from_pgn(j1939_pgn_t pgn)
{
	return (bool)((pgn & 0x10000) >> 16);
}

/** @brief Get the extended data page (EDP) bit from a PGN value. */
static inline bool j1939_get_extended_data_page_from_pgn(j1939_pgn_t pgn)
{
	return (bool)((pgn & 0x20000) >> 17);
}

/** @brief Build a PGN value from PDU format and PDU specific fields. */
static inline j1939_pgn_t j1939_build_pgn_from_pdu(j1939_pdu_format_t pduf,
								j1939_pdu_specific_t pdus)
{
	return (j1939_pgn_t)MAKEWORD(pdus, pduf);
}

/** @brief Build a 29-bit J1939 arbitration identifier from fields. */
static inline j1939_arbitration_t j1939_build_message_id(bool DP,
								   bool EDP,
								   j1939_priority_t PR,
								   j1939_pgn_t PGN,
								   j1939_source_address_t SA)
{
	return ((((uint32_t)((uint8_t)(PR) & (uint8_t)0x07)) << 26) |
			(((uint32_t)((uint16_t)(PGN)&0xFFFF)) << 8) |
			((uint32_t)((uint8_t)(SA) & (uint8_t)0xFF)) |
			(((uint32_t)((uint8_t)(DP) & (uint8_t)0x01)) << 24) |
			(((uint32_t)((uint8_t)(EDP) & (uint8_t)0x01)) << 25));
}

/**
 * Initialize J1939 routines
 */
void j1939_init(void);

/**
 * Periodic J1939 message processing routine.  This routine updates the various features that are
 * configured(transport, address claim, memory access, etc).  It should be called periodically
 */
void j1939_task(void);

/**
 * This function searches the PGN request list to find out if a request for the specified
 * Parameter Group Number has been requested by a module on the CAN bus.
 * @param pgn Parameter Group Number of Request.
 * @param source Pointer to application level variable where the source address of the requester
 * gets stored, if a PGN was requested.
 * @param node Node to look at
 * @return True If the PGN was requested by another module on the CAN bus. false If the PGN was not
 * requested.
 */
bool j1939_is_pgn_requested(j1939_pgn_t pgn, j1939_source_address_t *source,
								  j1939_node_t node);

/**
 * @brief Set a PGN request within the PGN request list.
 * @param pgn PGN to set request
 * @param source Source address requesting the PGN
 * @param node J1939 node to flag
 * @return True if PGN is in the list, false if not.
 */
bool j1939_flag_pgn_request(j1939_pgn_t pgn, j1939_source_address_t source, j1939_node_t node);

/**
 * @brief Send an ACK/NACK to another J1939 node
 * @param pgn PGN that is to be ACK'd or NACK'd
 * @param control Response of ACK or NACK
 * @param destination Source address of the response
 * @param node J1939 node to send response from
 */
void j1939_acknowledge(j1939_pgn_t pgn, j1939_response_t control,
			   j1939_destination_address_t destination, j1939_node_t node);

/**
 * @brief Send a PGN request to the specified node.
 * @param pgn PGN request to send
 * @param destination Source address of destination unit
 * @param node J1939 node to send request from
 * @return True if request was successfully sent, false if not.
 */
bool j1939_transmit_pgn_request(j1939_pgn_t pgn, j1939_destination_address_t destination,
									  j1939_node_t node);

/**
 * @brief Add a PGN to the list for request processing.
 * @param pgn PGN of requests to look for
 * @param node J1939 node to watch
 * @return True if PGN was added to list, false if not.
 */
bool j1939_register_request_pgn(j1939_pgn_t pgn, j1939_node_t node);

/**
 * @brief Sends out the data under the PGN specified. The use of transport protocol vs a std CAN
 * message is handled within this function.
 * @note The destination address is only used if the message PF is less than 240 or if the message
 * needs the transport layer (more than 8 bytes.) Setting the destination address to 255 specifies
 * a BAM transport message.  Other addresses specify the use of connection management for the
 * transport layer.
 * @param priority The Priority of the message
 * @param pgn The PGN to send the data out under
 * @param destination Destination address of a TP message
 * @param data Pointer to the data to send
 * @param count Number of bytes of the data to send
 * @param node Node which to send message out on
 * @return True if message was sent, false if not queued.
 */
bool j1939_transmit_pgn(j1939_priority_t priority, j1939_pgn_t pgn,
			   j1939_destination_address_t destination, uint8_t *data,
			   j1939_counter_t count, j1939_node_t node);

/**
 * @brief Check if provided PGN is valid
 * @param pgn PGN to check
 * @return True if valid, false if not.
 */
bool j1939_is_pgn_valid(j1939_pgn_t pgn);

#ifdef J1939_ENABLE_RECEIVED_PGN_SUPPORT
/** Callback type for single-frame received PGN handlers. */
typedef bool (*j1939_received_pgn_callback_t)(const struct can_frame *message);

/**
 * @brief Register a handler for the specified PGN on the specified J1939 address node.
 * @note These PGNs are handled AFTER the PF routing table .If the routing table callback indicates
 * that it handles a message it will never be handled by your callback.Basically, if it goes to
 * j1939_default_route(), then your callback will be able to handle it.
 * @warning This only handles single message PGNs. If you wish to handle transport message PGNs,
 * you will need to register a PGN callback with the transport layer using the
 * j1939tp_register_message_callback() function instead.
 * @param pgn PGN to handle
 * @param handler Callback function to handle the received PGN, must not be NULL
 * @param node J1939 node the message is received on
 * @return True on success, false if not.
 */
bool j1939_register_receive_pgn_callback(j1939_pgn_t pgn, j1939_received_pgn_callback_t handler,
							  j1939_node_t node);
#endif

/**
 * @brief Default routing table path
 * @param constmessage Received message to process
 * @param node J1939 node the message is received on
 * @return False
 */
bool j1939_default_route(const struct can_frame *constmessage, j1939_node_t node);

/**
 * @brief Routing table handler for PDUF 254 (0xFE)
 * @param message Received message to process
 * @param node J1939 node the message is received on
 * @return False
 */
bool j1939_pdu_f254_process(const struct can_frame *message, j1939_node_t node);

/**
 * @brief Routing table handler for request PGN (0xEA)
 * @param message Received message to process
 * @param node J1939 node the message is received on
 * @return False
 */
bool j1939_request_pgn(const struct can_frame *message, j1939_node_t node);

/**
 * @brief Extracts the PGN from a message ID
 * @param messageId Message ID to parse
 * @return PGN in Message ID
 */
j1939_pgn_t j1939_get_pgn(j1939_arbitration_t messageId);

/**
 * @brief Build up a PGN in the proper format
 * @param extendedDataPage Value to set the extended data page to (zero or one)
 * @param dataPage Value to set the data page to (zero or one)
 * @param pduf PDU Format field
 * @param groupExtension Group extension field
 * @return Formatted PGN
 */
j1939_pgn_t j1939_build_pgn(bool extendedDataPage, bool dataPage,
			   j1939_pdu_format_t pduf,
			   j1939_group_extension_t groupExtension);

/**
 * Enables the transmission of messages from a given J1939 address node
 * @param[in] node J1939 address node to enable messages
 */
void j1939_enable_virtual_mode_transmit(j1939_node_t node);

/**
 * Disables the transmission of messages from a given J1939 address node
 * @param[in] node J1939 address node to disable messages
 */
void j1939_disable_virtual_mode_transmit(j1939_node_t node);

/**
 * Check if the specified J1939 address node's transmit queue is enabled
 * @param[in] node J1939 address node to check
 * @return True if enabled, false otherwise
 */
bool j1939_is_virtual_node_transmit_enabled(j1939_node_t node);

/**
 * Process the Build and queue of CAN message for transmit for a J1939 address node
 * @param[in] node Node to transmit on
 * @param[in] arbitration ID to transmit(right justified)
 * @param[in] dataLength length to transmit
 * @param[in] isExtendedMessage true for 29 bit messages, false for 11 bit messages
 * @param[in] data Data to send
 * @return true if the message was queued for transmit, false otherwise
 */
bool j1939_build_and_queue_message(j1939_node_t node, j1939_arbitration_t arbitration,
							uint16_t dataLength,
							bool isExtendedMessage,
							const uint8_t *data);

/** Application-level PF routing table used by the J1939 core. */
extern const j1939_routing_callback_t J1939_App_RoutingTable[];

/**
 * Application specific initialization
 */
void j1939_app_init(void);

/** Device identification strings for J1939 ECU/software identification messages. */
typedef struct j1939_device_info_s {
	/** Serial number of the device */
	const char *hardware_serial_number;

	/** Hardware revision of the device */
	const char *hardware_revision;

	/** Part number of the device */
	const char *hardware_part_number;
} j1939_device_info_t;

/**
 * @brief Get the device information for this hardware
 * @param device_info Pointer to a j1939_device_info_t structure to populate with
 * device information
 */
void j1939_app_get_device_info(j1939_device_info_t *device_info);

/**
 * @brief User implements this function to fill out software ID information
 * according to SAE J1939 PGN 0xFEDA
 * @param software_id_ptr Pointer to a buffer to populate with software ID information
 */
void j1939_get_software_id(uint8_t **software_id_ptr);

/**
 * Handler for messages with a PF of 254.  This is one of the most common PFs we handle but the
 * there are some messages the layer handles for you.  This is done prior to calling this function.
 * @param message The message to process
 * @param node J1939 node the message is received on
 * @return True if successful, false otherwise
 */
bool j1939_app_pf254_process(const struct can_frame *message, j1939_node_t node);

/**
 * @brief Send out the ECU ID PGN
 * @param destination  Destination address to send to
 * @param node J1939 node to transmit on
 */
void j1939_app_transmit_ecu_id(j1939_destination_address_t destination, j1939_node_t node);

/**
 * @brief Send out the software ID PGN
 * @param destination  Destination address to send to
 * @param node J1939 node to transmit on
 */
void j1939_app_transmit_software_id(j1939_destination_address_t destination, j1939_node_t node);

/**
 * Get the J1939 address node index based on the PDU specific field in J1939Nodee message and
 * the physical node associated to it.
 * @param[in] address
 * @return J1939 address node index
 */
j1939_node_t j1939_app_get_j1939_node_from_source_address(j1939_source_address_t address);

/**
 * @brief Get the J1939 for a given source address
 * @param address Source address to match to J1939 node
 * @return J1939 node that source address belongs
 */
static inline j1939_node_t j1939_get_j1939_node_from_source_address(
	j1939_source_address_t address)
{
	return j1939_app_get_j1939_node_from_source_address(address);
}

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_CANBUS_J1939_H_ */
