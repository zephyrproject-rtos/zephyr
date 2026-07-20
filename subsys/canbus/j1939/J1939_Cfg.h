/*
 * Copyright (c) 2026 Deere & Company
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _J1939_CFG_H_
#define _J1939_CFG_H_

#include <CriticalSectionManager.h>

/***************************************************************************************************
 *  Memory Access Buffer Testing
 **************************************************************************************************/

// If you want to enable a buffer for testing purposes when using J1939
// memory access, enable these defines.  Most projects should not need it.
// #define J1939MA_TESTING_BUFFER
// #define J1939MA_TESTING_BUFFER_SIZE  (0x40000)

/***************************************************************************************************
 *  Queue Handling
 **************************************************************************************************/

// Enable this define to have the J1939 layer handle the Can_Transmit_SendQueues() call in
// J1939_Task() If you do not enable this define, you must call Can_Transmit_SendQueues()
// periodically elsewhere
#define J1939_TRANSMIT_QUEUE_HANDLING

/// There is a routine in the low level drivers called CANPeriodicRoutine().  This function does
/// things like handle the CAN statistics, reset us from bus off, etc. Enable this define if you
/// want the J1939 layer to take care of calling this function.  You can also adjust the rate at
/// which it gets called by changing the J1939_PERIODIC_ROUTINE_MILLISECONDS define.
// #define J1939_PERIODIC_ROUTINE

#ifdef J1939_PERIODIC_ROUTINE
#define J1939_PERIODIC_ROUTINE_MS ((j1939_millisecond_t)10)
#endif

/***************************************************************************************************
 * System Management
 **************************************************************************************************/

/// Some projects may not want the J1939 layer to "eat" messages by returning
/// true from it's receive service callback.  This helps eliminate bugs in projects
/// that also register other receive callbacks in addition to the J1939
/// catch-all-messages callback.  If you are only using the J1939 layer, and are
/// not registering any other callbacks, you should comment out this define.
/// @note Required to run J1939 and UDS at the same time
#define J1939_NEVER_EATS_MESSAGES

/// There is a default handler for messages with a PF of 254 messages: J1939_App_Pf254Process(). It
/// will "eat" messages for DM1, DM2 and DM4s if these options are enabled.  If you would like these
/// messages to also pass through to the app specific 254 handler called
/// J1939_App_Pf254Process(), then enable this option
// #define J1939_PASSTHROUGH_FOR_254_MESSAGES

// Run nodes through an internal loopback
// #define J1939_LOOPBACK_ENABLE

/***************************************************************************************************
 *  Address Claim
 **************************************************************************************************/

/// Enable this define to control if our unit will respond to units that have not claimed their
/// address
#define J1939_IGNORE_REQUESTS_FROM_UNCLAIMED_ADDRESSES

/// Enable to record the names of the controller which have address claimed on the bus.
// #define J1939_RECORD_ADDRESS_CLAIMED_NAMES

/// Enable this define to enable arbitrary address capabilities.  You will need to implement
/// handling in CANJ1939AcApplication.c to configure this for your application
// #define J1939AC_SELF_CONFIGURABLE

#ifdef J1939AC_SELF_CONFIGURABLE
/// Starting source address when performing arbitrary address claim.
#define J1939AC_ARBITRARY_MIN_SOURCE_ADDRESS ((j1939_source_address_t)(0x87))

/// Last source address to attempt when performing arbitrary address claim.
#define J1939AC_ARBITRARY_MAX_SOURCE_ADDRESS ((j1939_source_address_t)(0xF8))

// Configure how many alternate address you wish to use when you lose arbitration
// We subtract one because we don't put the default address in the arbitrary list
#define J1939AC_ALT_ADDRESS_LIST_LENGTH                                                            \
   ((J1939AC_ARBITRARY_MAX_SOURCE_ADDRESS - J1939AC_ARBITRARY_MIN_SOURCE_ADDRESS) + 1)
#endif

/***************************************************************************************************
 *  PGN's
 **************************************************************************************************/

/// Enable to be able to receive PGN requests
// #define J1939_ENABLE_RECEIVED_PGN_SUPPORT

#ifdef J1939_ENABLE_RECEIVED_PGN_SUPPORT
/// Number of PGN's requests to support per address node
#define J1939_RECEIVED_PGN_LIST_MAX (10)
#endif

/***************************************************************************************************
 *  J1939 Diagnostics
 **************************************************************************************************/
/// Define the following if J1939 Diagnostics support is needed.
// #define J1939_DIAGNOSTICS

#ifdef J1939_DIAGNOSTICS
/// J1939 Receive DM1 messages from other modules
// #define J1939DM1_RECEIVE

#ifdef J1939DM1_RECEIVE
/// Maximum number of total DTC's that can be on the CAN bus for all units
/// @warning Must be <= 250
#define J1939DM1_MAX_DTCS (40)

/// How often to traverse the received DTC table looking for DTCs that have timed out
#define J1939DM1_DTC_UPDATE_MS ((j1939_millisecond_t)100)
#endif

/// J1939 Receive DM2 messages from other modules
// #define J1939DM2_RECEIVE

#ifdef J1939DM2_RECEIVE
/// This defines how long we will wait after sending a DM2 request to the global address before
/// we consider the unit to have timed out
#define J1939DM2_REQUEST_TIMEOUT_MS ((j1939_millisecond_t)(1000))
/// Maximum number of total DTC's that can be on the CAN bus for all units
/// @warning Must be <= 250
#define J1939DM2_MAX_DTCS (40)
#endif

/// J1939 Transmit DM1 messages due to internal faults
// #define J1939DM1_TRANSMIT

/// If your system does not want to send DM1 with transport, enable this define.  This is only
/// useful for projects that have only one DM1 active at a time(i.e. mostly boot blocks)
// #define J1939DM1_NO_TRANSMIT_TRANSPORT

/// J1939 Transmit DM2 messages when requested
// #define J1939DM2_TRANSMIT

/// J1939 DM3 request support
// #define J1939DM3_ENABLE

/// J1939 DM4 transmit message support
// #define J1939DM4_TRANSMIT

/// J1939 DM4 transmit message support
// #define J1939DM4_RECEIVE

#ifdef J1939DM4_RECEIVE
/// # of DM4 Freeze Frame DTCs that will be stored.
#define J1939DM4_MAX_RECEIVE_DTCS (20)
#endif

/// J1939 DM5 request support
// #define J1939DM5_ENABLE

#ifdef J1939DM5_ENABLE
// Look in the -73 spec for appropriate values.  The defaults indicate non supported
#define J1939DM5_OBD_COMPLIANCE_LEVEL (0x05)
#define J1939DM5_CONTINOUSLY_MONITORED_SYSTEMS_SUPPORT_STATUS (0x00)
#define J1939DM5_NON_CONTINOUSLY_MONITORED_SYSTEMS_SUPPORT (0x0000)
#define J1939DM5_NON_CONTINOUSLY_MONITORED_SYSTEMS_STATUS (0x0000)
#endif

/// J1939 DM11 request support
// #define J1939DM11_ENABLE

/// J1939 DM13 message support
// #define J1939DM13_ENABLE

#endif

/***************************************************************************************************
 *  J1939 Memory Access
 **************************************************************************************************/
// #define J1939_MEMORY_ACCESS

#ifdef J1939_MEMORY_ACCESS
/// Define the timeout periods for various situations.  These are not well defined by
/// the standard but the default values are typically sufficent.
#define J1939MA_SECURITY_TIMEOUT_MS ((j1939_millisecond_t)100)
#define J1939MA_BIN_DATA_RECEIVE_TIMEOUT_MS ((j1939_millisecond_t)3000)

/// Enable this define if you want to support directed spatial memory accesses.  By default we
/// only support direct memory access
// #define J1939MA_SUPPORT_DIRECTED_SPATIAL_REQUESTS

/// If you would rather return OPERATION FAILED when a request too large to handle is received
/// rather than negotiating down, enable this define
// #define J1939MA_FAIL_IF_REQUESTS_TOO_LARGE

/// Enable to allow Reads and Writes to be requested by us
#define J1939MA_TRANSMIT_READ_WRITE_REQUEST

#ifdef J1939MA_TRANSMIT_READ_WRITE_REQUEST
#define J1939MA_SEED_KEY_SECURITY_FOR_READ_WRITE_REQUEST
#endif

// Most projects should not need to change the priority of these messages
#define J1939MA_REQUEST_PRIORITY (J1939_Priority_6)
#define J1939MA_RESPONSE_PRIORITY (J1939_Priority_6)
#define J1939MA_BIN_DATA_TRANSFER_PRIORITY (J1939_Priority_6)
#endif


/***************************************************************************************************
 * Debug
 **************************************************************************************************/
// Enables timing analysis debugging features for the transport layer
//#define J1939_DEBUG
#endif
