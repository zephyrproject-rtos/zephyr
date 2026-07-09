/*
 * Copyright (c) 2026 Deere & Company
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _J1939TP_H_
#define _J1939TP_H_

#include <zephyr/canbus/j1939.h>
#include <J1939_Cfg.h>

typedef enum J1939Tp_State_E
{
   J1939Tp_State_Available,
   J1939Tp_State_Waiting,
   J1939Tp_State_Receiving,
   J1939Tp_State_Sending,
   J1939Tp_State_Holding
} J1939Tp_State_T;

typedef enum J1939Tp_Message_E
{
   J1939Tp_Message_Accepted,
   J1939Tp_Message_NotAccepted
} J1939Tp_Message_T;

#define J1939TP_MAX_BYTES (1785)
#define J1939TP_MIN_BYTES (9)

/// Initialize the transport session handler
void J1939Tp_Init(void);

#ifndef J1939TP_RECEIVE_DISABLED

/// @brief Updates times of transport receive objects since the last reception of the PGN they
/// represent and times between sends done by the transmitter.
/// @note Expected to be done at 5ms rate.
/// @param callPeriod Rate at which is this function is called.
void J1939Tp_UpdateReceiveMessageTimes(J1939_Timer_T callPeriod);
#endif

/// @brief Updates times of transport transmit objects since the last send of the PGN they
/// represent and times between sends done by the transmitter.
/// @note Expected to be done at 5ms rate.
/// @param callPeriod Rate at which is this function is called.
void J1939Tp_UpdateSendMessageTimes(J1939_Timer_T callPeriod);

#ifndef J1939TP_RECEIVE_DISABLED
/// @brief Register a transport layer callback based on node and NPGN.
/// @param pgn PGN to register for the receive in the transport layer
/// @param node J1939 node that message should come in on
/// @param callback Callback function to call when matching message is received.
/// @return True if function was registered, false if not.
bool J1939Tp_RegisterMessageCallback(J1939_Pgn_T pgn, J1939_Node_T node,
                                             J1939Tp_Callback_T callback);

/// @brief Adds a PGN to the registry.
/// @param pgn PGN to add to the registry. Unused bytes must be set to 0.
/// @param node J1939 node on which to process
/// @return True if successfully registered, false if not.
bool J1939Tp_RegisterExtendedLengthMessages(J1939_Pgn_T pgn, J1939_Node_T node);
#endif

/// @brief Determines if a transmit connection already exists with a given source. A BAM'd receive
/// connection is considered separate from a RTS' d receive connection.
/// @param source Source address to check if we are already receiveing from
/// @param node J!939 node on which to process
/// @return true if a transmit connection exists with the source on the specified node false
/// otherwise
bool J1939Tp_TransmitSessionExists(J1939_SourceAddress_T source, J1939_Node_T node);

/// @brief Attempts to allocate a TP buffer from the available pool.
/// @param size Number of bytes need to allocate
/// @return `NULL` if not buffers are available, otherwise a pointer into buffer space.
uint8_t *J1939Tp_GetBuffer(J1939_Counter_T size);

/// @brief Deallocates a buffer from usage. If pointer does not match then no buffers are freed.
/// @param buffer Pointer to the allocated buffer to deallocate.
void J1939Tp_FreeBuffer(uint8_t *buffer);

/// @brief Processes the TP CM CAN message from the driver and sends the data to the appropriate TP
/// function.
/// @param message Pointer to CAN message
/// @param node J1939 node on which to process the message
void J1939Tp_ProcessTpcmMessage(const struct can_frame *message, J1939_Node_T node);

/// @brief Attempts to find TX transport object by searching through the currently allocated TX
/// transport objects. When it finds one large enough to hold the data to send, it loads it up and
/// initializes it for transmission. `J1939Tp_UpdateSendMessageTimes` will subsequently start actual
/// transmission of the message.
/// @param pgn PDN to transmit
/// @param destination Destination address of the message (255 (0xFF) for BAM messages)
/// @param dataLength Number of data bytes to transmit
/// @param data Pointer to the data bytes to transmit
/// @param node J1939 node on which to send the message
/// @return Result of message acceptance.
J1939Tp_Message_T J1939Tp_TransmitMultiPacket(J1939_Pgn_T pgn,
                                              J1939_DestinationAddress_T destination,
                                              J1939_Counter_T dataLength, uint8_t *data,
                                              J1939_Node_T node);

#ifndef J1939TP_RECEIVE_DISABLED
/// @brief Processes incoming TP.DT messages
/// @param source Source address of the node transmitting the TP.DT message
/// @param data Pointer to the TP.DT message data bytes
/// @param destination Destination address of the TP.DT message
/// @param node J1939 node on which to process
void J1939Tp_ProcessDt(J1939_SourceAddress_T source, const uint8_t *data,
                       J1939_DestinationAddress_T destination, J1939_Node_T node);
#endif

/// @brief Searches for a completed transport session buffer matches the specified PGN, source
/// address and node.
/// @note The caller of the function is responsible for calling `J1939Tp_FreeCompletedBuffer`() with
/// the returned pointer when they are done using the buffer
/// @param pgn PGN to search against
/// @param source Source address to look for
/// @param node J1939 node
/// @param length Variable to fill in with the length of the received data
/// @return NULL if there is no match, or the pointer to the buffer if there is a match
uint8_t *J1939Tp_GetCompletedSessionBuffer(J1939_Pgn_T pgn, J1939_SourceAddress_T source,
                                                  J1939_Node_T node, J1939_Counter_T *length);

/// @brief Frees the specified transport buffer retreived using `J1939Tp_GetCompletedSessionBuffer`.
/// @param buffer Pointer to buffer to free
void J1939Tp_FreeCompletedBuffer(uint8_t *buffer);

/// Access the largest available transport buffer size
/// @return Number of bytes of largest buffer
J1939_Counter_T J1939Tp_GetMaxAvailableBufferSize(void);

#ifndef J1939TP_RECEIVE_DISABLED
/// @brief Handles the data transfer transport messages
/// @param message Pointer to message to be processed
/// @return False. CAN layer will free message memory.
bool J1939Tp_DataTransfer(const struct can_frame *message, J1939_Node_T node);
#endif

/// @brief Handles the transport session connection management messages
/// @param message Pointer to message to be processed
/// @return False. CAN layer will free message memory.
bool J1939Tp_ConnectionManagement(const struct can_frame *message, J1939_Node_T node);

#endif
