/*
 * Copyright (c) 2026 Deere & Company
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_CANBUS_J1939_H_
#define ZEPHYR_INCLUDE_CANBUS_J1939_H_

#include <zephyr/drivers/can.h>

#define LOBYTE(w) ((uint8_t)((uint16_t)(w) & 0xFFU))
#define HIBYTE(w) ((uint8_t)(((uint16_t)(w) >> 8) & 0xFFU))
#define LOWORD(l) ((uint16_t)((uint32_t)(l) & 0xFFFFU))
#define HIWORD(l) ((uint16_t)(((uint32_t)(l) >> 16) & 0xFFFFU))
#define MAKEWORD(lo, hi) ((uint16_t)(((uint16_t)(uint8_t)(lo)) | ((uint16_t)(uint8_t)(hi) << 8)))
#define MAKEDWORD(lo, hi) ((uint32_t)(((uint32_t)(uint16_t)(lo)) | ((uint32_t)(uint16_t)(hi) << 16)))
#define ELEMENTS(a) (sizeof(a) / sizeof((a)[0]))

#define J1939_DT_NUM_NODES 1

typedef uint8_t j1939_address_t;

typedef j1939_address_t j1939_source_address_t;

typedef j1939_address_t j1939_destination_address_t;

typedef uint32_t j1939_memory_address_t;

typedef uint32_t j1939_pgn_t;

typedef uint8_t j1939_pdu_specific_t;

typedef uint8_t j1939_pdu_format_t;

typedef uint16_t j1939_security_seed_t;

typedef uint16_t j1939_security_key_t;

typedef uint32_t j1939_timer_t;

typedef uint32_t j1939_arbitration_t;

typedef uint32_t j1939_counter_t;

typedef uint8_t j1939_byte_counter_t;

typedef j1939_counter_t j1939_dtc_counter_t;

typedef uint32_t j1939_spn_t;

typedef uint8_t j1939_dtc_conversion_method_t;

typedef uint8_t j1939_group_extension_t;

/* lint -strong(AcJdX, Uds_Millisecond_T) */
typedef uint32_t j1939_millisecond_t;

/* lint -strong(AcJdX, Uds_Microsecond_T) */
typedef uint32_t j1939_microsecond_t;

/* Failure mode identification codes as defined by J1939, appendix A, SAE J1939-71 Draft JUN98. */
typedef enum j1939_fmi_e {
	J1939_Fmi_0, /* Valid, but above normal operating range. */
	J1939_Fmi_1, /* Valid, but below normal operating range. */
	J1939_Fmi_2, /* Data erratic, intermittent or incorrect. */
	J1939_Fmi_3, /* Voltage above normal, or shorted to high source. */
	J1939_Fmi_4, /* Voltage below normal, or shorted to low source. */
	J1939_Fmi_5, /* Current below normal, or open circuit. */
	J1939_Fmi_6, /* Current above normal, or grounded circuit. */
	J1939_Fmi_7, /* Mechanical system not responding or out of adjustment. */
	J1939_Fmi_8, /* Abnormal frequency, pulse width or period. */
	J1939_Fmi_9, /* Abnormal update rate. */
	J1939_Fmi_10, /* Abnormal rate of change. */
	J1939_Fmi_11, /* Failure code not identifiable. */
	J1939_Fmi_12, /* Bad intelligent device or component. */
	J1939_Fmi_13, /* Out of calibration. */
	J1939_Fmi_14, /* Special Instructions. */
	/* FMI_15 to FMI_30 Not defined. */
	J1939_Fmi_31 = 31, /* Not available */
	J1939_Fmi_255 = 255
} j1939_fmi_t;

typedef uint32_t j1939_spn_fmi_t;

/* These types are specifically for DM4, which is why they are prefixed as such */
typedef uint8_t j1939_dm4_freeze_frame_engine_torque_mode_t;
typedef uint8_t j1939_dm4_freeze_frame_boost_t;
typedef uint16_t j1939_dm4_freeze_frame_engine_speed_t;
typedef uint8_t j1939_dm4_freeze_frame_engine_load_percent_t;
typedef uint8_t j1939_dm4_freeze_frame_engine_coolant_temp_t;
typedef uint16_t j1939_dm4_freeze_frame_vehicle_speed_t;

#define J1939_NULL_ADDRESS ((j1939_address_t)0xFE)
#define J1939_GLOBAL_ADDRESS ((j1939_address_t)0xFF)

#define J1939_BYTE_UNAVAILABLE (0xFF)

/* PDU Format Byte definitions */
#define J1939_PDUF_232 ((j1939_pdu_format_t)0xE8)
#define J1939_PDUF_235 ((j1939_pdu_format_t)0xEB)
#define J1939_PDUF_236 ((j1939_pdu_format_t)0xEC)
#define J1939_PDUF_240 ((j1939_pdu_format_t)0xF0)

/* Diagnostic PGN's */
#define J1939_DM1_PGN ((j1939_pgn_t)0xFECA)
#define J1939_DM2_PGN ((j1939_pgn_t)0xFECB)
#define J1939_DM3_PGN ((j1939_pgn_t)0xFECC)
#define J1939_DM4_PGN ((j1939_pgn_t)0xFECD)
#define J1939_DM5_PGN ((j1939_pgn_t)0xFECE)
#define J1939_DM11_PGN ((j1939_pgn_t)0xFED3)

/* Other J1939 Protocol PGN's */
#define J1939_ADDRESS_CLAIMED_PGN ((j1939_pgn_t)0xEE00)
#define J1939_REQUEST_PGN ((j1939_pgn_t)0xEA00)
#define J1939_SOFTWARE_ID_PGN ((j1939_pgn_t)0xFEDA)
#define J1939_ECU_ID_INFO_PGN ((j1939_pgn_t)0xFDC5)

/* PS definitions */
#define J1939_ACTIVE_DTC_PS ((j1939_pdu_specific_t)202)      /* DM1 message */
#define J1939_FREEZE_FRAME_PS ((j1939_pdu_specific_t)205)    /* DM4 message */
#define J1939_PREVIOUS_ACTIVE_PS ((j1939_pdu_specific_t)203) /* DM2 message */

/* PF definitions */
#define J1939_PGN_ACK_PF J1939_PDUF_232
#define J1939_TP_DATA_TRANSFER_PF J1939_PDUF_235
#define J1939_TP_CONN_MANAGEMENT_PF J1939_PDUF_236

/* Declarations for Priority Bits in Identifiers */
typedef enum j1939_priority_e {
	J1939_Priority_0,
	J1939_Priority_1,
	J1939_Priority_2,
	J1939_Priority_3,
	J1939_Priority_4,
	J1939_Priority_5,
	J1939_Priority_6,
	J1939_Priority_7,
} j1939_priority_t;

typedef enum j1939_response_e {
	J1939_Response_Ack,
	J1939_Response_Nack
} j1939_response_t;

typedef uint32_t j1939_id_number_t;
typedef uint16_t j1939_manufacturer_code_t;
typedef uint8_t j1939_ecu_instance_t;
typedef uint8_t j1939_function_instance_t;
typedef uint8_t j1939_function_t;
typedef uint8_t j1939_vehicle_system_t;
typedef uint8_t j1939_vehicle_system_instance_t;
typedef uint8_t j1939_industry_group_t;
typedef uint8_t j1939_reserved_t;

/* Structure for address claimed message information */
typedef struct j1939_name_s
{
   j1939_id_number_t idNumber;
   j1939_manufacturer_code_t mfgCode;
   j1939_ecu_instance_t ecuInstance;
   j1939_function_instance_t functionInstance;
   j1939_function_t function;
   j1939_vehicle_system_t vehicleSystem;
   j1939_vehicle_system_instance_t vehicleSystemInstance;
   j1939_industry_group_t industryGroup;
   bool isSelfConfig;
   j1939_reserved_t reservedBit;
} j1939_name_t;


/* TODO - definitely not a great way to do this but not sure where to put this now */
typedef enum j1939_ac_state_e {
	J1939_AC_STATE_WAITING_STARTUP_INIT,
	J1939_AC_STATE_START,
	J1939_AC_STATE_WAITING,
	J1939_AC_STATE_CLAIMED,
	J1939_AC_STATE_LOST_CONTENTION,
	J1939_AC_STATE_WAITING_CANNOT_CLAIM,
	J1939_AC_STATE_CANNOT_CLAIM
} j1939_ac_state_t;

typedef struct j1939_ac_record_bus_info_s {
   uint8_t source; /* Source address associated with the name pointer below */
#if defined(J1939_RECORD_ADDRESS_CLAIMED_NAMES)
   j1939_name_t name;
#endif
} j1939_ac_record_bus_info_t;

/* Structure definition for items which may be requested by another module on the bus. */
typedef struct J1939_PgnRequest_S {
   uint16_t pgn; /* PGN registered and requested */
   uint8_t source; /* Source address of registered or requested PGN */
   bool isRequested; /* True if PGN has been requested */
   bool isUsed; /* True if PGN is used */
} j1939_pgn_request_t;

typedef struct j1939_dt_node_cfg * j1939_node_t;

typedef bool (*j1939_routing_callback_t)(const struct can_frame *message, j1939_node_t node);

/**
 * Callback function pointer for transport sessions
 */
typedef bool (*j1939_tp_callback_t)(uint16_t pgn, uint8_t *data,
                                           uint32_t length, uint8_t sender,
                                           j1939_node_t node);

typedef struct j1939_tp_pgn_params_s
{
   uint16_t pgn;
   j1939_tp_callback_t callback;
} j1939_tp_pgn_params_t;

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
};


/* Get the priority from the arbitration register */
static inline j1939_priority_t j1939_get_priority(j1939_arbitration_t arbitration)
{
   return (j1939_priority_t)((arbitration >> 26) & 0x07);
}

/* Gets the PDU Format byte from the arbitration register */
static inline j1939_pdu_format_t j1939_get_pdu_format(j1939_arbitration_t arbitration)
{
   return (j1939_pdu_format_t)(LOBYTE(HIWORD(arbitration)));
}

/* Gets the PDU Specific byte from the arbitration register */
static inline j1939_pdu_specific_t j1939_get_pdu_specific(j1939_arbitration_t arbitration)
{
   return (j1939_pdu_specific_t)(HIBYTE(LOWORD(arbitration)));
}

/* Gets the Source Address byte from the arbitration register */
static inline j1939_source_address_t j1939_get_source_address(j1939_arbitration_t arbitration)
{
   return ((j1939_source_address_t)LOBYTE(LOWORD(arbitration)));
}

/* Get the Data page from the arbitration register */
static inline bool j1939_get_data_page(j1939_arbitration_t arbitration)
{
   return (bool)(LOBYTE(HIWORD(arbitration)) & 0x01);
}

static inline bool j1939_get_data_page_from_pgn(j1939_pgn_t pgn)
{
   return (bool)((pgn & 0x10000) >> 16);
}

static inline bool j1939_get_extended_data_page_from_pgn(j1939_pgn_t pgn)
{
   return (bool)((pgn & 0x20000) >> 17);
}

static inline j1939_pgn_t j1939_build_pgn_from_pdu(j1939_pdu_format_t pduf, j1939_pdu_specific_t pdus)
{
   return (j1939_pgn_t)MAKEWORD(pdus, pduf);
}

static inline j1939_arbitration_t j1939_build_message_id(bool DP,
                                                       bool EDP,
                                                       j1939_priority_t PR, j1939_pgn_t PGN,
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
 * @return Ture if PGN is in the list, false if not.
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
 * @param message Received message to process
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
                           j1939_pdu_format_t pduf, j1939_group_extension_t groupExtension);

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
                                        uint16_t dataLength, bool isExtendedMessage,
                                        const uint8_t *data);

extern const j1939_routing_callback_t J1939_App_RoutingTable[];

/**
 * Application specific initialization
 */
void j1939_app_init(void);

/**
 * Application specific periodic task
 */
void j1939_app_task(void);

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

#endif
