/*
 * Copyright (c) 2026 Caleb Perkinson
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _CANJ1939_H_
#define _CANJ1939_H_

#include <zephyr/drivers/can.h>
#include <zephyr/canbus/j1939_dt.h>

#define LOBYTE(w) ((uint8_t)((uint16_t)(w) & 0xFFU))
#define HIBYTE(w) ((uint8_t)(((uint16_t)(w) >> 8) & 0xFFU))
#define LOWORD(l) ((uint16_t)((uint32_t)(l) & 0xFFFFU))
#define HIWORD(l) ((uint16_t)(((uint32_t)(l) >> 16) & 0xFFFFU))
#define MAKEWORD(lo, hi) ((uint16_t)(((uint16_t)(uint8_t)(lo)) | ((uint16_t)(uint8_t)(hi) << 8)))
#define MAKEDWORD(lo, hi) ((uint32_t)(((uint32_t)(uint16_t)(lo)) | ((uint32_t)(uint16_t)(hi) << 16)))
#define ELEMENTS(a) (sizeof(a) / sizeof((a)[0]))

typedef uint8_t J1939_Address_T;

typedef J1939_Address_T J1939_SourceAddress_T;

typedef J1939_Address_T J1939_DestinationAddress_T;

typedef uint32_t J1939_MemoryAddress_T;

typedef uint32_t J1939_Pgn_T;

typedef uint8_t J1939_PduSpecific_T;

typedef uint8_t J1939_PduFormat_T;

typedef uint16_t J1939_SecuritySeed_T;

typedef uint16_t J1939_SecurityKey_T;

typedef uint32_t J1939_Timer_T;

typedef uint32_t J1939_Arbitration_T;

typedef uint32_t J1939_Counter_T;

typedef uint8_t J1939_ByteCounter_T;

typedef J1939_Counter_T J1939_DtcCounter_T;

typedef uint32_t J1939_Spn_T;

typedef uint8_t J1939_DtcConversionMethod_T;

typedef uint8_t J1939_GroupExtension_T;

//lint -strong(AcJdX, Uds_Millisecond_T)
typedef uint32_t J1939_Millisecond_T;

//lint -strong(AcJdX, Uds_Microsecond_T)
typedef uint32_t J1939_Microsecond_T;

// Failure mode identification codes as defined by J1939, appendix A, SAE J1939-71 Draft JUN98.
typedef enum J1939_Fmi_E
{
   J1939_Fmi_0,       // Valid, but above normal operating range.
   J1939_Fmi_1,       // Valid, but below normal operating range.
   J1939_Fmi_2,       // Data erratic, intermittent or incorrect.
   J1939_Fmi_3,       // Voltage above normal, or shorted to high source.
   J1939_Fmi_4,       // Voltage below normal, or shorted to low source.
   J1939_Fmi_5,       // Current below normal, or open circuit.
   J1939_Fmi_6,       // Current above normal, or grounded circuit.
   J1939_Fmi_7,       // Mechanical system not responding or out of adjustment.
   J1939_Fmi_8,       // Abnormal frequency, pulse width or period.
   J1939_Fmi_9,       // Abnormal update rate.
   J1939_Fmi_10,      // Abnormal rate of change.
   J1939_Fmi_11,      // Failure code not identifiable.
   J1939_Fmi_12,      // Bad intelligent device or component.
   J1939_Fmi_13,      // Out of calibration.
   J1939_Fmi_14,      // Special Instructions.
                      // FMI_15 to FMI_30 Not defined.
   J1939_Fmi_31 = 31, // Not available
   J1939_Fmi_255 = 255
} J1939_Fmi_T;

typedef uint32_t J1939_SpnFmi_T;

// These types are specifically for DM4, which is why they are prefixed as such
typedef uint8_t J1939Dm4_FreezeFrame_EngineTorqueMode_T;
typedef uint8_t J1939Dm4_FreezeFrame_Boost_T;
typedef uint16_t J1939Dm4_FreezeFrame_EngineSpeed_T;
typedef uint8_t J1939Dm4_FreezeFrame_EngineLoadPercent_T;
typedef uint8_t J1939Dm4_FreezeFrame_EngineCoolantTemp_T;
typedef uint16_t J1939Dm4_FreezeFrame_VehicleSpeed_T;

#define J1939_NULL_ADDRESS ((J1939_Address_T)0xFE)
#define J1939_GLOBAL_ADDRESS ((J1939_Address_T)0xFF)

#define J1939_BYTE_UNAVAILABLE (0xFF)

// PDU Format Byte definitions
#define J1939_PDUF_232 ((J1939_PduFormat_T)0xE8)
#define J1939_PDUF_235 ((J1939_PduFormat_T)0xEB)
#define J1939_PDUF_236 ((J1939_PduFormat_T)0xEC)
#define J1939_PDUF_240 ((J1939_PduFormat_T)0xF0)

// Diagnostic PGN's
#define J1939_DM1_PGN ((J1939_Pgn_T)0xFECA)
#define J1939_DM2_PGN ((J1939_Pgn_T)0xFECB)
#define J1939_DM3_PGN ((J1939_Pgn_T)0xFECC)
#define J1939_DM4_PGN ((J1939_Pgn_T)0xFECD)
#define J1939_DM5_PGN ((J1939_Pgn_T)0xFECE)
#define J1939_DM11_PGN ((J1939_Pgn_T)0xFED3)

// Other J1939 Protocol PGN's
#define J1939_ADDRESS_CLAIMED_PGN ((J1939_Pgn_T)0xEE00)
#define J1939_REQUEST_PGN ((J1939_Pgn_T)0xEA00)
#define J1939_SOFTWARE_ID_PGN ((J1939_Pgn_T)0xFEDA)
#define J1939_ECU_ID_INFO_PGN ((J1939_Pgn_T)0xFDC5)

// PS definitions
#define J1939_ACTIVE_DTC_PS ((J1939_PduSpecific_T)202)      /* DM1 message */
#define J1939_FREEZE_FRAME_PS ((J1939_PduSpecific_T)205)    /* DM4 message */
#define J1939_PREVIOUS_ACTIVE_PS ((J1939_PduSpecific_T)203) /* DM2 message */

// PF definitions
#define J1939_PGN_ACK_PF J1939_PDUF_232
#define J1939_TP_DATA_TRANSFER_PF J1939_PDUF_235
#define J1939_TP_CONN_MANAGEMENT_PF J1939_PDUF_236

// Declarations for Priority Bits in Identifiers
typedef enum J1939_Priority_E
{
   J1939_Priority_0,
   J1939_Priority_1,
   J1939_Priority_2,
   J1939_Priority_3,
   J1939_Priority_4,
   J1939_Priority_5,
   J1939_Priority_6,
   J1939_Priority_7,
} J1939_Priority_T;

typedef bool (*J1939_RoutingCallback_T)(const struct can_frame *message, J1939_Node_T node);

typedef enum J1939_Response_E
{
   J1939_Response_Ack,
   J1939_Response_Nack
} J1939_Response_T;

typedef uint32_t J1939_IdNumber_T;
typedef uint16_t J1939_ManufacturerCode_T;
typedef uint8_t J1939_EcuInstance_T;
typedef uint8_t J1939_FunctionInstance_T;
typedef uint8_t J1939_Function_T;
typedef uint8_t J1939_VehicleSystem_T;
typedef uint8_t J1939_VehicleSystemInstance_T;
typedef uint8_t J1939_IndustryGroup_T;
typedef uint8_t J1939_Reserved_T;

// Structure for address claimed message information
typedef struct J1939_Name_S
{
   J1939_IdNumber_T idNumber;
   J1939_ManufacturerCode_T mfgCode;
   J1939_EcuInstance_T ecuInstance;
   J1939_FunctionInstance_T functionInstance;
   J1939_Function_T function;
   J1939_VehicleSystem_T vehicleSystem;
   J1939_VehicleSystemInstance_T vehicleSystemInstance;
   J1939_IndustryGroup_T industryGroup;
   bool isSelfConfig;
   J1939_Reserved_T reservedBit;
} J1939_Name_T;

/**************************************************************************************************/
// Get the priority from the arbitration register
static inline J1939_Priority_T J1939_GetPriority(J1939_Arbitration_T arbitration)
{
   return (J1939_Priority_T)((arbitration >> 26) & 0x07);
}

/**************************************************************************************************/
// Gets the PDU Format byte from the arbitration register
static inline J1939_PduFormat_T J1939_GetPduFormat(J1939_Arbitration_T arbitration)
{
   return (J1939_PduFormat_T)(LOBYTE(HIWORD(arbitration)));
}

/**************************************************************************************************/
// Gets the PDU Specific byte from the arbitration register
static inline J1939_PduSpecific_T J1939_GetPduSpecific(J1939_Arbitration_T arbitration)
{
   return (J1939_PduSpecific_T)(HIBYTE(LOWORD(arbitration)));
}

/**************************************************************************************************/
// Gets the Source Address byte from the arbitration register
static inline J1939_SourceAddress_T J1939_GetSourceAddress(J1939_Arbitration_T arbitration)
{
   return ((J1939_SourceAddress_T)LOBYTE(LOWORD(arbitration)));
}

/**************************************************************************************************/
// Get the Data page from the arbitration register
static inline bool J1939_GetDataPage(J1939_Arbitration_T arbitration)
{
   return (bool)(LOBYTE(HIWORD(arbitration)) & 0x01);
}

/**************************************************************************************************/
static inline bool J1939_GetDataPageFromPgn(J1939_Pgn_T pgn)
{
   return (bool)((pgn & 0x10000) >> 16);
}

/**************************************************************************************************/
static inline bool J1939_GetExtendedDataPageFromPgn(J1939_Pgn_T pgn)
{
   return (bool)((pgn & 0x20000) >> 17);
}

/**************************************************************************************************/
static inline J1939_Pgn_T J1939_BuildPgnFromPdu(J1939_PduFormat_T pduf, J1939_PduSpecific_T pdus)
{
   return (J1939_Pgn_T)MAKEWORD(pdus, pduf);
}

/**************************************************************************************************/
static inline J1939_Arbitration_T J1939_BuildMessageId(bool DP,
                                                       bool EDP,
                                                       J1939_Priority_T PR, J1939_Pgn_T PGN,
                                                       J1939_SourceAddress_T SA)
{
   return ((((uint32_t)((uint8_t)(PR) & (uint8_t)0x07)) << 26) |
           (((uint32_t)((uint16_t)(PGN)&0xFFFF)) << 8) |
           ((uint32_t)((uint8_t)(SA) & (uint8_t)0xFF)) |
           (((uint32_t)((uint8_t)(DP) & (uint8_t)0x01)) << 24) |
           (((uint32_t)((uint8_t)(EDP) & (uint8_t)0x01)) << 25));
}

/// Initialize J1939 routines
void J1939_Init(void);

/// Periodic J1939 message processing routine.  This routine updates the various features that are
/// configured(transport, address claim, memory access, etc).  It should be called periodically
void J1939_Task(void);

/// This function searches the PGN request list to find out if a request for the specified
/// Parameter Group Number has been requested by a module on the CAN bus.
/// @param pgn Parameter Group Number of Request.
/// @param source Pointer to application level variable where the source address of the requester
/// gets stored, if a PGN was requested.
/// @param node Node to look at
/// @return True If the PGN was requested by another module on the CAN bus. false If the PGN was not
/// requested.
bool J1939_IsPgnRequested(J1939_Pgn_T pgn, J1939_SourceAddress_T *source,
                                  J1939_Node_T node);

/// @brief Set a PGN request within the PGN request list.
/// @param pgn PGN to set request
/// @param source Source address requesting the PGN
/// @param node J1939 node to flag
/// @return Ture if PGN is in the list, false if not.
bool J1939_FlagPgnRequest(J1939_Pgn_T pgn, J1939_SourceAddress_T source, J1939_Node_T node);

/// @brief Send an ACK/NACK to another J1939 node
/// @param pgn PGN that is to be ACK'd or NACK'd
/// @param control Response of ACK or NACK
/// @param destination Source address of the response
/// @param node J1939 node to send response from
void J1939_Acknowledge(J1939_Pgn_T pgn, J1939_Response_T control,
                       J1939_DestinationAddress_T destination, J1939_Node_T node);

/// @brief Send a PGN request to the specified node.
/// @param pgn PGN request to send
/// @param destination Source address of destination unit
/// @param node J1939 node to send request from
/// @return True if request was successfully sent, false if not.
bool J1939_TransmitPgnRequest(J1939_Pgn_T pgn, J1939_DestinationAddress_T destination,
                                      J1939_Node_T node);

/// @brief Add a PGN to the list for request processing.
/// @param pgn PGN of requests to look for
/// @param node J1939 node to watch
/// @return True if PGN was added to list, false if not.
bool J1939_RegisterRequestPgn(J1939_Pgn_T pgn, J1939_Node_T node);

/// @brief Sends out the data under the PGN specified. The use of transport protocol vs a std CAN
/// message is handled within this function.
/// @note The destination address is only used if the message PF is less than 240 or if the message
/// needs the transport layer (more than 8 bytes.) Setting the destination address to 255 specifies
/// a BAM transport message.  Other addresses specify the use of connection management for the
/// transport layer.
/// @param priority The Priority of the message
/// @param pgn The PGN to send the data out under
/// @param destination Destination address of a TP message
/// @param data Pointer to the data to send
/// @param count Number of bytes of the data to send
/// @param node Node which to send message out on
/// @return True if message was sent, false if not queued.
bool J1939_TransmitPgn(J1939_Priority_T priority, J1939_Pgn_T pgn,
                               J1939_DestinationAddress_T destination, uint8_t *data,
                               J1939_Counter_T count, J1939_Node_T node);

/// @brief Check if provided PGN is valid
/// @param pgn PGN to check
/// @return True if valid, false if not.
bool J1939_IsPgnValid(J1939_Pgn_T pgn);

#ifdef J1939_ENABLE_RECEIVED_PGN_SUPPORT
typedef bool (*J1939_ReceivedPgnCallback_T)(const struct can_frame *message);

/// @brief Register a handler for the specified PGN on the specified J1939 address node.
/// @note These PGNs are handled AFTER the PF routing table .If the routing table callback indicates
/// that it handles a message it will never be handled by your callback.Basically, if it goes to
/// J1939_DefaultRoute(), then your callback will be able to handle it.
/// @warning This only handles single message PGNs. If you wish to handle transport message PGNs,
/// you will need to register a PGN callback with the transport layer using the
/// J1939Tp_RegisterMessageCallback() function instead.
/// @param pgn PGN to handle
/// @param handler Callback function to handle the received PGN, must not be NULL
/// @param node J1939 node the message is received on
/// @return True on success, false if not.
bool J1939_RegisterReceivePgnCallback(J1939_Pgn_T pgn, J1939_ReceivedPgnCallback_T handler,
                                              J1939_Node_T node);
#endif

/// @brief Default routing table path
/// @param message Received message to process
/// @param node J1939 node the message is received on
/// @return False
bool J1939_DefaultRoute(const struct can_frame *constmessage, J1939_Node_T node);

/// @brief Routing table handler for PDUF 254 (0xFE)
/// @param message Received message to process
/// @param node J1939 node the message is received on
/// @return False
bool J1939_PduF254Process(const struct can_frame *message, J1939_Node_T node);

/// @brief Routing table handler for request PGN (0xEA)
/// @param message Received message to process
/// @param node J1939 node the message is received on
/// @return False
bool J1939_RequestPgn(const struct can_frame *message, J1939_Node_T node);

/// @brief Extracts the PGN from a message ID
/// @param messageId Message ID to parse
/// @return PGN in Message ID
J1939_Pgn_T J1939_GetPgn(J1939_Arbitration_T messageId);

/// @brief Build up a PGN in the proper format
/// @param extendedDataPage Value to set the extended data page to (zero or one)
/// @param dataPage Value to set the data page to (zero or one)
/// @param pduf PDU Format field
/// @param groupExtension Group extension field
/// @return Formatted PGN
J1939_Pgn_T J1939_BuildPgn(bool extendedDataPage, bool dataPage,
                           J1939_PduFormat_T pduf, J1939_GroupExtension_T groupExtension);

/// Enables the transmission of messages from a given J1939 address node
/// @param[in] node J1939 address node to enable messages
void J1939_EnableVirtualModeTransmit(J1939_Node_T node);

/// Disables the transmission of messages from a given J1939 address node
/// @param[in] node J1939 address node to disable messages
void J1939_DisableVirtualModeTransmit(J1939_Node_T node);

/// Check if the specified J1939 address node's transmit queue is enabled
/// @param[in] node J1939 address node to check
/// @return True if enabled, false otherwise
bool J1939_IsVirtualNodeTransmitEnabled(J1939_Node_T node);

/// Process the Build and queue of CAN message for transmit for a J1939 address node
/// @param[in] node Node to transmit on
/// @param[in] arbitration ID to transmit(right justified)
/// @param[in] dataLength length to transmit
/// @param[in] isExtendedMessage true for 29 bit messages, false for 11 bit messages
/// @param[in] data Data to send
/// @return true if the message was queued for transmit, false otherwise
bool J1939_BuildAndQueueMessage(J1939_Node_T node, J1939_Arbitration_T arbitration,
                                        uint16_t dataLength, bool isExtendedMessage,
                                        const uint8_t *data);

extern const J1939_RoutingCallback_T J1939_App_RoutingTable[];

/// Application specific initialization
void J1939_App_Init(void);

/// Application specific periodic task
void J1939_App_Task(void);

/// Handler for messages with a PF of 254.  This is one of the most common PFs we handle but the
/// there are some messages the layer handles for you.  This is done prior to calling this function.
/// @param message The message to process
/// @param node J1939 node the message is received on
/// @return True if successful, false otherwise
bool J1939_App_Pf254Process(const struct can_frame *message, J1939_Node_T node);

/// @brief Send out the ECU ID PGN
/// @param destination  Destination address to send to
/// @param node J1939 node to transmit on
void J1939_App_TransmitEcuId(J1939_DestinationAddress_T destination, J1939_Node_T node);

/// @brief Send out the software ID PGN
/// @param destination  Destination address to send to
/// @param node J1939 node to transmit on
void J1939_App_TransmitSoftwareId(J1939_DestinationAddress_T destination, J1939_Node_T node);

/// Get the J1939 address node index based on the PDU specific field in J1939Nodee message and
/// the physical node associated to it.
/// @param[in] address
/// @return J1939 address node index
J1939_Node_T J1939_App_GetJ1939NodeFromSourceAddress(J1939_SourceAddress_T address);

/// @brief Get the J1939 for a given source address
/// @param address Source address to match to J1939 node
/// @return J1939 node that source address belongs
static inline J1939_Node_T J1939_GetJ1939NodeFromSourceAddress(J1939_SourceAddress_T address)
{
   return J1939_App_GetJ1939NodeFromSourceAddress(address);
}

// We include this file here because it depends on some of the defines at the top of this file.
// #include "J1939ConfigurationErrorChecking.h"

#endif
