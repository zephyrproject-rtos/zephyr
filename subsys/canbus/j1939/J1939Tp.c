/*
 * Copyright (c) 2026 Deere & Company
 * SPDX-License-Identifier: Apache-2.0
 */
#include "J1939Tp.h"
#include <zephyr/canbus/j1939.h>
#include "J1939Ac.h"

/*==========================================================================
 * Misc. defines from the J1939 standard only used inside this module
 *========================================================================*/
// See J1939/21 Figure 10-Format of Messages for Transport Protocol
#define J1939TP_CM_RTS (16)
#define J1939TP_CM_CTS (17)
#define J1939TP_CM_EOMACK (19)
#define J1939TP_CM_ERTS (20)
#define J1939TP_CM_BAM (32)
#define J1939TP_CONNECTION_ABORT (255)

// Define timeouts in milliseconds. From the J1939 standard
#define J1939TP_T1_TIMEOUT (750)
#define J1939TP_T2_TIMEOUT (1250)
#define J1939TP_T3_TIMEOUT (1250)
#define J1939TP_T4_TIMEOUT (1050)

// Transport protocol times in milliseconds.
#define J1939TP_BAM_TIME (50)

// After this was implemented it was discovered that some projects needed to adjust the minimum
// time between packets.  We therefore created a new define in CANJ1939Config.h  to allow
// adjustments.  For backwards compatibility reasons, we used the #ifdef below
#ifndef CONFIG_J1939TP_MILLISECONDS_BETWEEN_PACKET_GROUPS
#define J1939TP_PACKET_TIME (5)
#else
#define J1939TP_PACKET_TIME CONFIG_J1939TP_MILLISECONDS_BETWEEN_PACKET_GROUPS
#endif

// The following ifdefs are in for backwards compatibility reasons.  They allowed us to rename some
// defines to more appropriate names without having to go modify CANJ1939Config.h for all existing
// projects
#ifndef CONFIG_J1939TP_MAX_MESSAGES_QUEUED_PER_SESSION
#define CONFIG_J1939TP_MAX_MESSAGES_QUEUED_PER_SESSION 1
#endif

#ifndef J1939TP_MAX_TOTAL_MESSAGES_QUEUED_FOR_ALL_SESSIONS
#define J1939TP_MAX_TOTAL_MESSAGES_QUEUED_FOR_ALL_SESSIONS 255
#endif

#ifndef CONFIG_J1939TP_NUM_TRANSMIT_SESSIONS
#define CONFIG_J1939TP_NUM_TRANSMIT_SESSIONS NUMBER_OF_TP_TX_MSGS
#endif

#ifndef CONFIG_J1939TP_NUMBER_OF_TP_RX_SESSIONS
#define CONFIG_J1939TP_NUMBER_OF_TP_RX_SESSIONS NUMBER_OF_TP_RX_MSGS
#endif

extern struct j1939_dt_node_cfg* j1939_nodes[];

// This struct is used to look for buffers that aren't in use and are large
// enough to hold the incoming/outgoing data.
typedef struct j1939_tp_buffer_s
{
   uint8_t *dataArea;
   j1939_counter_t size;
   bool inUse;
   //lint -esym(754, reserved) Suppress not referenced warning
   // TODO Just get rid of this
   j1939_reserved_t reserved[2]; // Uneven boundary -- currently wasted space.
#ifdef J1939_DEBUG
   j1939_counter_t maxSize;
   j1939_counter_t minSize;
   j1939_counter_t totalSize;
   j1939_counter_t allocations;
#endif
} j1939_tp_buffer_t;

#ifndef CONFIG_CONFIG_J1939TP_RECEIVE_DISABLED

typedef struct j1939_tp_receive_message_info_s
{
   j1939_source_address_t source;               // Source Address of Transmitter
   J1939Tp_State_T state;                      // State given by TP_RX_STATE
   uint8_t lastReceivedPacket; // Current Packet Sequence Number
   uint8_t lastExpectedPacket; // The last expected Packet Sequence Number
   uint8_t packetCountDown; // Countdown on the maximum number of packets we can request at a time
   uint8_t maxRequestedPackets;           // Maximum number of packets we can request at a time
   j1939_node_t node;                 // CAN node which is RXing this message.
   bool isBamMessage;         // Whether the message is BAM or not
   j1939_counter_t sizeMessage;       // Size of message
   j1939_counter_t currentByteOffset; // Currently received byte, used as offset into DataObject
   j1939_timer_t timeFromLastReceivedPacket; // Used to determine failures on tx'r end.
   j1939_timer_t timeFromLastCtsSent;        // Used to prevent a CTS = 0 timeout
   j1939_counter_t dataObjectSize;           // Info on the size of the object rx'ing data
   uint8_t *incomingDataObject;       // Pointer to the area to store rx'd data
   j1939_pgn_t pgn;                          // PGN currently being received
} j1939_tp_receive_message_info_t;              // Transport Protocol Receive Message Information
#endif

typedef struct j1939_tp_transmit_message_info_s
{
   j1939_destination_address_t destination; // Address to send message to (global = BAM message)
   J1939Tp_State_T state;                  // State given by TP_TX_STATE
   j1939_node_t node;                      // CAN node which to send this message out on.
   uint8_t totalPackets;   // Total packets to send
   uint8_t currentPacketSequenceNum; // Current packet sequence number
   uint8_t nextRequestedPacket;      // Next requested packet # as given by CTS
   uint8_t highestPacketSequenceNum; // The highest packet sequence # that we allow to be sent
   uint8_t lowestAblePacketSequenceNum; // The lowest allow packet number that is
                                        // allowed on a retransmit
   uint8_t numPacketsLeftCtsAllows;           // The number of packets left to send from given CTS
   j1939_timer_t elapsedTimeFromLastCts;  // counter from last reception of CTS
   j1939_timer_t elapsedTimeFromLastTpDt; // counter from last TP.DT transmission
   j1939_pgn_t pgn;                       // PGN associated with data in dataRegion
   uint8_t *currentDataLocation;   // pointer into dataRegion
   uint8_t *dataRegion;            // pointer to data to transmit using TP
   j1939_counter_t sizeDataRegion;        // size of pbyDataRegion
   j1939_counter_t totalBytesToTransmit;  // Total number of bytes to transmit
   bool isWaitingBetweenCts; // Describes if this object is waiting for the first CTS after
                                     // RTS or TP.DTs, or if waiting between successive CTS's
   bool isWaitingForEomAck;  // Is waiting for end of message acknowledge
} j1939_tp_transmit_message_info_t;

typedef enum J1939Tp_AbortCode_E
{
   //   TP_ABORT_CANNOT_SUPPORT_CONNECT = 1,
   //   TP_ABORT_SYSTEM_RESOURCES_NEEDED_ELSEWHERE = 2,
   J1939TP_AbortCode_Timeout = 3,
   J1939TP_AbortCode_Unknown = 255
} j1939_tp_abort_code_t;

#ifdef J1939_DEBUG

typedef struct J1939Tp_DebugMetrics_S
{
   j1939_counter_t numTenMsTicks;
   j1939_counter_t numSmallBuffersUsed;
   j1939_counter_t numMedBuffersUsed;
   j1939_counter_t numLargeBuffersUsed;
   j1939_counter_t numMaxBuffersUsed;
   j1939_counter_t numFilterAcceptedMsgs;
   j1939_counter_t numResourceAcceptedMsgs;
   j1939_counter_t sizeOfResourceAcceptedMessages;
   j1939_counter_t numResourceRejectedMsgs;
   j1939_counter_t numFilteredOutMsgs;
   j1939_counter_t maxNumSmallBuffersUsed;
   j1939_counter_t maxNumMedBuffersUsed;
   j1939_counter_t maxNumLargeBuffersUsed;
   j1939_counter_t maxNumMaxBuffersUsed;
} J1939Tp_DebugMetrics_T;

J1939Tp_DebugMetrics_T J1939Tp_DebugMetrics;

#endif

// Storage buffers for transport protocol objects.
// There is support for 4 different sized buffers - small, medium, large, and max (extra
// extra EXTRA large).  They are not defined if the number of buffers is set to zero.
#if (CONFIG_J1939TP_NUM_SMALL_BUFFERS > 0)
static uint8_t j1939_tp_small_buffer[CONFIG_J1939TP_NUM_SMALL_BUFFERS][CONFIG_J1939TP_BUFFER_SMALL_SIZE];
#endif

#if (CONFIG_J1939TP_NUM_MEDIUM_BUFFERS > 0)
static uint8_t j1939_tp_medium_buffer[CONFIG_J1939TP_NUM_MEDIUM_BUFFERS][CONFIG_J1939TP_BUFFER_MEDIUM_SIZE];
#endif

#if (CONFIG_J1939TP_NUM_LARGE_BUFFERS > 0)
static uint8_t j1939_tp_large_buffer[CONFIG_J1939TP_NUM_LARGE_BUFFERS][CONFIG_J1939TP_BUFFER_LARGE_SIZE];
#endif

#if (CONFIG_J1939TP_NUM_MAX_BUFFERS > 0)
static uint8_t j1939_tp_max_buffer[CONFIG_J1939TP_NUM_MAX_BUFFERS][CONFIG_J1939TP_BUFFER_MAX_SIZE];
#endif

static j1939_tp_buffer_t j1939_tp_all_buffers[CONFIG_J1939TP_NUM_ALL_BUFFERS];

static j1939_tp_transmit_message_info_t j1939_tp_transmit_message_list[CONFIG_J1939TP_NUM_TRANSMIT_SESSIONS];

#ifndef CONFIG_CONFIG_J1939TP_RECEIVE_DISABLED
static j1939_tp_receive_message_info_t
    J1939Tp_ReceiveMessageList[CONFIG_J1939TP_NUMBER_OF_TP_RX_SESSIONS]; // contains info on
                                                                  // incoming messages

static j1939_counter_t j1939_tp_num_open_receive_connections;
static j1939_counter_t j1939_tp_num_holding_messages;
#endif

static j1939_byte_counter_t j1939_tp_num_open_transmit_connections;

/// @brief Attempt to allocate buffer large enough to hold the passed size
/// @param sizeMessage Size of message bytes needed
/// @param buffer Pointer to update with allocated buffer pointer
/// @param sizeBuffer Pointer to update with size of allocated buffer
/// @return True on success, false if not
static bool j1939_tp_buffer_allocate(j1939_counter_t sizeMessage, uint8_t **buffer,
                                   j1939_counter_t *sizeBuffer);

/// @brief Buils and sends a connector abort message on the requested node
/// @param abortCode Abort code
/// @param pgn PGN of message to abort
/// @param source Source address of the unit to receive the abort message
/// @param node J1939 on which to send the abort
static void j1939_tp_transmit_connection_abort(j1939_tp_abort_code_t abortCode, j1939_pgn_t pgn,
                                            j1939_source_address_t source, j1939_node_t node);
#ifndef CONFIG_J1939TP_RECEIVE_DISABLED
/// @brief Checks if PGN is registered
/// @param pgn PGN to check
/// @param node J1939 node which to check
/// @return True if PGN is registered, false if not
static bool j1939_tp_is_pgn_supported(j1939_pgn_t pgn, j1939_node_t node);

/// @brief Register routing PGN to proper callback
/// @param pgn PGN to register
/// @param data Received data
/// @param length Number of bytes in `data`
/// @param source Source of message sender
/// @param node J1939 node message was received on
/// @return True if callback was called, false if not.
static bool j1939_tp_register_user_callback(j1939_pgn_t pgn, uint8_t *data,
                                         j1939_counter_t length,
                                         j1939_source_address_t source, j1939_node_t node);

/// @brief Cancels existing receive session and frees he associated receive buffer
/// @param index Index in to receive list
/// @param source Source address of sender
/// @param node J!939 node message was received on
/// @param isBam True if BAM message, false if not
/// @return True if session was cancelled, false if not.
static bool j1939_tp_cancel_old_receive_sessions(j1939_counter_t index,
                                             j1939_source_address_t source,
                                             j1939_node_t node, bool isBam);
#endif

/// @brief Processes CTS messages received by the RHCP in response to RTS messages sent.
/// @param source Source address of unit transmitting the CTS
/// @param data CTS message data
/// @param node J1939 node message was received on
static void j1939_tp_process_cts(j1939_source_address_t source, const uint8_t *data,
                               j1939_node_t node);

/// @brief Processes incoming End of Message Acknowledgements
/// @param source Source address of the incoming EOMAck message
/// @param data Data bytes of the message
/// @param node J1939 node message was received on
static void j1939_tp_process_eom_ack(j1939_source_address_t source, const uint8_t *data,
                                  j1939_node_t node);

/// @brief Processes incoming Connection Abort messages received by the RHCP
/// @param source Source address of the node transmitting the connection abort
/// @param data Pointer to the connection abort message
/// @param node J1939 node message was received on
static void j1939_tp_process_abort(j1939_source_address_t source, const uint8_t *data,
                                 j1939_node_t node);
#ifndef CONFIG_J1939TP_RECEIVE_DISABLED
/// @brief Processes BAM and RTS messages received by the RHCP. Determines if the message is a BAM
/// or RTS and then checks registered PGNs.
/// @note Rejects message if its a duplicate of a current session and determines if there is a large
/// enough object for available reception.
/// @param control PDU Format of message (TP.CM_BAM or TP.CM_RTS)
/// @param source Source address of the node sending the message
/// @param data Data bytes for message
/// @param node J1939 on which to process
static void j1939_tp_process_bam_rts(j1939_pdu_format_t control, j1939_source_address_t source,
                                  const uint8_t *data, j1939_node_t node);

/// @brief Generates a CTS message to the specified `destination`.
/// @param destination Source address of where message should be sent.
/// @param index Index in message list for the session sending the CTS
/// @param packetNum Next expected packet number.
static void j1939_tp_transmit_cts(j1939_destination_address_t destination, j1939_counter_t index,
                                uint8_t packetNum);
#endif

/// Macro to simplify handle initializing a buffer
#define j1939_tp_buffer_init(buffer, numBuffers, bufferSize)                                        \
   {                                                                                               \
      for (j1939_counter_t i = 0; i < numBuffers; i++)                                             \
      {                                                                                            \
         for (j1939_counter_t j = 0; j < bufferSize; j++)                                          \
         {                                                                                         \
            buffer[i][j] = 0;                                                                      \
         }                                                                                         \
      }                                                                                            \
   }

#ifdef J1939_DEBUG
/// @brief Helps analyze network traffic the RHCP sees.
/// @note This information can be used to tailor the number of buffers being used.
static void j1939_tp_timing_analysis(void);
#endif

/**************************************************************************************************/
void j1939_tp_init(void)
{
   j1939_counter_t allIndex = 0;

#ifndef CONFIG_J1939TP_RECEIVE_DISABLED
   for (uint8_t node_index = 0; node_index < CONFIG_J1939_NODES_COUNT; node_index++)
   {
      j1939_node_t node = j1939_nodes[node_index];
      node->j1939_tp_register_pgn_index = 0;

      for (uint8_t receivePgn = 0; receivePgn < CONFIG_J1939TP_NUM_ALLOWED_RECEIVE_PGN; receivePgn++)
      {
         node->j1939_tp_register_pgn_list[receivePgn].pgn = UINT32_MAX;
         node->j1939_tp_register_pgn_list[receivePgn].callback = NULL;
      }
   }
#endif

   // Leave the order of assignments in the following for-loops  as they are.  The algorithm to
   // locate a free buffer is first-fit.
#if (CONFIG_J1939TP_NUM_SMALL_BUFFERS > 0)

   j1939_tp_buffer_init(j1939_tp_small_buffer, CONFIG_J1939TP_NUM_SMALL_BUFFERS, CONFIG_J1939TP_BUFFER_SMALL_SIZE);
   for (j1939_counter_t i = 0; i < CONFIG_J1939TP_NUM_SMALL_BUFFERS; i++)
   {
      j1939_tp_all_buffers[allIndex].dataArea = j1939_tp_small_buffer[i];
      j1939_tp_all_buffers[allIndex].size = CONFIG_J1939TP_BUFFER_SMALL_SIZE;
      j1939_tp_all_buffers[allIndex++].inUse = false;
   }
#endif

#if (CONFIG_J1939TP_NUM_MEDIUM_BUFFERS > 0)

   //lint !e2454
   j1939_tp_buffer_init(j1939_tp_medium_buffer, CONFIG_J1939TP_NUM_MEDIUM_BUFFERS,
                       CONFIG_J1939TP_BUFFER_MEDIUM_SIZE);

   for (j1939_counter_t i = 0; i < CONFIG_J1939TP_NUM_MEDIUM_BUFFERS; i++)
   {
      j1939_tp_all_buffers[allIndex].dataArea = j1939_tp_medium_buffer[i];
      j1939_tp_all_buffers[allIndex].size = CONFIG_J1939TP_BUFFER_MEDIUM_SIZE;
      j1939_tp_all_buffers[allIndex++].inUse = false;
   }
#endif

#if (CONFIG_J1939TP_NUM_LARGE_BUFFERS > 0)

   j1939_tp_buffer_init(j1939_tp_large_buffer, CONFIG_J1939TP_NUM_LARGE_BUFFERS, CONFIG_J1939TP_BUFFER_LARGE_SIZE);

   for (j1939_counter_t i = 0; i < CONFIG_J1939TP_NUM_LARGE_BUFFERS; i++)
   {
      j1939_tp_all_buffers[allIndex].dataArea = j1939_tp_large_buffer[i];
      j1939_tp_all_buffers[allIndex].size = CONFIG_J1939TP_BUFFER_LARGE_SIZE;
      j1939_tp_all_buffers[allIndex++].inUse = false;
   }
#endif

#if (CONFIG_J1939TP_NUM_MAX_BUFFERS > 0)

   j1939_tp_buffer_init(j1939_tp_max_buffer, CONFIG_J1939TP_NUM_MAX_BUFFERS, CONFIG_J1939TP_BUFFER_MAX_SIZE);

   for (j1939_counter_t i = 0; i < CONFIG_J1939TP_NUM_MAX_BUFFERS; i++)
   {
      j1939_tp_all_buffers[allIndex].dataArea = j1939_tp_max_buffer[i];
      j1939_tp_all_buffers[allIndex].size = CONFIG_J1939TP_BUFFER_MAX_SIZE;
      j1939_tp_all_buffers[allIndex++].inUse = false;
   }
#endif

#ifndef CONFIG_J1939TP_RECEIVE_DISABLED
   for (j1939_counter_t tpRx = 0; tpRx < CONFIG_J1939TP_NUMBER_OF_TP_RX_SESSIONS; tpRx++)
   {
      J1939Tp_ReceiveMessageList[tpRx].state = j1939_tp_state_available;
   }
#endif

   for (j1939_counter_t transmit = 0; transmit < CONFIG_J1939TP_NUM_TRANSMIT_SESSIONS; transmit++)
   {
      j1939_tp_transmit_message_list[transmit].state = j1939_tp_state_available;
   }

#ifndef CONFIG_J1939TP_RECEIVE_DISABLED
   j1939_tp_num_holding_messages = 0;
   j1939_tp_num_open_receive_connections = 0;
#endif
   j1939_tp_num_open_transmit_connections = 0;

#ifdef J1939_DEBUG
   for (j1939_counter_t all = 0; all < CONFIG_J1939TP_NUM_ALL_BUFFERS; all++)
   {
      j1939_tp_all_buffers[all].maxSize = 0;
      j1939_tp_all_buffers[all].minSize = 0xFFFF;
      j1939_tp_all_buffers[all].totalSize = 0;
      j1939_tp_all_buffers[all].allocations = 0;
   }

   J1939Tp_DebugMetrics.numTenMsTicks = 0;
   J1939Tp_DebugMetrics.numSmallBuffersUsed = 0;
   J1939Tp_DebugMetrics.numMedBuffersUsed = 0;
   J1939Tp_DebugMetrics.numLargeBuffersUsed = 0;
   J1939Tp_DebugMetrics.numMaxBuffersUsed = 0;
   J1939Tp_DebugMetrics.numFilterAcceptedMsgs = 0;
   J1939Tp_DebugMetrics.numResourceAcceptedMsgs = 0;
   J1939Tp_DebugMetrics.sizeOfResourceAcceptedMessages = 0;
   J1939Tp_DebugMetrics.numResourceRejectedMsgs = 0;
   J1939Tp_DebugMetrics.numFilteredOutMsgs = 0;
   J1939Tp_DebugMetrics.maxNumSmallBuffersUsed = 0;
   J1939Tp_DebugMetrics.maxNumMedBuffersUsed = 0;
   J1939Tp_DebugMetrics.maxNumLargeBuffersUsed = 0;
   J1939Tp_DebugMetrics.maxNumMaxBuffersUsed = 0;
#endif
}

#ifndef CONFIG_J1939TP_RECEIVE_DISABLED
/**************************************************************************************************/
void j1939_tp_update_receive_message_times(j1939_timer_t callPeriod)
{
   j1939_counter_t index1;
   j1939_counter_t index2;
   bool check;
   CriticalSection_Lock();

   // Loop through the rx transport object list
   index2 = j1939_tp_num_open_receive_connections;
   for (index1 = 0; index2 && (index1 < CONFIG_J1939TP_NUMBER_OF_TP_RX_SESSIONS); index1++)
   {
      if (J1939Tp_ReceiveMessageList[index1].state == j1939_tp_state_receiving)
      {
         index2--;

         //  Increment the time from the last received packet.
         J1939Tp_ReceiveMessageList[index1].timeFromLastReceivedPacket += callPeriod;

         if (J1939Tp_ReceiveMessageList[index1].timeFromLastReceivedPacket > J1939TP_T1_TIMEOUT)
         {
            if (!J1939Tp_ReceiveMessageList[index1].isBamMessage)
            {
               // Send TP Connection Abort!
               j1939_tp_transmit_connection_abort(J1939TP_AbortCode_Timeout,
                                               J1939Tp_ReceiveMessageList[index1].pgn,
                                               J1939Tp_ReceiveMessageList[index1].source,
                                               J1939Tp_ReceiveMessageList[index1].node);
            }

            // Free up the buffer that this TP Rx object was using
            j1939_tp_free_buffer(J1939Tp_ReceiveMessageList[index1].incomingDataObject);

            j1939_tp_num_open_receive_connections--;

            // Disable message reception and ignore whatever was received
            J1939Tp_ReceiveMessageList[index1].state = j1939_tp_state_available;
         }
      }
      else if (J1939Tp_ReceiveMessageList[index1].state == j1939_tp_state_waiting)
      {
         index2--;
         J1939Tp_ReceiveMessageList[index1].timeFromLastCtsSent += callPeriod;

         if (J1939Tp_ReceiveMessageList[index1].timeFromLastCtsSent >= J1939TP_T2_TIMEOUT)
         {
            // kill the connection since tx'r did not send data
            if (!J1939Tp_ReceiveMessageList[index1].isBamMessage)
            {
               // Send TP Connection Abort!
               j1939_tp_transmit_connection_abort(J1939TP_AbortCode_Timeout,
                                               J1939Tp_ReceiveMessageList[index1].pgn,
                                               J1939Tp_ReceiveMessageList[index1].source,
                                               J1939Tp_ReceiveMessageList[index1].node);
            }

            // Free up the buffer that this TP Rx object was using
            j1939_tp_free_buffer(J1939Tp_ReceiveMessageList[index1].incomingDataObject);
            j1939_tp_num_open_receive_connections--;

            // Disable message reception and ignore whatever was received
            J1939Tp_ReceiveMessageList[index1].state = j1939_tp_state_available;
         }
      }
   }

   CriticalSection_Unlock();

   // We unlock, then relock to reduce latency

   CriticalSection_Lock();

   // Function callback code.
   index2 = j1939_tp_num_holding_messages;
   for (index1 = 0; index2 && (index1 < CONFIG_J1939TP_NUMBER_OF_TP_RX_SESSIONS); index1++)
   {
      if (J1939Tp_ReceiveMessageList[index1].state == j1939_tp_state_holding)
      {
         // VCallback user function
         check = j1939_tp_register_user_callback(J1939Tp_ReceiveMessageList[index1].pgn,
                                              J1939Tp_ReceiveMessageList[index1].incomingDataObject,
                                              J1939Tp_ReceiveMessageList[index1].sizeMessage,
                                              J1939Tp_ReceiveMessageList[index1].source,
                                              J1939Tp_ReceiveMessageList[index1].node);

         if (check)
         {
            // Free up resources after messages were processed.
            j1939_tp_free_buffer(J1939Tp_ReceiveMessageList[index1].incomingDataObject);
            j1939_tp_num_holding_messages--;
            J1939Tp_ReceiveMessageList[index1].state = j1939_tp_state_available;
         }
         // else message not registered for call backs.
      }
   }
   CriticalSection_Unlock();
}
#endif

/**************************************************************************************************/
void j1939_tp_update_send_message_times(j1939_timer_t callPeriod)
{
   uint16_t index1;
   j1939_byte_counter_t index2;
   j1939_byte_counter_t index3;
   uint8_t data[CAN_MAX_DLC];
   uint32_t id;
   j1939_timer_t deltaTime;
   uint16_t totalMessagesQueued = 0;
   uint16_t totalMessagesQueuedThisSession;
   uint8_t *currentDataLocation;
   CriticalSection_Lock();

   index3 = j1939_tp_num_open_transmit_connections; // short circuit if there are none
   for (index1 = 0; index3 && (index1 < CONFIG_J1939TP_NUM_TRANSMIT_SESSIONS); index1++)
   {
      //  If the current message is in use...
      if (j1939_tp_transmit_message_list[index1].state != j1939_tp_state_available)
      {
         // Update elapsed time counters:
         j1939_tp_transmit_message_list[index1].elapsedTimeFromLastCts += callPeriod;
         j1939_tp_transmit_message_list[index1].elapsedTimeFromLastTpDt += callPeriod;

         index3--;
         // If object is setup as destination specific...
         if (j1939_tp_transmit_message_list[index1].destination != J1939_GLOBAL_ADDRESS)
         {
            // If we've sent out the CTS allowed number of packets...
            if (j1939_tp_transmit_message_list[index1].numPacketsLeftCtsAllows == 0)
            {
               // If we're not completely done transmitting and are simply waiting for
               // the receiver to send us a CTS or EndofMsgACK...
               if (j1939_tp_transmit_message_list[index1].currentDataLocation <
                   (j1939_tp_transmit_message_list[index1].dataRegion +
                    (j1939_tp_transmit_message_list[index1].totalBytesToTransmit)))
               {
                  // Determine the time since the last received CTS
                  deltaTime = j1939_tp_transmit_message_list[index1].elapsedTimeFromLastCts;

                  // If we just finished sending the last allowed TP.DT and haven't
                  //       received a follow-up CTS in time or
                  //    we have received a CTS w/packets to send == 0 and haven't
                  //       received a subsequent CTS in time...
                  if (((j1939_tp_transmit_message_list[index1].isWaitingBetweenCts) &&
                       (deltaTime > J1939TP_T4_TIMEOUT)) ||
                      ((!j1939_tp_transmit_message_list[index1].isWaitingBetweenCts) &&
                       (deltaTime > J1939TP_T3_TIMEOUT)))
                  {
                     // Didn't get a CTS in time so disable this object and send a connection abort.
                     j1939_tp_transmit_connection_abort(
                         J1939TP_AbortCode_Timeout, j1939_tp_transmit_message_list[index1].pgn,
                         j1939_tp_transmit_message_list[index1].destination,
                         j1939_tp_transmit_message_list[index1].node);

                     // Free up the buffer that this TP Rx object was using
                     j1939_tp_num_open_transmit_connections--;
                     j1939_tp_free_buffer(j1939_tp_transmit_message_list[index1].dataRegion);
                     j1939_tp_transmit_message_list[index1].state = j1939_tp_state_available;
                  }
               }
               else
               {
                  // We are waiting for a TP.CM_EndofMsgACK
                  // An abort should be sent if an EOMAck hasn't been received in a timely manner
                  // since the last packet of the connection was transmitted or the last received
                  // CTS (whichever occurred most recently.)
                  if (j1939_tp_transmit_message_list[index1].elapsedTimeFromLastTpDt <
                      j1939_tp_transmit_message_list[index1].elapsedTimeFromLastCts)
                  {
                     /* TP.DT sent is more recent than the last received CTS */
                     deltaTime = j1939_tp_transmit_message_list[index1].elapsedTimeFromLastTpDt;
                  }
                  else
                  {
                     // The last CTS was received more recently than the send of the last
                     // packet of the conneciton.
                     deltaTime = j1939_tp_transmit_message_list[index1].elapsedTimeFromLastCts;
                  }

                  //  If we don't receive an EndofMsgACK in time...
                  if (deltaTime > J1939TP_T3_TIMEOUT)
                  {
                     // Didn't get a EOM in time so disable this object and
                     // send a connection abort.
                     j1939_tp_transmit_connection_abort(
                         J1939TP_AbortCode_Timeout, j1939_tp_transmit_message_list[index1].pgn,
                         j1939_tp_transmit_message_list[index1].destination,
                         j1939_tp_transmit_message_list[index1].node);

                     // Free up the buffer that this TP Rx object was using
                     j1939_tp_num_open_transmit_connections--;
                     j1939_tp_free_buffer(j1939_tp_transmit_message_list[index1].dataRegion);
                     j1939_tp_transmit_message_list[index1].state = j1939_tp_state_available;
                  }
               }
            }
            else
            {
#if (J1939TP_PACKET_TIME != 0)
               // This is a regular TP.DT messageDetermine if it is time to send the next packet out
               deltaTime = j1939_tp_transmit_message_list[index1].elapsedTimeFromLastTpDt;

               if (deltaTime >= J1939TP_PACKET_TIME)
#endif
               {
                  // If a CTS indicated that it doesn't want us to start
                  // at the next indicated packet...
                  if (j1939_tp_transmit_message_list[index1].nextRequestedPacket)
                  {
                     // Recalculate location in the transmit buffer and
                     // overwrite the current packet sequence number.
                     j1939_tp_transmit_message_list[index1].currentDataLocation =
                         j1939_tp_transmit_message_list[index1].dataRegion +
                         (7 * (j1939_tp_transmit_message_list[index1].nextRequestedPacket -
                               1)); //lint !e679
                     j1939_tp_transmit_message_list[index1].currentPacketSequenceNum =
                         j1939_tp_transmit_message_list[index1].nextRequestedPacket;
                     j1939_tp_transmit_message_list[index1].nextRequestedPacket = 0;
                  }

                  for (totalMessagesQueuedThisSession = 0;
                       (totalMessagesQueuedThisSession < CONFIG_J1939TP_MAX_MESSAGES_QUEUED_PER_SESSION) &&
                       j1939_tp_transmit_message_list[index1].numPacketsLeftCtsAllows &&
                       (totalMessagesQueued < J1939TP_MAX_TOTAL_MESSAGES_QUEUED_FOR_ALL_SESSIONS);
                       totalMessagesQueuedThisSession++)
                  {
                     data[0] =
                         j1939_tp_transmit_message_list[index1].currentPacketSequenceNum++; // packet #

                     if (j1939_tp_transmit_message_list[index1].currentPacketSequenceNum >
                         j1939_tp_transmit_message_list[index1].highestPacketSequenceNum)
                     {
                        j1939_tp_transmit_message_list[index1].highestPacketSequenceNum =
                            j1939_tp_transmit_message_list[index1].currentPacketSequenceNum;
                     }

                     // Store off the existing pointer in case we fail to queue the message
                     currentDataLocation = j1939_tp_transmit_message_list[index1].currentDataLocation;

                     // Copy over the 7 bytes of data to include in the packet.
                     for (index2 = 1; index2 < CAN_MAX_DLC; index2++)
                     {
                        //  If there is still data in the buffer to send...
                        if (j1939_tp_transmit_message_list[index1].currentDataLocation <
                            (j1939_tp_transmit_message_list[index1].dataRegion +
                             j1939_tp_transmit_message_list[index1].totalBytesToTransmit))
                        {
                           // Load data to transmit
                           data[index2] =
                               *(j1939_tp_transmit_message_list[index1].currentDataLocation)++;
                        }
                        else
                        {
                           // Default to J1939_BYTE_UNAVAILABLE
                           data[index2] = J1939_BYTE_UNAVAILABLE;
                        }
                     }

                     // Build the message ID
                     id = j1939_build_message_id(
                         0, 0, (j1939_priority_t)CONFIG_J1939TP_PRIORITY,
                         j1939_build_pgn_from_pdu(J1939_TP_DATA_TRANSFER_PF,
                                               j1939_tp_transmit_message_list[index1].destination),
                         j1939_tp_transmit_message_list[index1].node->source_address);

                     // Send the message out to the appropriate node.
                     if (!j1939_build_and_queue_message(j1939_tp_transmit_message_list[index1].node, id,
                                                     CAN_MAX_DLC, true,
                                                     data))
                     {
                        j1939_tp_transmit_message_list[index1].currentPacketSequenceNum--;
                        j1939_tp_transmit_message_list[index1].currentDataLocation =
                            currentDataLocation;
                        // Leave the for(totalMessagesQueuedThisSession...) loop right now since we
                        // cannot queue more messages
                        break;
                     }

                     j1939_tp_transmit_message_list[index1].numPacketsLeftCtsAllows--;
                     j1939_tp_transmit_message_list[index1].elapsedTimeFromLastTpDt = 0;

                     /*  If was message entirely transmitted...  */
                     if (j1939_tp_transmit_message_list[index1].currentDataLocation ==
                         (j1939_tp_transmit_message_list[index1].dataRegion +
                          j1939_tp_transmit_message_list[index1].totalBytesToTransmit))
                     {
                        j1939_tp_transmit_message_list[index1].isWaitingForEomAck = true;

                        // Leave the for(totalMessagesQueuedThisSession...) loop right now since we
                        // are done
                        break;
                     }

                     totalMessagesQueued++;
                  }
               } // if(deltaTime > J1939TP_PACKET_TIME)
            }
         }
         else
         {
            //  This is a BAM message
            deltaTime = j1939_tp_transmit_message_list[index1].elapsedTimeFromLastTpDt;

            // If it is time to send out the BAM message
            if (deltaTime >= J1939TP_BAM_TIME)
            {
               data[0] = j1939_tp_transmit_message_list[index1].currentPacketSequenceNum++;
               for (index2 = 1; index2 < CAN_MAX_DLC; index2++)
               {
                  // If there is still data in the buffer to send...
                  if (j1939_tp_transmit_message_list[index1].currentDataLocation <
                      (j1939_tp_transmit_message_list[index1].dataRegion +
                       j1939_tp_transmit_message_list[index1].totalBytesToTransmit))
                  {
                     //  Load data into transmit object
                     data[index2] = *(j1939_tp_transmit_message_list[index1].currentDataLocation)++;
                  }
                  else
                  {
                     // Default to J1939_BYTE_UNAVAILABLE
                     data[index2] = J1939_BYTE_UNAVAILABLE;
                  }
               }

               // Build the message ID
               id = j1939_build_message_id(
                   0, 0, (j1939_priority_t)CONFIG_J1939TP_PRIORITY,
                   j1939_build_pgn_from_pdu(J1939_TP_DATA_TRANSFER_PF, J1939_GLOBAL_ADDRESS),
                   j1939_tp_transmit_message_list[index1].node->source_address);

               // Send the message out to the appropriate node.
               (void)j1939_build_and_queue_message(j1939_tp_transmit_message_list[index1].node, id,
                                                CAN_MAX_DLC, true, data);

               //  If there is nothing left to send...
               if (j1939_tp_transmit_message_list[index1].currentDataLocation ==
                   (j1939_tp_transmit_message_list[index1].dataRegion +
                    j1939_tp_transmit_message_list[index1].totalBytesToTransmit))
               {
                  //  Completed BAM transmission...disable object
                  j1939_tp_num_open_transmit_connections--;

                  // Free up the buffer that this TP Rx object was using
                  j1939_tp_free_buffer(j1939_tp_transmit_message_list[index1].dataRegion);
                  j1939_tp_transmit_message_list[index1].state = j1939_tp_state_available;
                  if (j1939_tp_transmit_message_list[index1].node < CONFIG_J1939_NODES_COUNT)
                  {
                     j1939_tp_transmit_message_list[index1].node->j1939_tp_transmit_bam = false;
                  }
               }
               else
               {
                  // Reset the elapsed time with last tx time
                  j1939_tp_transmit_message_list[index1].elapsedTimeFromLastTpDt = 0;
               }
            }
         }
      }
   }
   CriticalSection_Unlock();

#ifdef J1939_DEBUG
   j1939_tp_timing_analysis();
#endif
}

#ifndef CONFIG_J1939TP_RECEIVE_DISABLED
/**************************************************************************************************/
bool j1939_tp_register_message_callback(j1939_pgn_t pgn, j1939_node_t node,
                                             j1939_tp_callback_t callback)
{
   uint16_t index;
   bool result = true;
   CriticalSection_Lock();

   for (index = 0;
        (index < node->j1939_tp_register_pgn_index) && (index < CONFIG_J1939TP_NUM_ALLOWED_RECEIVE_PGN);
        index++)
   {
      if (node->j1939_tp_register_pgn_list[index].pgn == pgn)
      {
         // Update function pointer & return.
         node->j1939_tp_register_pgn_list[index].callback = callback;

         // Exit the critical section and return to avoid below checks
         CriticalSection_Unlock();
         return true;
      }
   }

   if (index < CONFIG_J1939TP_NUM_ALLOWED_RECEIVE_PGN)
   {
      // add PGN to the list and increment the index
      node->j1939_tp_register_pgn_list[node->j1939_tp_register_pgn_index].pgn = pgn;
      node->j1939_tp_register_pgn_list[node->j1939_tp_register_pgn_index++].callback = callback;
   }
   else
   {
      // the list is full!
      result = false;
   }

   CriticalSection_Unlock();

   return result;
}

/**************************************************************************************************/
bool j1939_tp_register_extended_length_messages(j1939_pgn_t pgn, j1939_node_t node)
{
   j1939_counter_t index;

   if (node < CONFIG_J1939_NODES_COUNT)
   {
      CriticalSection_Lock();
      for (index = 0;
           (index < node->j1939_tp_register_pgn_index) && (index < CONFIG_J1939TP_NUM_ALLOWED_RECEIVE_PGN);
           index++)
      {
         if (node->j1939_tp_register_pgn_list[index].pgn == pgn)
         {
            // this PGN is already in the list
            CriticalSection_Unlock();
            return true;
         }
      }

      if (index < CONFIG_J1939TP_NUM_ALLOWED_RECEIVE_PGN)
      {
         // add PGN to the list and increment the index
         if (node->j1939_tp_register_pgn_index < CONFIG_J1939TP_NUM_ALLOWED_RECEIVE_PGN)
         {
            node->j1939_tp_register_pgn_list[node->j1939_tp_register_pgn_index++].pgn = pgn;
            CriticalSection_Unlock();
            return true;
         }
         else
         {
            CriticalSection_Unlock();
            return false;
         }
      }
      else
      {
         // the list is full!
         CriticalSection_Unlock();
         return false;
      }
   }
   else
   {
      // Bad node passed, could not register so we return false.
      return false;
   }
}
#endif

/**************************************************************************************************/
bool j1939_tp_transmit_session_exists(j1939_source_address_t source, j1939_node_t node)
{
   uint16_t index;
   bool isFound = false;
   if (source != J1939_GLOBAL_ADDRESS)
   {
      CriticalSection_Lock();
      for (index = 0; index < CONFIG_J1939TP_NUM_TRANSMIT_SESSIONS; index++)
      {
         if (j1939_tp_transmit_message_list[index].state != j1939_tp_state_available)
         {
            if ((j1939_tp_transmit_message_list[index].destination == source) &&
                (j1939_tp_transmit_message_list[index].node == node))
            {
               isFound = true;
               // Break out for performance reasons
               break;
            }
         }
      }
      CriticalSection_Unlock();
   }

   return (isFound);
}

/**************************************************************************************************/
uint8_t *j1939_tp_get_buffer(j1939_counter_t sizeMessage)
{
   uint8_t *buffer = NULL;
   j1939_counter_t actualBufferSize;
   CriticalSection_Lock();
   // Check if we allocated the required buffer space.
   if (!j1939_tp_buffer_allocate(sizeMessage, &buffer, &actualBufferSize))
   {
      // No buffer available. return a Null pointer.
      buffer = NULL;
   }
   CriticalSection_Unlock();

   // Silence lint warning
   (void)actualBufferSize;

   return buffer;
}

/**************************************************************************************************/
void j1939_tp_process_tpcm_message(const struct can_frame *message, j1939_node_t node)
{
   j1939_pgn_t pgn;

   if (message)
   {
      // Route data to appropriate function based on control byte
      switch (message->data[0])
      {
#ifndef CONFIG_J1939TP_RECEIVE_DISABLED
      case J1939TP_CM_BAM: // for TP rx messages
      case J1939TP_CM_RTS: // for TP rx messages
         j1939_tp_process_bam_rts(message->data[0], j1939_get_source_address(message->id),
                               message->data, node);
         break;
#endif

      case J1939TP_CM_CTS: // for TP tx messages
         j1939_tp_process_cts(j1939_get_source_address(message->id), message->data,
                            node);
         break;

      case J1939TP_CM_EOMACK: // for TP tx messages
         j1939_tp_process_eom_ack(j1939_get_source_address(message->id), message->data,
                               node);
         break;

      case J1939TP_CONNECTION_ABORT: // for TP tx messages
         j1939_tp_process_abort(j1939_get_source_address(message->id), message->data,
                              node);
         break;

      case J1939TP_CM_ERTS:
         pgn =
             MAKEDWORD(MAKEWORD(message->data[5], message->data[6]), MAKEWORD(message->data[7], 0));
         j1939_tp_transmit_connection_abort(J1939TP_AbortCode_Unknown, pgn,
                                         j1939_get_source_address(message->id),
                                         node);
         break;

      default:
         break;
      }
   }
}

/**************************************************************************************************/
J1939Tp_Message_T j1939_tp_transmit_multi_packet(j1939_pgn_t pgn,
                                              j1939_destination_address_t destination,
                                              j1939_counter_t dataLength, uint8_t *data,
                                              j1939_node_t node)
{
   j1939_counter_t txSessionIndex;
   j1939_counter_t bufferIndex;
   uint8_t txData[9];
   j1939_arbitration_t id;
   J1939Tp_Message_T result;
   bool isValidBuffer;

   __ASSERT_NO_MSG(data != NULL);

   result = j1939_tp_message_not_accepted;

   // If invalid data length requested  or this is a BAM request and
   // we are already transmitting a BAM msg..
   if (!(((dataLength > J1939TP_MAX_BYTES) || (dataLength < J1939TP_MIN_BYTES)) ||
         ((destination == J1939_GLOBAL_ADDRESS) && node->j1939_tp_transmit_bam)))
   {
      // if Tx Session already exists on that node.
      if (j1939_tp_transmit_session_exists(destination, node))
      {
         j1939_tp_free_buffer(data);
      }
      else
      {
         // Search through TP messages to see if we can load and send out that message
         CriticalSection_Lock();
         for (txSessionIndex = 0; txSessionIndex < CONFIG_J1939TP_NUM_TRANSMIT_SESSIONS; txSessionIndex++)
         {
            if (j1939_tp_transmit_message_list[txSessionIndex].state == j1939_tp_state_available)
            {
               // See if byte pointer matches one of our already allocated buffers.  If so, then
               // we'll use it to send the message. Else we'll allocate a buffer and copy the data
               isValidBuffer = false;
               for (bufferIndex = 0; bufferIndex < CONFIG_J1939TP_NUM_ALL_BUFFERS; bufferIndex++)
               {
                  if (j1939_tp_all_buffers[bufferIndex].dataArea == data)
                  {
                     // This is a match. Use the already allocated buffer and break out of this loop
                     // In this case we know we can safely cast away the  since all transport
                     // buffers are writable
                     j1939_tp_transmit_message_list[txSessionIndex].dataRegion =
                         (uint8_t *)data;
                     j1939_tp_transmit_message_list[txSessionIndex].sizeDataRegion =
                         j1939_tp_all_buffers[bufferIndex].size;
                     isValidBuffer = true;
                     break;
                  }
               }

               if (!isValidBuffer)
               {
                  isValidBuffer = j1939_tp_buffer_allocate(
                      dataLength, &j1939_tp_transmit_message_list[txSessionIndex].dataRegion,
                      &j1939_tp_transmit_message_list[txSessionIndex].sizeDataRegion);

                  if (isValidBuffer)
                  {
                     // Yep, we've got a TP that we can use for transmit
                     // store *packetData to associated TP PGN object
                     for (bufferIndex = 0; bufferIndex < dataLength; bufferIndex++)
                     {

                        *((j1939_tp_transmit_message_list[txSessionIndex].dataRegion) + bufferIndex) =
                            *(data + bufferIndex);
                     }
                  }
               }

               if (isValidBuffer)
               {
                  j1939_tp_transmit_message_list[txSessionIndex].currentDataLocation =
                      j1939_tp_transmit_message_list[txSessionIndex].dataRegion;

                  // Determine # of 7 byte data packets to send
                  j1939_tp_transmit_message_list[txSessionIndex].totalPackets =
                      (uint8_t)(dataLength / 7);

                  // Add a packet if there are any straggling bytes
                  if (dataLength % 7)
                  {
                     j1939_tp_transmit_message_list[txSessionIndex].totalPackets++;
                  }

                  // Set up TP message structure
                  j1939_tp_transmit_message_list[txSessionIndex].elapsedTimeFromLastCts = 0;
                  j1939_tp_transmit_message_list[txSessionIndex].elapsedTimeFromLastTpDt = 0;
                  j1939_tp_transmit_message_list[txSessionIndex].destination = destination;
                  j1939_tp_transmit_message_list[txSessionIndex].currentPacketSequenceNum = 1;
                  j1939_tp_transmit_message_list[txSessionIndex].nextRequestedPacket = 0;
                  j1939_tp_transmit_message_list[txSessionIndex].highestPacketSequenceNum = 1;
                  j1939_tp_transmit_message_list[txSessionIndex].lowestAblePacketSequenceNum = 0;
                  j1939_tp_transmit_message_list[txSessionIndex].totalBytesToTransmit = dataLength;
                  j1939_tp_transmit_message_list[txSessionIndex].numPacketsLeftCtsAllows = 0;
                  j1939_tp_transmit_message_list[txSessionIndex].pgn = pgn;
                  j1939_tp_transmit_message_list[txSessionIndex].state = j1939_tp_state_sending;
                  j1939_tp_transmit_message_list[txSessionIndex].isWaitingForEomAck = false;
                  j1939_tp_transmit_message_list[txSessionIndex].isWaitingBetweenCts = false;
                  j1939_tp_transmit_message_list[txSessionIndex].node = node;

                  // Send RTS to initiate transmit & Build message Data
                  if (destination == J1939_GLOBAL_ADDRESS)
                  {
                     txData[0] = J1939TP_CM_BAM;
                     txData[4] = J1939_BYTE_UNAVAILABLE;
                     node->j1939_tp_transmit_bam = true;
                  }
                  else
                  {
                     txData[0] = J1939TP_CM_RTS;
                     /* we can handle a request for all the packets */
                     txData[4] = j1939_tp_transmit_message_list[txSessionIndex].totalPackets;
                  }

                  txData[1] = LOBYTE(dataLength);
                  txData[2] = HIBYTE(dataLength);
                  txData[3] = j1939_tp_transmit_message_list[txSessionIndex].totalPackets;
                  txData[5] = LOBYTE(LOWORD(pgn));
                  txData[6] = HIBYTE(LOWORD(pgn));
                  txData[7] = LOBYTE(HIWORD(pgn));

                  // Build the message ID
                  id = j1939_build_message_id(
                      0, 0, (j1939_priority_t)CONFIG_J1939TP_PRIORITY,
                      j1939_build_pgn_from_pdu(J1939_TP_CONN_MANAGEMENT_PF, destination),
                      j1939_tp_transmit_message_list[txSessionIndex].node->source_address);

                  // Send the message out to the appropriate node.
                  (void)j1939_build_and_queue_message(j1939_tp_transmit_message_list[txSessionIndex].node,
                                                   id, CAN_MAX_DLC, true,
                                                   txData);

                  result = j1939_tp_message_accepted;
                  j1939_tp_num_open_transmit_connections++;
                  break;
               }
            }
         }
         CriticalSection_Unlock();

         // Were we able to transmit?
         if (txSessionIndex >= CONFIG_J1939TP_NUM_TRANSMIT_SESSIONS)
         {
            // No TP Tx Message Available. Free up any allocated buffer.
            j1939_tp_free_buffer(data);
         }
      }
   }
   // ELSE, the message is invalid or we are already transmitting a BAM.
   else
   {
      // Free up any allocated buffer.
      j1939_tp_free_buffer(data);
   }

   return (result);
}

#ifndef CONFIG_J1939TP_RECEIVE_DISABLED
/**************************************************************************************************/
void j1939_tp_process_dt(j1939_source_address_t source, const uint8_t *packetData,
                       j1939_destination_address_t destination, j1939_node_t node)
{
   j1939_counter_t rxSessionIndex;
   j1939_arbitration_t id;
   uint8_t sequenceNum;
   uint8_t data[9];
   CriticalSection_Lock();
   // Determine if TP.DT message is being listened to...
   for (rxSessionIndex = 0; rxSessionIndex < CONFIG_J1939TP_NUMBER_OF_TP_RX_SESSIONS; rxSessionIndex++)
   {
      // if the TP Rx resource is being used and the transmitter's address matches the address
      // we're getting the TP.DT from and it is on the same CAN node...
      if (((J1939Tp_ReceiveMessageList[rxSessionIndex].state == j1939_tp_state_receiving) ||
           (J1939Tp_ReceiveMessageList[rxSessionIndex].state == j1939_tp_state_waiting)) &&
          (J1939Tp_ReceiveMessageList[rxSessionIndex].source == source) &&
          (J1939Tp_ReceiveMessageList[rxSessionIndex].node == node) &&
          (J1939Tp_ReceiveMessageList[rxSessionIndex].isBamMessage ==
           (bool)(destination == J1939_GLOBAL_ADDRESS)))
      {
         break;
      }
   }

   if ((rxSessionIndex < CONFIG_J1939TP_NUMBER_OF_TP_RX_SESSIONS) && packetData)
   {
      // Looks like the message is being received
      sequenceNum = packetData[0];
      if ((J1939Tp_ReceiveMessageList[rxSessionIndex].lastReceivedPacket + 1) != sequenceNum)
      {
         // Uh, oh!  A packet was missed.
         // !!!! We can perform a connection abort or try to re-synch !!!!!

         // TBD We should really try to resync here.  The problem is that if we send a CTS now, we
         // may get more packets that were already queued by the sender, resulting in multiple
         // CTS message.  This may cause problems, so we are holding off on this change for now.

         // If this isn't a BAM message
         if (!J1939Tp_ReceiveMessageList[rxSessionIndex].isBamMessage)
         {
            j1939_tp_transmit_connection_abort(J1939TP_AbortCode_Unknown,
                                            J1939Tp_ReceiveMessageList[rxSessionIndex].pgn, source,
                                            node);
         }

         // Free up the buffer that this TP Rx object was using
         j1939_tp_num_open_receive_connections--;
         j1939_tp_free_buffer(J1939Tp_ReceiveMessageList[rxSessionIndex].incomingDataObject);
         J1939Tp_ReceiveMessageList[rxSessionIndex].state = j1939_tp_state_available;
      }
      else
      {
         // If this is the last packet of the session...
         if (J1939Tp_ReceiveMessageList[rxSessionIndex].lastExpectedPacket == sequenceNum)
         {
            // Copy data over
            J1939Tp_ReceiveMessageList[rxSessionIndex].timeFromLastReceivedPacket = 0;
            J1939Tp_ReceiveMessageList[rxSessionIndex].lastReceivedPacket = sequenceNum;
            for (j1939_counter_t index = 1;
                 (index < CAN_MAX_DLC) &&
                 (J1939Tp_ReceiveMessageList[rxSessionIndex].currentByteOffset <
                  J1939Tp_ReceiveMessageList[rxSessionIndex].sizeMessage);
                 index++)
            {
               *(J1939Tp_ReceiveMessageList[rxSessionIndex].incomingDataObject +
                 J1939Tp_ReceiveMessageList[rxSessionIndex].currentByteOffset) = packetData[index];
               J1939Tp_ReceiveMessageList[rxSessionIndex].currentByteOffset++;
            }

            // If this wasn't a BAM message...
            if (!J1939Tp_ReceiveMessageList[rxSessionIndex].isBamMessage)
            {
               data[0] = J1939TP_CM_EOMACK;
               data[1] = LOBYTE(J1939Tp_ReceiveMessageList[rxSessionIndex].sizeMessage);
               data[2] = HIBYTE(J1939Tp_ReceiveMessageList[rxSessionIndex].sizeMessage);
               data[3] = sequenceNum;
               data[4] = J1939_BYTE_UNAVAILABLE;
               data[5] = LOBYTE(LOWORD(J1939Tp_ReceiveMessageList[rxSessionIndex].pgn));
               data[6] = HIBYTE(LOWORD(J1939Tp_ReceiveMessageList[rxSessionIndex].pgn));
               data[7] = LOBYTE(HIWORD(J1939Tp_ReceiveMessageList[rxSessionIndex].pgn));

               // Build the message ID
               id = j1939_build_message_id(
                   0, 0, (j1939_priority_t)CONFIG_J1939TP_PRIORITY,
                   j1939_build_pgn_from_pdu(J1939_TP_CONN_MANAGEMENT_PF, source),
                   J1939Tp_ReceiveMessageList[rxSessionIndex].node->source_address);

               // Send the message out to the appropriate node.
               (void)j1939_build_and_queue_message(J1939Tp_ReceiveMessageList[rxSessionIndex].node, id,
                                                CAN_MAX_DLC, true, data);
            }

            // Turn off data recording.
            J1939Tp_ReceiveMessageList[rxSessionIndex].state = j1939_tp_state_holding;
            j1939_tp_num_holding_messages++;
            j1939_tp_num_open_receive_connections--;
         }
         else
         {
            // Stuff data into the incoming data object
            J1939Tp_ReceiveMessageList[rxSessionIndex].timeFromLastReceivedPacket = 0;
            J1939Tp_ReceiveMessageList[rxSessionIndex].state = j1939_tp_state_receiving;
            J1939Tp_ReceiveMessageList[rxSessionIndex].lastReceivedPacket = sequenceNum;

            for (j1939_counter_t index = 1;
                 (index < CAN_MAX_DLC) &&
                 (J1939Tp_ReceiveMessageList[rxSessionIndex].currentByteOffset <
                  J1939Tp_ReceiveMessageList[rxSessionIndex].sizeMessage);
                 index++)
            {
               *(J1939Tp_ReceiveMessageList[rxSessionIndex].incomingDataObject +
                 J1939Tp_ReceiveMessageList[rxSessionIndex].currentByteOffset) = packetData[index];
               J1939Tp_ReceiveMessageList[rxSessionIndex].currentByteOffset++;
            }

            if (--J1939Tp_ReceiveMessageList[rxSessionIndex].packetCountDown == 0)
            {
               j1939_tp_transmit_cts(source, rxSessionIndex, sequenceNum + 1);
            }
         }
      }
   }
   CriticalSection_Unlock();
}

/**************************************************************************************************/
uint8_t *j1939_tp_get_completed_session_buffer(j1939_pgn_t pgn, j1939_source_address_t source,
                                                  j1939_node_t node, j1939_counter_t *length)
{
   j1939_counter_t wHoldingMessages = j1939_tp_num_holding_messages;
   uint8_t *buffer = NULL;

   if (length)
   {
      // Default to zero in case we do not find a match.
      *length = 0;

      CriticalSection_Lock();
      for (j1939_counter_t index = 0;
           wHoldingMessages && (index < CONFIG_J1939TP_NUMBER_OF_TP_RX_SESSIONS); index++)
      {
         if (J1939Tp_ReceiveMessageList[index].state == j1939_tp_state_holding)
         {
            wHoldingMessages--;
            if (pgn == J1939Tp_ReceiveMessageList[index].pgn)
            {
               if (source == J1939Tp_ReceiveMessageList[index].source)
               {
                  if (node == J1939Tp_ReceiveMessageList[index].node)
                  {
                     *length = J1939Tp_ReceiveMessageList[index].sizeMessage;

                     // Return from inside the loop for efficency
                     buffer = J1939Tp_ReceiveMessageList[index].incomingDataObject;
                     break;
                  }
               }
            }
         }
      }
      CriticalSection_Unlock();
   }

   return buffer;
}

/**************************************************************************************************/
void j1939_tp_free_completed_buffer(uint8_t *buffer)
{
   CriticalSection_Lock();

   for (j1939_counter_t index = 0; (index < CONFIG_J1939TP_NUMBER_OF_TP_RX_SESSIONS); index++)
   {
      if (J1939Tp_ReceiveMessageList[index].incomingDataObject == buffer)
      {
         j1939_tp_free_buffer(buffer);
         j1939_tp_num_holding_messages--;
         J1939Tp_ReceiveMessageList[index].state = j1939_tp_state_available;
         break;
      }
   }

   CriticalSection_Unlock();
}

/**************************************************************************************************/
static bool j1939_tp_is_pgn_supported(j1939_pgn_t pgn, j1939_node_t node)
{
   if (node < CONFIG_J1939_NODES_COUNT)
   {
      for (j1939_counter_t index = 0;
           (index < node->j1939_tp_register_pgn_index) && (index < CONFIG_J1939TP_NUM_ALLOWED_RECEIVE_PGN);
           index++)
      {
         if (node->j1939_tp_register_pgn_list[index].pgn == pgn)
         {
            return true;
         }
      }
   }

   return false;
}
#endif

/**************************************************************************************************/
static bool j1939_tp_buffer_allocate(j1939_counter_t sizeMessage, uint8_t **buffer,
                                           j1939_counter_t *sizeBuffer)
{
   bool result = false;

   __ASSERT_NO_MSG(sizeBuffer != NULL);
   __ASSERT_NO_MSG(buffer != NULL);

   if ((buffer != NULL) && (sizeBuffer != NULL))
   {
      for (j1939_counter_t index = 0; index < CONFIG_J1939TP_NUM_ALL_BUFFERS; index++)
      {
         if (j1939_tp_all_buffers[index].size >= sizeMessage)
         {
            // This portion of code must be protected against multiple calls.  If unprotected, it
            // could possibly assign a single buffer to two different requesters.
            if (!j1939_tp_all_buffers[index].inUse)
            {
               j1939_tp_all_buffers[index].inUse = true;
               *buffer = j1939_tp_all_buffers[index].dataArea;
               *sizeBuffer = j1939_tp_all_buffers[index].size;
               result = true;

#ifdef J1939_DEBUG
               if (sizeMessage > j1939_tp_all_buffers[index].maxSize)
               {
                  j1939_tp_all_buffers[index].maxSize = sizeMessage;
               }

               if (sizeMessage < j1939_tp_all_buffers[index].minSize)
               {
                  j1939_tp_all_buffers[index].minSize = sizeMessage;
               }
               j1939_tp_all_buffers[index].totalSize += sizeMessage;
               j1939_tp_all_buffers[index].allocations++;
#endif
               // Break out as soon as we have a buffer for performance reasons
               break;
            }
         }
      }
   }

   return (result);
}

/**************************************************************************************************/
void j1939_tp_free_buffer(uint8_t *buffer)
{
   for (j1939_counter_t index = 0; index < CONFIG_J1939TP_NUM_ALL_BUFFERS; index++)
   {
      if (j1939_tp_all_buffers[index].dataArea == buffer)
      {
         j1939_tp_all_buffers[index].inUse = false;
         break;
      }
   }
}

/**************************************************************************************************/
static void j1939_tp_transmit_connection_abort(j1939_tp_abort_code_t abortCode, j1939_pgn_t pgn,
                                            j1939_source_address_t source, j1939_node_t node)
{
   j1939_arbitration_t id;
   uint8_t data[CAN_MAX_DLC];

   // Fill in the data
   data[0] = J1939TP_CONNECTION_ABORT;
   data[1] = LOBYTE(abortCode);
   data[2] = J1939_BYTE_UNAVAILABLE;
   data[3] = J1939_BYTE_UNAVAILABLE;
   data[4] = J1939_BYTE_UNAVAILABLE;
   data[5] = LOBYTE(LOWORD(pgn));
   data[6] = HIBYTE(LOWORD(pgn));
   data[7] = LOBYTE(HIWORD(pgn));

   // Build identifier for message
   id = j1939_build_message_id(0, 0, (j1939_priority_t)CONFIG_J1939TP_PRIORITY,
                             j1939_build_pgn_from_pdu(J1939_TP_CONN_MANAGEMENT_PF, source),
                             node->source_address);

   // Send the message out to the appropriate node.
   (void)j1939_build_and_queue_message(node, id, CAN_MAX_DLC, true, data);
}

#ifndef CONFIG_J1939TP_RECEIVE_DISABLED
/**************************************************************************************************/
static bool j1939_tp_register_user_callback(j1939_pgn_t pgn, uint8_t *data,
                                                 j1939_counter_t length,
                                                 j1939_source_address_t source, j1939_node_t node)
{
   j1939_tp_callback_t callback;

   if (node < CONFIG_J1939_NODES_COUNT)
   {
      // See if the PGN is registered & supported on this node.
      for (j1939_counter_t index = 0;
           (index < node->j1939_tp_register_pgn_index) && (index < CONFIG_J1939TP_NUM_ALLOWED_RECEIVE_PGN);
           index++)
      {
         if (node->j1939_tp_register_pgn_list[index].pgn == pgn)
         {
            callback = node->j1939_tp_register_pgn_list[index].callback;
            // Yes PGN is supported. If call back is not a NULL pointer, then we're ready to go.
            if (callback != NULL)
            {
               // Call user function.
               (void)(*callback)(pgn, data, length, source, node);
               return true;
            }
         }
      }
   }

   return false;
}

/**************************************************************************************************/
static bool j1939_tp_cancel_old_receive_sessions(j1939_counter_t index,
                                                     j1939_source_address_t source,
                                                     j1939_node_t node, bool isBam)
{
   bool result = false;

   if ((J1939Tp_ReceiveMessageList[index].state != j1939_tp_state_available) &&
       (J1939Tp_ReceiveMessageList[index].node == node) &&
       (J1939Tp_ReceiveMessageList[index].source == source) &&
       (J1939Tp_ReceiveMessageList[index].isBamMessage == (bool)isBam))
   {
      // session exists, cancel it.
      J1939Tp_ReceiveMessageList[index].state = j1939_tp_state_available;

      // free the associated buffer
      j1939_tp_free_buffer(J1939Tp_ReceiveMessageList[index].incomingDataObject);

      result = true;
   }

   return result;
}
#endif

/**************************************************************************************************/
static void j1939_tp_process_cts(j1939_source_address_t source, const uint8_t *data,
                               j1939_node_t node)
{
   uint8_t numAllowedPackets;
   uint8_t nextPacketToSend;
   j1939_pgn_t pgn;
   __ASSERT_NO_MSG(data != NULL);

   numAllowedPackets = data[1];
   nextPacketToSend = data[2];
   pgn = MAKEDWORD(MAKEWORD(data[5], data[6]), MAKEWORD(data[7], 0));

   CriticalSection_Lock();
   // Loop through the TP messages and find the one referred to by the CTS msg
   for (j1939_counter_t index = 0; index < CONFIG_J1939TP_NUM_TRANSMIT_SESSIONS; index++)
   {
      // If this TP message has the same PGN as that indicated by the CTS message
      //   and this message is also being sent to the source of that CTS...
      if ((j1939_tp_transmit_message_list[index].pgn == pgn) &&
          (j1939_tp_transmit_message_list[index].destination == source) &&
          (j1939_tp_transmit_message_list[index].state != j1939_tp_state_available) &&
          (j1939_tp_transmit_message_list[index].node == node))
      {
         // Capture the current time to indicate when last CTS was rxd
         j1939_tp_transmit_message_list[index].elapsedTimeFromLastCts = 0;

         // If there aren't any packets left that the previous CTS allowed and
         //    the next packet requested is less than or equal to the highest one sent and
         //    the number of packets to send and the next one to send won't go past the end of the
         //    total number in the connection and the number of packets to send is !0 (ie this is
         //    not a hold type CTS) and the next packet to send isn't one that we assumed we had
         //    previously transmitted correctly...
         if ((j1939_tp_transmit_message_list[index].numPacketsLeftCtsAllows == 0) &&
             (j1939_tp_transmit_message_list[index].highestPacketSequenceNum >= nextPacketToSend) &&
             ((nextPacketToSend + numAllowedPackets) <=
              (j1939_tp_transmit_message_list[index].totalPackets + 1)) &&
             (numAllowedPackets != 0) &&
             (nextPacketToSend >= j1939_tp_transmit_message_list[index].lowestAblePacketSequenceNum))
         {
            j1939_tp_transmit_message_list[index].numPacketsLeftCtsAllows = numAllowedPackets;

            // If the next requested packet is not the expected next packet...
            if (nextPacketToSend != j1939_tp_transmit_message_list[index].currentPacketSequenceNum)
            {
               // There must have been an error so set conditions to retransmit
               // starting at the requested packet
               j1939_tp_transmit_message_list[index].nextRequestedPacket = nextPacketToSend;
            }
            else
            {
               j1939_tp_transmit_message_list[index].lowestAblePacketSequenceNum = nextPacketToSend;
               j1939_tp_transmit_message_list[index].nextRequestedPacket = 0;
            }

            j1939_tp_transmit_message_list[index].isWaitingBetweenCts = false;
         }
         else if (numAllowedPackets == 0)
         {
            j1939_tp_transmit_message_list[index].isWaitingBetweenCts = true;
         }
         else
         {
            // Why did we receive CTS in the middle of transmitting?  We should
            // only get one when we've transmitted a chunk of packets.
            // Send out a connection abort, due to receiver's ineptitude, here!!!
            // Also disable the object.
            j1939_tp_transmit_connection_abort(J1939TP_AbortCode_Unknown,
                                            j1939_tp_transmit_message_list[index].pgn,
                                            j1939_tp_transmit_message_list[index].destination,
                                            j1939_tp_transmit_message_list[index].node);

            // Free up the buffer that this TP Rx object was using
            j1939_tp_num_open_transmit_connections--;
            j1939_tp_free_buffer(j1939_tp_transmit_message_list[index].dataRegion);
            j1939_tp_transmit_message_list[index].state = j1939_tp_state_available;
         }
         break; // no need to continue with the for loop
      }
   }
   CriticalSection_Unlock();
}

/**************************************************************************************************/
static void j1939_tp_process_eom_ack(j1939_source_address_t source, const uint8_t *data,
                                  j1939_node_t node)
{
   j1939_pgn_t pgn;
   __ASSERT_NO_MSG(data != NULL);

   pgn = MAKEDWORD(MAKEWORD(data[5], data[6]), MAKEWORD(data[7], 0));

   CriticalSection_Lock();
   // Loop through the TP messages and find the one referred to by the EOMACK msg
   for (j1939_counter_t index = 0; index < CONFIG_J1939TP_NUM_TRANSMIT_SESSIONS; index++)
   {
      // If this TP message has the same PGN as that indicated by the EOMACK message
      //    and this message is also being sent to the source of that EOMACK
      //    and this message is on the same node
      //    and this module is actually waiting for a EOMACK...
      if ((j1939_tp_transmit_message_list[index].pgn == pgn) &&
          (j1939_tp_transmit_message_list[index].destination == source) &&
          (j1939_tp_transmit_message_list[index].state != j1939_tp_state_available) &&
          (j1939_tp_transmit_message_list[index].node == node) &&
          (j1939_tp_transmit_message_list[index].isWaitingForEomAck))
      {
         j1939_tp_num_open_transmit_connections--;
         j1939_tp_free_buffer(j1939_tp_transmit_message_list[index].dataRegion);
         j1939_tp_transmit_message_list[index].state = j1939_tp_state_available;
         break;
      }
   }
   CriticalSection_Unlock();
}

/**************************************************************************************************/
static void j1939_tp_process_abort(j1939_source_address_t source, const uint8_t *data,
                                 j1939_node_t node)
{
   j1939_pgn_t pgn;
   __ASSERT_NO_MSG(data != NULL);

   pgn = MAKEDWORD(MAKEWORD(data[5], data[6]), MAKEWORD(data[7], 0));

#ifndef CONFIG_J1939TP_RECEIVE_DISABLED
   CriticalSection_Lock();
   // Loop through the TP messages and find the one referred to by the Conn_Abort msg
   for (j1939_counter_t index = 0; index < CONFIG_J1939TP_NUMBER_OF_TP_RX_SESSIONS; index++)
   {
      // if the TP Rx resource is being used and the transmitter's address matches the address
      // we're getting the abort from and it is on the same CAN node...
      if ((J1939Tp_ReceiveMessageList[index].pgn == pgn) &&
          (J1939Tp_ReceiveMessageList[index].state != j1939_tp_state_available) &&
          (J1939Tp_ReceiveMessageList[index].source == source) &&
          (J1939Tp_ReceiveMessageList[index].isBamMessage == 0) &&
          (J1939Tp_ReceiveMessageList[index].node == node))
      {
         // Free up the buffer that this TP Rx object was using
         j1939_tp_free_buffer(J1939Tp_ReceiveMessageList[index].incomingDataObject);
         j1939_tp_num_open_receive_connections--;
         J1939Tp_ReceiveMessageList[index].state = j1939_tp_state_available;
         break; // no need to continue with the for loop
      }
   }
   CriticalSection_Unlock();
#endif

   // We unlock then relock here to reduce latency when we shut off interrupts

   CriticalSection_Lock();
   // Loop through the TP messages and find the one referred to by the Conn_Abort msg
   for (j1939_counter_t index = 0; index < CONFIG_J1939TP_NUM_TRANSMIT_SESSIONS; index++)
   {
      // If this TP message has the same PGN as that indicated by the Conn_Abort message
      //    and this message is also being sent to the source of that Conn_Abort
      //    and it occurs on the same CAN node...
      if ((j1939_tp_transmit_message_list[index].pgn == pgn) &&
          (j1939_tp_transmit_message_list[index].destination == source) &&
          (j1939_tp_transmit_message_list[index].state != j1939_tp_state_available) &&
          (j1939_tp_transmit_message_list[index].node == node))
      {
         // Free up the buffer that this TP Rx object was using
         j1939_tp_num_open_transmit_connections--;
         j1939_tp_free_buffer(j1939_tp_transmit_message_list[index].dataRegion);
         j1939_tp_transmit_message_list[index].state = j1939_tp_state_available;
         break; // no need to continue with the for loop
      }
   }
   CriticalSection_Unlock();
}

#ifndef CONFIG_J1939TP_RECEIVE_DISABLED
/**************************************************************************************************/
static void j1939_tp_process_bam_rts(j1939_pdu_format_t control, j1939_source_address_t source,
                                  const uint8_t *packetData, j1939_node_t node)
{
   j1939_counter_t totalMessageSize;
   j1939_arbitration_t id;
   j1939_pgn_t pgn;
   uint8_t totalPackets;
   uint8_t maxPacketsCanSend;
   j1939_counter_t rxSessionIndex;
   uint8_t data[CAN_MAX_DLC];
   __ASSERT_NO_MSG(packetData != NULL);

   totalMessageSize = MAKEWORD(packetData[1], packetData[2]);
   totalPackets = packetData[3];
   maxPacketsCanSend = packetData[4];

   if (control == J1939TP_CM_BAM)
   {
      maxPacketsCanSend = totalPackets;
   }
   else
   {
      // If max greater than total packets set max to total packets.
      if (maxPacketsCanSend > totalPackets)
      {
         maxPacketsCanSend = totalPackets;
      }

      // Lets only allow at max number of packets at a time for flow control
      if (maxPacketsCanSend > CONFIG_J1939TP_MAX_PACKETS)
      {
         maxPacketsCanSend = CONFIG_J1939TP_MAX_PACKETS;
      }
   }

   pgn = MAKEDWORD(MAKEWORD(packetData[5], packetData[6]), MAKEWORD(packetData[7], 0));

   CriticalSection_Lock();
   for (rxSessionIndex = 0; rxSessionIndex < CONFIG_J1939TP_NUMBER_OF_TP_RX_SESSIONS; rxSessionIndex++)
   {
      // if sessions exists, cancel it and start over.
      (void)j1939_tp_cancel_old_receive_sessions(rxSessionIndex, source, node,
                                             (bool)(control == J1939TP_CM_BAM));
   }

   // Okay, we've got a Request To Send message.  Let's first see if we even support it.
   if (j1939_tp_is_pgn_supported(pgn, node))
   {
#ifdef J1939_DEBUG
      J1939Tp_DebugMetrics.numFilterAcceptedMsgs++;
#endif
      // PGN is supported, assign resources
      for (rxSessionIndex = 0; rxSessionIndex < CONFIG_J1939TP_NUMBER_OF_TP_RX_SESSIONS; rxSessionIndex++)
      {
         if (J1939Tp_ReceiveMessageList[rxSessionIndex].state == j1939_tp_state_available)
         {
            // attempt to assign a rx buffer to this message
            // If the total packets is invalid or
            //    the message size is invalid or
            //    space couldn't be allocated
            if ((totalPackets <= 1) || (totalMessageSize < J1939TP_MIN_BYTES) ||
                (totalMessageSize > J1939TP_MAX_BYTES) ||
                (j1939_tp_buffer_allocate(
                     totalMessageSize,
                     &J1939Tp_ReceiveMessageList[rxSessionIndex].incomingDataObject,
                     &J1939Tp_ReceiveMessageList[rxSessionIndex].dataObjectSize) == false))
            {
#ifdef J1939_DEBUG
               J1939Tp_DebugMetrics.numResourceRejectedMsgs++;
#endif
               if (control == J1939TP_CM_RTS)
               {
                  // Spec says we're supposed to send back a Connection Abort if not a BAM session
                  j1939_tp_transmit_connection_abort(J1939TP_AbortCode_Unknown, pgn, source, node);
               }
            }
            else
            {

               // Prepare resource found with session information
               J1939Tp_ReceiveMessageList[rxSessionIndex].state = j1939_tp_state_waiting;
               J1939Tp_ReceiveMessageList[rxSessionIndex].lastReceivedPacket = 0;
               J1939Tp_ReceiveMessageList[rxSessionIndex].lastExpectedPacket = totalPackets;
               J1939Tp_ReceiveMessageList[rxSessionIndex].packetCountDown = maxPacketsCanSend;
               J1939Tp_ReceiveMessageList[rxSessionIndex].maxRequestedPackets = maxPacketsCanSend;
               J1939Tp_ReceiveMessageList[rxSessionIndex].source = source;
               J1939Tp_ReceiveMessageList[rxSessionIndex].isBamMessage =
                   (bool)(control == J1939TP_CM_BAM);
               J1939Tp_ReceiveMessageList[rxSessionIndex].sizeMessage = totalMessageSize;
               J1939Tp_ReceiveMessageList[rxSessionIndex].currentByteOffset = 0;
               J1939Tp_ReceiveMessageList[rxSessionIndex].timeFromLastReceivedPacket = 0;
               J1939Tp_ReceiveMessageList[rxSessionIndex].pgn = pgn;
               J1939Tp_ReceiveMessageList[rxSessionIndex].node = node;

               // If point-to-point transport session.
               if (control == J1939TP_CM_RTS)
               {
                  // Send back an acceptance message for the session
                  J1939Tp_ReceiveMessageList[rxSessionIndex].timeFromLastCtsSent = 0;

                  data[0] = J1939TP_CM_CTS;
                  data[1] = maxPacketsCanSend;
                  data[2] = 1; // packet 1.
                  data[3] = J1939_BYTE_UNAVAILABLE;
                  data[4] = J1939_BYTE_UNAVAILABLE;
                  data[5] = LOBYTE(LOWORD(pgn));
                  data[6] = HIBYTE(LOWORD(pgn));
                  data[7] = LOBYTE(HIWORD(pgn));

                  // Build the message ID
                  id = j1939_build_message_id(
                      0, 0, (j1939_priority_t)CONFIG_J1939TP_PRIORITY,
                      j1939_build_pgn_from_pdu(J1939_TP_CONN_MANAGEMENT_PF, source),
                      J1939Tp_ReceiveMessageList[rxSessionIndex].node->source_address);

                  // Send the message out to the appropriate node.
                  (void)j1939_build_and_queue_message(J1939Tp_ReceiveMessageList[rxSessionIndex].node,
                                                   id, CAN_MAX_DLC, true,
                                                   data);
               } // end IF (RTS/CTS point-to-point session).
               // If BAM message.
               else
               {
                  J1939Tp_ReceiveMessageList[rxSessionIndex].timeFromLastCtsSent = 0;
               }
               j1939_tp_num_open_receive_connections++;
               break;
            }
         } // bystate == j1939_tp_state_available;
      }    // for loop

      if (rxSessionIndex >= CONFIG_J1939TP_NUMBER_OF_TP_RX_SESSIONS)
      {
         if (control == J1939TP_CM_RTS)
         {
            // Not a BAM message, spec says we're supposed to send back a Connection Abort.
            j1939_tp_transmit_connection_abort(J1939TP_AbortCode_Unknown, pgn, source, node);
         }
      }
   }
   else
   {
#ifdef J1939_DEBUG
      J1939Tp_DebugMetrics.numFilteredOutMsgs++;
#endif
      // reject message
      if (control == J1939TP_CM_RTS)
      {
         // Not a BAM message, spec says we're supposed to send back a Connection Abort.
         j1939_tp_transmit_connection_abort(J1939TP_AbortCode_Unknown, pgn, source, node);
      }
   }

   CriticalSection_Unlock();
}
#endif

#ifdef J1939_DEBUG
/**************************************************************************************************/
static void j1939_tp_timing_analysis(void)
{
   uint16_t smallBufferCount = 0;
   uint16_t mediumBufferCount = 0;
   uint16_t largeBufferCount = 0;
   uint16_t hugeBufferCount = 0;
   uint16_t index;

   J1939Tp_DebugMetrics.numTenMsTicks++;

   for (index = 0; index < CONFIG_J1939TP_NUM_ALL_BUFFERS; index++)
   {
      if (j1939_tp_all_buffers[index].inUse == true)
      {
         switch (j1939_tp_all_buffers[index].size)
         {
         case CONFIG_J1939TP_BUFFER_SMALL_SIZE:
            J1939Tp_DebugMetrics.numSmallBuffersUsed++;
            smallBufferCount++;
            break;

         case CONFIG_J1939TP_BUFFER_MEDIUM_SIZE:
            J1939Tp_DebugMetrics.numMedBuffersUsed++;
            mediumBufferCount++;
            break;

         case CONFIG_J1939TP_BUFFER_LARGE_SIZE:
            J1939Tp_DebugMetrics.numLargeBuffersUsed++;
            largeBufferCount++;
            break;

         case CONFIG_J1939TP_BUFFER_MAX_SIZE:
            J1939Tp_DebugMetrics.numMaxBuffersUsed++;
            hugeBufferCount++;
            break;

         default:
            break;
         }
      }
   }

   if (smallBufferCount > J1939Tp_DebugMetrics.maxNumSmallBuffersUsed)
   {
      J1939Tp_DebugMetrics.maxNumSmallBuffersUsed = smallBufferCount;
   }
   if (mediumBufferCount > J1939Tp_DebugMetrics.maxNumMedBuffersUsed)
   {
      J1939Tp_DebugMetrics.maxNumMedBuffersUsed = mediumBufferCount;
   }
   if (largeBufferCount > J1939Tp_DebugMetrics.maxNumLargeBuffersUsed)
   {
      J1939Tp_DebugMetrics.maxNumLargeBuffersUsed = largeBufferCount;
   }
   if (hugeBufferCount > J1939Tp_DebugMetrics.maxNumMaxBuffersUsed)
   {
      J1939Tp_DebugMetrics.maxNumMaxBuffersUsed = hugeBufferCount;
   }
}
#endif

/**************************************************************************************************/
j1939_counter_t j1939_tp_get_max_available_buffer_size(void)
{
   j1939_counter_t result = 0;
   j1939_counter_t index;
   CriticalSection_Lock();

   for (index = 0; index < ELEMENTS(j1939_tp_all_buffers); index++)
   {
      if (!j1939_tp_all_buffers[index].inUse)
      {
         if (result < j1939_tp_all_buffers[index].size)
         {
            result = j1939_tp_all_buffers[index].size;
         }
      }
   }

   CriticalSection_Unlock();

   return (result);
}

#ifndef CONFIG_J1939TP_RECEIVE_DISABLED

/**************************************************************************************************/
static void j1939_tp_transmit_cts(j1939_destination_address_t destination, j1939_counter_t index,
                                uint8_t packetNum)
{
   uint8_t data[CAN_MAX_DLC];
   uint8_t temp;
   j1939_arbitration_t id;

   temp = (uint8_t)((J1939Tp_ReceiveMessageList[index].lastExpectedPacket - packetNum) + 1);
   if (temp > J1939Tp_ReceiveMessageList[index].maxRequestedPackets)
   {
      temp = J1939Tp_ReceiveMessageList[index].maxRequestedPackets;
   }

   data[0] = J1939TP_CM_CTS;
   data[1] = temp;      // J1939Tp_ReceiveMessageList[index].maxRequestedPackets;
   data[2] = packetNum; // refer to the NEXT expected packet
   data[3] = J1939_BYTE_UNAVAILABLE;
   data[4] = J1939_BYTE_UNAVAILABLE;
   data[5] = LOBYTE(LOWORD(J1939Tp_ReceiveMessageList[index].pgn));
   data[6] = HIBYTE(LOWORD(J1939Tp_ReceiveMessageList[index].pgn));
   data[7] = LOBYTE(HIWORD(J1939Tp_ReceiveMessageList[index].pgn));

   // Build the message ID
   id = j1939_build_message_id(0, 0, (j1939_priority_t)CONFIG_J1939TP_PRIORITY,
                             j1939_build_pgn_from_pdu(J1939_TP_CONN_MANAGEMENT_PF, destination),
                             J1939Tp_ReceiveMessageList[index].node->source_address);

   (void)j1939_build_and_queue_message(J1939Tp_ReceiveMessageList[index].node, id,
                                    CAN_MAX_DLC, true, data);

   J1939Tp_ReceiveMessageList[index].packetCountDown =
       J1939Tp_ReceiveMessageList[index].maxRequestedPackets;

   J1939Tp_ReceiveMessageList[index].timeFromLastCtsSent = 0;
   J1939Tp_ReceiveMessageList[index].state = j1939_tp_state_waiting;
}
#endif

#ifndef CONFIG_J1939TP_RECEIVE_DISABLED
/**************************************************************************************************/
bool j1939_tp_data_transfer(const struct can_frame *message, j1939_node_t node)
{
   j1939_source_address_t source;
   j1939_pdu_specific_t ps;

   __ASSERT_NO_MSG(message != NULL);

   if (message)
   {
      source = j1939_get_source_address(message->id);
      ps = j1939_get_pdu_specific(message->id);

      j1939_tp_process_dt(source, message->data, ps, node);
   }

   // Allow driver to handle message pointer.
   return false;
}
#endif

/**************************************************************************************************/
bool j1939_tp_connection_management(const struct can_frame *message, j1939_node_t node)
{
   __ASSERT_NO_MSG(message != NULL);

   // Process the connection management message.
   j1939_tp_process_tpcm_message(message, node);

   // Allow driver to handle message pointer.
   return false;
}
