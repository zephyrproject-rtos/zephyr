/*
 * Copyright (c) 2026 Deere & Company
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _J1939TP_H_
#define _J1939TP_H_

#include <zephyr/canbus/j1939.h>

typedef enum j1939_tp_state_e
{
   j1939_tp_state_available,
   j1939_tp_state_waiting,
   j1939_tp_state_receiving,
   j1939_tp_state_sending,
   j1939_tp_state_holding
} J1939Tp_State_T;

typedef enum j1939_tp_message_e
{
   j1939_tp_message_accepted,
   j1939_tp_message_not_accepted
} J1939Tp_Message_T;

#define J1939TP_MAX_BYTES (1785)
#define J1939TP_MIN_BYTES (9)

/// Initialize the transport session handler
void j1939_tp_init(void);

#ifndef CONFIG_J1939TP_RECEIVE_DISABLED

/// @brief Updates times of transport receive objects since the last reception of the PGN they
/// represent and times between sends done by the transmitter.
/// @note Expected to be done at 5ms rate.
/// @param callPeriod Rate at which is this function is called.
void j1939_tp_update_receive_message_times(j1939_timer_t callPeriod);
#endif

/// @brief Updates times of transport transmit objects since the last send of the PGN they
/// represent and times between sends done by the transmitter.
/// @note Expected to be done at 5ms rate.
/// @param callPeriod Rate at which is this function is called.
void j1939_tp_update_send_message_times(j1939_timer_t callPeriod);

#ifndef CONFIG_J1939TP_RECEIVE_DISABLED
/// @brief Register a transport layer callback based on node and NPGN.
/// @param pgn PGN to register for the receive in the transport layer
/// @param node J1939 node that message should come in on
/// @param callback Callback function to call when matching message is received.
/// @return True if function was registered, false if not.
bool j1939_tp_register_message_callback(j1939_pgn_t pgn, j1939_node_t node,
                                             j1939_tp_callback_t callback);

/// @brief Adds a PGN to the registry.
/// @param pgn PGN to add to the registry. Unused bytes must be set to 0.
/// @param node J1939 node on which to process
/// @return True if successfully registered, false if not.
bool j1939_tp_register_extended_length_messages(j1939_pgn_t pgn, j1939_node_t node);
#endif

/// @brief Determines if a transmit connection already exists with a given source. A BAM'd receive
/// connection is considered separate from a RTS' d receive connection.
/// @param source Source address to check if we are already receiveing from
/// @param node J!939 node on which to process
/// @return true if a transmit connection exists with the source on the specified node false
/// otherwise
bool j1939_tp_transmit_session_exists(j1939_source_address_t source, j1939_node_t node);

/// @brief Attempts to allocate a TP buffer from the available pool.
/// @param size Number of bytes need to allocate
/// @return `NULL` if not buffers are available, otherwise a pointer into buffer space.
uint8_t *j1939_tp_get_buffer(j1939_counter_t size);

/// @brief Deallocates a buffer from usage. If pointer does not match then no buffers are freed.
/// @param buffer Pointer to the allocated buffer to deallocate.
void j1939_tp_free_buffer(uint8_t *buffer);

/// @brief Processes the TP CM CAN message from the driver and sends the data to the appropriate TP
/// function.
/// @param message Pointer to CAN message
/// @param node J1939 node on which to process the message
void j1939_tp_process_tpcm_message(const struct can_frame *message, j1939_node_t node);

/// @brief Attempts to find TX transport object by searching through the currently allocated TX
/// transport objects. When it finds one large enough to hold the data to send, it loads it up and
/// initializes it for transmission. `j1939_tp_update_send_message_times` will subsequently start actual
/// transmission of the message.
/// @param pgn PDN to transmit
/// @param destination Destination address of the message (255 (0xFF) for BAM messages)
/// @param dataLength Number of data bytes to transmit
/// @param data Pointer to the data bytes to transmit
/// @param node J1939 node on which to send the message
/// @return Result of message acceptance.
J1939Tp_Message_T j1939_tp_transmit_multi_packet(j1939_pgn_t pgn,
                                              j1939_destination_address_t destination,
                                              j1939_counter_t dataLength, uint8_t *data,
                                              j1939_node_t node);

#ifndef CONFIG_J1939TP_RECEIVE_DISABLED
/// @brief Processes incoming TP.DT messages
/// @param source Source address of the node transmitting the TP.DT message
/// @param data Pointer to the TP.DT message data bytes
/// @param destination Destination address of the TP.DT message
/// @param node J1939 node on which to process
void j1939_tp_process_dt(j1939_source_address_t source, const uint8_t *data,
                       j1939_destination_address_t destination, j1939_node_t node);
#endif

/// @brief Searches for a completed transport session buffer matches the specified PGN, source
/// address and node.
/// @note The caller of the function is responsible for calling `j1939_tp_free_completed_buffer`() with
/// the returned pointer when they are done using the buffer
/// @param pgn PGN to search against
/// @param source Source address to look for
/// @param node J1939 node
/// @param length Variable to fill in with the length of the received data
/// @return NULL if there is no match, or the pointer to the buffer if there is a match
uint8_t *j1939_tp_get_completed_session_buffer(j1939_pgn_t pgn, j1939_source_address_t source,
                                                  j1939_node_t node, j1939_counter_t *length);

/// @brief Frees the specified transport buffer retreived using `j1939_tp_get_completed_session_buffer`.
/// @param buffer Pointer to buffer to free
void j1939_tp_free_completed_buffer(uint8_t *buffer);

/// Access the largest available transport buffer size
/// @return Number of bytes of largest buffer
j1939_counter_t j1939_tp_get_max_available_buffer_size(void);

#ifndef CONFIG_J1939TP_RECEIVE_DISABLED
/// @brief Handles the data transfer transport messages
/// @param message Pointer to message to be processed
/// @return False. CAN layer will free message memory.
bool j1939_tp_data_transfer(const struct can_frame *message, j1939_node_t node);
#endif

/// @brief Handles the transport session connection management messages
/// @param message Pointer to message to be processed
/// @return False. CAN layer will free message memory.
bool j1939_tp_connection_management(const struct can_frame *message, j1939_node_t node);

#endif
