/*
 * Copyright (c) 2026 Deere & Company
 * SPDX-License-Identifier: Apache-2.0
 */
#include "J1939Tp.h"
#include <zephyr/canbus/j1939.h>
#include "J1939Ac.h"
#include <J1939_Cfg.h>

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
#ifndef J1939TP_MILLISECONDS_BETWEEN_PACKET_GROUPS
#define J1939TP_PACKET_TIME (5)
#else
#define J1939TP_PACKET_TIME J1939TP_MILLISECONDS_BETWEEN_PACKET_GROUPS
#endif

// The following ifdefs are in for backwards compatibility reasons.  They allowed us to rename some
// defines to more appropriate names without having to go modify CANJ1939Config.h for all existing
// projects
#ifndef J1939TP_MAX_MESSAGES_QUEUED_PER_SESSION
#define J1939TP_MAX_MESSAGES_QUEUED_PER_SESSION 1
#endif

#ifndef J1939TP_MAX_TOTAL_MESSAGES_QUEUED_FOR_ALL_SESSIONS
#define J1939TP_MAX_TOTAL_MESSAGES_QUEUED_FOR_ALL_SESSIONS 255
#endif

#ifndef J1939TP_NUM_TRANSMIT_SESSIONS
#define J1939TP_NUM_TRANSMIT_SESSIONS NUMBER_OF_TP_TX_MSGS
#endif

#ifndef J1939TP_NUMBER_OF_TP_RX_SESSIONS
#define J1939TP_NUMBER_OF_TP_RX_SESSIONS NUMBER_OF_TP_RX_MSGS
#endif

extern struct j1939_dt_node_cfg* j1939_nodes[];

// This struct is used to look for buffers that aren't in use and are large
// enough to hold the incoming/outgoing data.
typedef struct J1939Tp_Buffer_S
{
   uint8_t *dataArea;
   J1939_Counter_T size;
   bool inUse;
   //lint -esym(754, reserved) Suppress not referenced warning
   // TODO Just get rid of this
   J1939_Reserved_T reserved[2]; // Uneven boundary -- currently wasted space.
#ifdef J1939_DEBUG
   J1939_Counter_T maxSize;
   J1939_Counter_T minSize;
   J1939_Counter_T totalSize;
   J1939_Counter_T allocations;
#endif
} J1939Tp_Buffer_T;

#ifndef J1939TP_RECEIVE_DISABLED

typedef struct J1939Tp_ReceiveMessageInfo_S
{
   J1939_SourceAddress_T source;               // Source Address of Transmitter
   J1939Tp_State_T state;                      // State given by TP_RX_STATE
   uint8_t lastReceivedPacket; // Current Packet Sequence Number
   uint8_t lastExpectedPacket; // The last expected Packet Sequence Number
   uint8_t packetCountDown; // Countdown on the maximum number of packets we can request at a time
   uint8_t maxRequestedPackets;           // Maximum number of packets we can request at a time
   J1939_Node_T node;                 // CAN node which is RXing this message.
   bool isBamMessage;         // Whether the message is BAM or not
   J1939_Counter_T sizeMessage;       // Size of message
   J1939_Counter_T currentByteOffset; // Currently received byte, used as offset into DataObject
   J1939_Timer_T timeFromLastReceivedPacket; // Used to determine failures on tx'r end.
   J1939_Timer_T timeFromLastCtsSent;        // Used to prevent a CTS = 0 timeout
   J1939_Counter_T dataObjectSize;           // Info on the size of the object rx'ing data
   uint8_t *incomingDataObject;       // Pointer to the area to store rx'd data
   J1939_Pgn_T pgn;                          // PGN currently being received
} J1939Tp_ReceiveMessageInfo_T;              // Transport Protocol Receive Message Information
#endif

typedef struct J1939Tp_TransmitMessageInfo_S
{
   J1939_DestinationAddress_T destination; // Address to send message to (global = BAM message)
   J1939Tp_State_T state;                  // State given by TP_TX_STATE
   J1939_Node_T node;                      // CAN node which to send this message out on.
   uint8_t totalPackets;   // Total packets to send
   uint8_t currentPacketSequenceNum; // Current packet sequence number
   uint8_t nextRequestedPacket;      // Next requested packet # as given by CTS
   uint8_t highestPacketSequenceNum; // The highest packet sequence # that we allow to be sent
   uint8_t lowestAblePacketSequenceNum; // The lowest allow packet number that is
                                        // allowed on a retransmit
   uint8_t numPacketsLeftCtsAllows;           // The number of packets left to send from given CTS
   J1939_Timer_T elapsedTimeFromLastCts;  // counter from last reception of CTS
   J1939_Timer_T elapsedTimeFromLastTpDt; // counter from last TP.DT transmission
   J1939_Pgn_T pgn;                       // PGN associated with data in dataRegion
   uint8_t *currentDataLocation;   // pointer into dataRegion
   uint8_t *dataRegion;            // pointer to data to transmit using TP
   J1939_Counter_T sizeDataRegion;        // size of pbyDataRegion
   J1939_Counter_T totalBytesToTransmit;  // Total number of bytes to transmit
   bool isWaitingBetweenCts; // Describes if this object is waiting for the first CTS after
                                     // RTS or TP.DTs, or if waiting between successive CTS's
   bool isWaitingForEomAck;  // Is waiting for end of message acknowledge
} J1939Tp_TransmitMessageInfo_T;

typedef enum J1939Tp_AbortCode_E
{
   //   TP_ABORT_CANNOT_SUPPORT_CONNECT = 1,
   //   TP_ABORT_SYSTEM_RESOURCES_NEEDED_ELSEWHERE = 2,
   J1939TP_AbortCode_Timeout = 3,
   J1939TP_AbortCode_Unknown = 255
} J1939Tp_AbortCode_T;

#ifdef J1939_DEBUG

typedef struct J1939Tp_DebugMetrics_S
{
   J1939_Counter_T numTenMsTicks;
   J1939_Counter_T numSmallBuffersUsed;
   J1939_Counter_T numMedBuffersUsed;
   J1939_Counter_T numLargeBuffersUsed;
   J1939_Counter_T numMaxBuffersUsed;
   J1939_Counter_T numFilterAcceptedMsgs;
   J1939_Counter_T numResourceAcceptedMsgs;
   J1939_Counter_T sizeOfResourceAcceptedMessages;
   J1939_Counter_T numResourceRejectedMsgs;
   J1939_Counter_T numFilteredOutMsgs;
   J1939_Counter_T maxNumSmallBuffersUsed;
   J1939_Counter_T maxNumMedBuffersUsed;
   J1939_Counter_T maxNumLargeBuffersUsed;
   J1939_Counter_T maxNumMaxBuffersUsed;
} J1939Tp_DebugMetrics_T;

J1939Tp_DebugMetrics_T J1939Tp_DebugMetrics;

#endif

// Storage buffers for transport protocol objects.
// There is support for 4 different sized buffers - small, medium, large, and max (extra
// extra EXTRA large).  They are not defined if the number of buffers is set to zero.
#if (J1939TP_NUM_SMALL_BUFFERS > 0)
static uint8_t J1939Tp_SmallBuffer[J1939TP_NUM_SMALL_BUFFERS][J1939TP_BUFFER_SMALL_SIZE];
#endif

#if (J1939TP_NUM_MEDIUM_BUFFERS > 0)
static uint8_t J1939Tp_MediumBuffer[J1939TP_NUM_MEDIUM_BUFFERS][J1939TP_BUFFER_MEDIUM_SIZE];
#endif

#if (J1939TP_NUM_LARGE_BUFFERS > 0)
static uint8_t J1939Tp_LargeBuffer[J1939TP_NUM_LARGE_BUFFERS][J1939TP_BUFFER_LARGE_SIZE];
#endif

#if (J1939TP_NUM_MAX_BUFFERS > 0)
static uint8_t J1939Tp_MaxBuffer[J1939TP_NUM_MAX_BUFFERS][J1939TP_BUFFER_MAX_SIZE];
#endif

static J1939Tp_Buffer_T J1939Tp_AllBuffers[J1939TP_NUM_ALL_BUFFERS];

static J1939Tp_TransmitMessageInfo_T J1939Tp_TransmitMessageList[J1939TP_NUM_TRANSMIT_SESSIONS];

#ifndef J1939TP_RECEIVE_DISABLED
static J1939Tp_ReceiveMessageInfo_T
    J1939Tp_ReceiveMessageList[J1939TP_NUMBER_OF_TP_RX_SESSIONS]; // contains info on
                                                                  // incoming messages

static J1939_Counter_T J1939Tp_NumOpenReceiveConnections;
static J1939_Counter_T J1939Tp_NumHoldingMessages;
#endif

static J1939_ByteCounter_T J1939Tp_NumOpenTransmitConnections;

/// @brief Attempt to allocate buffer large enough to hold the passed size
/// @param sizeMessage Size of message bytes needed
/// @param buffer Pointer to update with allocated buffer pointer
/// @param sizeBuffer Pointer to update with size of allocated buffer
/// @return True on success, false if not
static bool J1939Tp_BufferAllocate(J1939_Counter_T sizeMessage, uint8_t **buffer,
                                   J1939_Counter_T *sizeBuffer);

/// @brief Buils and sends a connector abort message on the requested node
/// @param abortCode Abort code
/// @param pgn PGN of message to abort
/// @param source Source address of the unit to receive the abort message
/// @param node J1939 on which to send the abort
static void J1939Tp_TransmitConnectionAbort(J1939Tp_AbortCode_T abortCode, J1939_Pgn_T pgn,
                                            J1939_SourceAddress_T source, J1939_Node_T node);
#ifndef J1939TP_RECEIVE_DISABLED
/// @brief Checks if PGN is registered
/// @param pgn PGN to check
/// @param node J1939 node which to check
/// @return True if PGN is registered, false if not
static bool J1939Tp_IsPgnSupported(J1939_Pgn_T pgn, J1939_Node_T node);

/// @brief Register routing PGN to proper callback
/// @param pgn PGN to register
/// @param data Received data
/// @param length Number of bytes in `data`
/// @param source Source of message sender
/// @param node J1939 node message was received on
/// @return True if callback was called, false if not.
static bool J1939Tp_RegisterUserCallback(J1939_Pgn_T pgn, uint8_t *data,
                                         J1939_Counter_T length,
                                         J1939_SourceAddress_T source, J1939_Node_T node);

/// @brief Cancels existing receive session and frees he associated receive buffer
/// @param index Index in to receive list
/// @param source Source address of sender
/// @param node J!939 node message was received on
/// @param isBam True if BAM message, false if not
/// @return True if session was cancelled, false if not.
static bool J1939Tp_CancelOldReceiveSessions(J1939_Counter_T index,
                                             J1939_SourceAddress_T source,
                                             J1939_Node_T node, bool isBam);
#endif

/// @brief Processes CTS messages received by the RHCP in response to RTS messages sent.
/// @param source Source address of unit transmitting the CTS
/// @param data CTS message data
/// @param node J1939 node message was received on
static void J1939Tp_ProcessCts(J1939_SourceAddress_T source, const uint8_t *data,
                               J1939_Node_T node);

/// @brief Processes incoming End of Message Acknowledgements
/// @param source Source address of the incoming EOMAck message
/// @param data Data bytes of the message
/// @param node J1939 node message was received on
static void J1939Tp_ProcessEomAck(J1939_SourceAddress_T source, const uint8_t *data,
                                  J1939_Node_T node);

/// @brief Processes incoming Connection Abort messages received by the RHCP
/// @param source Source address of the node transmitting the connection abort
/// @param data Pointer to the connection abort message
/// @param node J1939 node message was received on
static void J1939Tp_ProcessAbort(J1939_SourceAddress_T source, const uint8_t *data,
                                 J1939_Node_T node);
#ifndef J1939TP_RECEIVE_DISABLED
/// @brief Processes BAM and RTS messages received by the RHCP. Determines if the message is a BAM
/// or RTS and then checks registered PGNs.
/// @note Rejects message if its a duplicate of a current session and determines if there is a large
/// enough object for available reception.
/// @param control PDU Format of message (TP.CM_BAM or TP.CM_RTS)
/// @param source Source address of the node sending the message
/// @param data Data bytes for message
/// @param node J1939 on which to process
static void J1939Tp_ProcessBamRts(J1939_PduFormat_T control, J1939_SourceAddress_T source,
                                  const uint8_t *data, J1939_Node_T node);

/// @brief Generates a CTS message to the specified `destination`.
/// @param destination Source address of where message should be sent.
/// @param index Index in message list for the session sending the CTS
/// @param packetNum Next expected packet number.
static void J1939Tp_TransmitCts(J1939_DestinationAddress_T destination, J1939_Counter_T index,
                                uint8_t packetNum);
#endif

/// Macro to simplify handle initializing a buffer
#define J1939TP_BUFFER_INIT(buffer, numBuffers, bufferSize)                                        \
   {                                                                                               \
      for (J1939_Counter_T i = 0; i < numBuffers; i++)                                             \
      {                                                                                            \
         for (J1939_Counter_T j = 0; j < bufferSize; j++)                                          \
         {                                                                                         \
            buffer[i][j] = 0;                                                                      \
         }                                                                                         \
      }                                                                                            \
   }

#ifdef J1939_DEBUG
/// @brief Helps analyze network traffic the RHCP sees.
/// @note This information can be used to tailor the number of buffers being used.
static void J1939Tp_TimingAnalysis(void);
#endif

/**************************************************************************************************/
void J1939Tp_Init(void)
{
   J1939_Counter_T allIndex = 0;

#ifndef J1939TP_RECEIVE_DISABLED
   for (uint8_t node_index = 0; node_index < J1939_NUM_NODES; node_index++)
   {
      J1939_Node_T node = j1939_nodes[node_index];
      node->J1939Tp_RegisterPgnIndex = 0;

      for (uint8_t receivePgn = 0; receivePgn < CONFIG_J1939TP_NUM_ALLOWED_RECEIVE_PGN; receivePgn++)
      {
         node->J1939Tp_RegisterPgnList[receivePgn].pgn = UINT32_MAX;
         node->J1939Tp_RegisterPgnList[receivePgn].callback = NULL;
      }
   }
#endif

   // Leave the order of assignments in the following for-loops  as they are.  The algorithm to
   // locate a free buffer is first-fit.
#if (J1939TP_NUM_SMALL_BUFFERS > 0)

   J1939TP_BUFFER_INIT(J1939Tp_SmallBuffer, J1939TP_NUM_SMALL_BUFFERS, J1939TP_BUFFER_SMALL_SIZE);
   for (J1939_Counter_T i = 0; i < J1939TP_NUM_SMALL_BUFFERS; i++)
   {
      J1939Tp_AllBuffers[allIndex].dataArea = J1939Tp_SmallBuffer[i];
      J1939Tp_AllBuffers[allIndex].size = J1939TP_BUFFER_SMALL_SIZE;
      J1939Tp_AllBuffers[allIndex++].inUse = false;
   }
#endif

#if (J1939TP_NUM_MEDIUM_BUFFERS > 0)

   //lint !e2454
   J1939TP_BUFFER_INIT(J1939Tp_MediumBuffer, J1939TP_NUM_MEDIUM_BUFFERS,
                       J1939TP_BUFFER_MEDIUM_SIZE);

   for (J1939_Counter_T i = 0; i < J1939TP_NUM_MEDIUM_BUFFERS; i++)
   {
      J1939Tp_AllBuffers[allIndex].dataArea = J1939Tp_MediumBuffer[i];
      J1939Tp_AllBuffers[allIndex].size = J1939TP_BUFFER_MEDIUM_SIZE;
      J1939Tp_AllBuffers[allIndex++].inUse = false;
   }
#endif

#if (J1939TP_NUM_LARGE_BUFFERS > 0)

   J1939TP_BUFFER_INIT(J1939Tp_LargeBuffer, J1939TP_NUM_LARGE_BUFFERS, J1939TP_BUFFER_LARGE_SIZE);

   for (J1939_Counter_T i = 0; i < J1939TP_NUM_LARGE_BUFFERS; i++)
   {
      J1939Tp_AllBuffers[allIndex].dataArea = J1939Tp_LargeBuffer[i];
      J1939Tp_AllBuffers[allIndex].size = J1939TP_BUFFER_LARGE_SIZE;
      J1939Tp_AllBuffers[allIndex++].inUse = false;
   }
#endif

#if (J1939TP_NUM_MAX_BUFFERS > 0)

   J1939TP_BUFFER_INIT(J1939Tp_MaxBuffer, J1939TP_NUM_MAX_BUFFERS, J1939TP_BUFFER_MAX_SIZE);

   for (J1939_Counter_T i = 0; i < J1939TP_NUM_MAX_BUFFERS; i++)
   {
      J1939Tp_AllBuffers[allIndex].dataArea = J1939Tp_MaxBuffer[i];
      J1939Tp_AllBuffers[allIndex].size = J1939TP_BUFFER_MAX_SIZE;
      J1939Tp_AllBuffers[allIndex++].inUse = false;
   }
#endif

#ifndef J1939TP_RECEIVE_DISABLED
   for (J1939_Counter_T tpRx = 0; tpRx < J1939TP_NUMBER_OF_TP_RX_SESSIONS; tpRx++)
   {
      J1939Tp_ReceiveMessageList[tpRx].state = J1939Tp_State_Available;
   }
#endif

   for (J1939_Counter_T transmit = 0; transmit < J1939TP_NUM_TRANSMIT_SESSIONS; transmit++)
   {
      J1939Tp_TransmitMessageList[transmit].state = J1939Tp_State_Available;
   }

#ifndef J1939TP_RECEIVE_DISABLED
   J1939Tp_NumHoldingMessages = 0;
   J1939Tp_NumOpenReceiveConnections = 0;
#endif
   J1939Tp_NumOpenTransmitConnections = 0;

#ifdef J1939_DEBUG
   for (J1939_Counter_T all = 0; all < J1939TP_NUM_ALL_BUFFERS; all++)
   {
      J1939Tp_AllBuffers[all].maxSize = 0;
      J1939Tp_AllBuffers[all].minSize = 0xFFFF;
      J1939Tp_AllBuffers[all].totalSize = 0;
      J1939Tp_AllBuffers[all].allocations = 0;
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

#ifndef J1939TP_RECEIVE_DISABLED
/**************************************************************************************************/
void J1939Tp_UpdateReceiveMessageTimes(J1939_Timer_T callPeriod)
{
   J1939_Counter_T index1;
   J1939_Counter_T index2;
   bool check;
   CriticalSection_Lock();

   // Loop through the rx transport object list
   index2 = J1939Tp_NumOpenReceiveConnections;
   for (index1 = 0; index2 && (index1 < J1939TP_NUMBER_OF_TP_RX_SESSIONS); index1++)
   {
      if (J1939Tp_ReceiveMessageList[index1].state == J1939Tp_State_Receiving)
      {
         index2--;

         //  Increment the time from the last received packet.
         J1939Tp_ReceiveMessageList[index1].timeFromLastReceivedPacket += callPeriod;

         if (J1939Tp_ReceiveMessageList[index1].timeFromLastReceivedPacket > J1939TP_T1_TIMEOUT)
         {
            if (!J1939Tp_ReceiveMessageList[index1].isBamMessage)
            {
               // Send TP Connection Abort!
               J1939Tp_TransmitConnectionAbort(J1939TP_AbortCode_Timeout,
                                               J1939Tp_ReceiveMessageList[index1].pgn,
                                               J1939Tp_ReceiveMessageList[index1].source,
                                               J1939Tp_ReceiveMessageList[index1].node);
            }

            // Free up the buffer that this TP Rx object was using
            J1939Tp_FreeBuffer(J1939Tp_ReceiveMessageList[index1].incomingDataObject);

            J1939Tp_NumOpenReceiveConnections--;

            // Disable message reception and ignore whatever was received
            J1939Tp_ReceiveMessageList[index1].state = J1939Tp_State_Available;
         }
      }
      else if (J1939Tp_ReceiveMessageList[index1].state == J1939Tp_State_Waiting)
      {
         index2--;
         J1939Tp_ReceiveMessageList[index1].timeFromLastCtsSent += callPeriod;

         if (J1939Tp_ReceiveMessageList[index1].timeFromLastCtsSent >= J1939TP_T2_TIMEOUT)
         {
            // kill the connection since tx'r did not send data
            if (!J1939Tp_ReceiveMessageList[index1].isBamMessage)
            {
               // Send TP Connection Abort!
               J1939Tp_TransmitConnectionAbort(J1939TP_AbortCode_Timeout,
                                               J1939Tp_ReceiveMessageList[index1].pgn,
                                               J1939Tp_ReceiveMessageList[index1].source,
                                               J1939Tp_ReceiveMessageList[index1].node);
            }

            // Free up the buffer that this TP Rx object was using
            J1939Tp_FreeBuffer(J1939Tp_ReceiveMessageList[index1].incomingDataObject);
            J1939Tp_NumOpenReceiveConnections--;

            // Disable message reception and ignore whatever was received
            J1939Tp_ReceiveMessageList[index1].state = J1939Tp_State_Available;
         }
      }
   }

   CriticalSection_Unlock();

   // We unlock, then relock to reduce latency

   CriticalSection_Lock();

   // Function callback code.
   index2 = J1939Tp_NumHoldingMessages;
   for (index1 = 0; index2 && (index1 < J1939TP_NUMBER_OF_TP_RX_SESSIONS); index1++)
   {
      if (J1939Tp_ReceiveMessageList[index1].state == J1939Tp_State_Holding)
      {
         // VCallback user function
         check = J1939Tp_RegisterUserCallback(J1939Tp_ReceiveMessageList[index1].pgn,
                                              J1939Tp_ReceiveMessageList[index1].incomingDataObject,
                                              J1939Tp_ReceiveMessageList[index1].sizeMessage,
                                              J1939Tp_ReceiveMessageList[index1].source,
                                              J1939Tp_ReceiveMessageList[index1].node);

         if (check)
         {
            // Free up resources after messages were processed.
            J1939Tp_FreeBuffer(J1939Tp_ReceiveMessageList[index1].incomingDataObject);
            J1939Tp_NumHoldingMessages--;
            J1939Tp_ReceiveMessageList[index1].state = J1939Tp_State_Available;
         }
         // else message not registered for call backs.
      }
   }
   CriticalSection_Unlock();
}
#endif

/**************************************************************************************************/
void J1939Tp_UpdateSendMessageTimes(J1939_Timer_T callPeriod)
{
   uint16_t index1;
   J1939_ByteCounter_T index2;
   J1939_ByteCounter_T index3;
   uint8_t data[CAN_MAX_DLC];
   uint32_t id;
   J1939_Timer_T deltaTime;
   uint16_t totalMessagesQueued = 0;
   uint16_t totalMessagesQueuedThisSession;
   uint8_t *currentDataLocation;
   CriticalSection_Lock();

   index3 = J1939Tp_NumOpenTransmitConnections; // short circuit if there are none
   for (index1 = 0; index3 && (index1 < J1939TP_NUM_TRANSMIT_SESSIONS); index1++)
   {
      //  If the current message is in use...
      if (J1939Tp_TransmitMessageList[index1].state != J1939Tp_State_Available)
      {
         // Update elapsed time counters:
         J1939Tp_TransmitMessageList[index1].elapsedTimeFromLastCts += callPeriod;
         J1939Tp_TransmitMessageList[index1].elapsedTimeFromLastTpDt += callPeriod;

         index3--;
         // If object is setup as destination specific...
         if (J1939Tp_TransmitMessageList[index1].destination != J1939_GLOBAL_ADDRESS)
         {
            // If we've sent out the CTS allowed number of packets...
            if (J1939Tp_TransmitMessageList[index1].numPacketsLeftCtsAllows == 0)
            {
               // If we're not completely done transmitting and are simply waiting for
               // the receiver to send us a CTS or EndofMsgACK...
               if (J1939Tp_TransmitMessageList[index1].currentDataLocation <
                   (J1939Tp_TransmitMessageList[index1].dataRegion +
                    (J1939Tp_TransmitMessageList[index1].totalBytesToTransmit)))
               {
                  // Determine the time since the last received CTS
                  deltaTime = J1939Tp_TransmitMessageList[index1].elapsedTimeFromLastCts;

                  // If we just finished sending the last allowed TP.DT and haven't
                  //       received a follow-up CTS in time or
                  //    we have received a CTS w/packets to send == 0 and haven't
                  //       received a subsequent CTS in time...
                  if (((J1939Tp_TransmitMessageList[index1].isWaitingBetweenCts) &&
                       (deltaTime > J1939TP_T4_TIMEOUT)) ||
                      ((!J1939Tp_TransmitMessageList[index1].isWaitingBetweenCts) &&
                       (deltaTime > J1939TP_T3_TIMEOUT)))
                  {
                     // Didn't get a CTS in time so disable this object and send a connection abort.
                     J1939Tp_TransmitConnectionAbort(
                         J1939TP_AbortCode_Timeout, J1939Tp_TransmitMessageList[index1].pgn,
                         J1939Tp_TransmitMessageList[index1].destination,
                         J1939Tp_TransmitMessageList[index1].node);

                     // Free up the buffer that this TP Rx object was using
                     J1939Tp_NumOpenTransmitConnections--;
                     J1939Tp_FreeBuffer(J1939Tp_TransmitMessageList[index1].dataRegion);
                     J1939Tp_TransmitMessageList[index1].state = J1939Tp_State_Available;
                  }
               }
               else
               {
                  // We are waiting for a TP.CM_EndofMsgACK
                  // An abort should be sent if an EOMAck hasn't been received in a timely manner
                  // since the last packet of the connection was transmitted or the last received
                  // CTS (whichever occurred most recently.)
                  if (J1939Tp_TransmitMessageList[index1].elapsedTimeFromLastTpDt <
                      J1939Tp_TransmitMessageList[index1].elapsedTimeFromLastCts)
                  {
                     /* TP.DT sent is more recent than the last received CTS */
                     deltaTime = J1939Tp_TransmitMessageList[index1].elapsedTimeFromLastTpDt;
                  }
                  else
                  {
                     // The last CTS was received more recently than the send of the last
                     // packet of the conneciton.
                     deltaTime = J1939Tp_TransmitMessageList[index1].elapsedTimeFromLastCts;
                  }

                  //  If we don't receive an EndofMsgACK in time...
                  if (deltaTime > J1939TP_T3_TIMEOUT)
                  {
                     // Didn't get a EOM in time so disable this object and
                     // send a connection abort.
                     J1939Tp_TransmitConnectionAbort(
                         J1939TP_AbortCode_Timeout, J1939Tp_TransmitMessageList[index1].pgn,
                         J1939Tp_TransmitMessageList[index1].destination,
                         J1939Tp_TransmitMessageList[index1].node);

                     // Free up the buffer that this TP Rx object was using
                     J1939Tp_NumOpenTransmitConnections--;
                     J1939Tp_FreeBuffer(J1939Tp_TransmitMessageList[index1].dataRegion);
                     J1939Tp_TransmitMessageList[index1].state = J1939Tp_State_Available;
                  }
               }
            }
            else
            {
#if (J1939TP_PACKET_TIME != 0)
               // This is a regular TP.DT messageDetermine if it is time to send the next packet out
               deltaTime = J1939Tp_TransmitMessageList[index1].elapsedTimeFromLastTpDt;

               if (deltaTime >= J1939TP_PACKET_TIME)
#endif
               {
                  // If a CTS indicated that it doesn't want us to start
                  // at the next indicated packet...
                  if (J1939Tp_TransmitMessageList[index1].nextRequestedPacket)
                  {
                     // Recalculate location in the transmit buffer and
                     // overwrite the current packet sequence number.
                     J1939Tp_TransmitMessageList[index1].currentDataLocation =
                         J1939Tp_TransmitMessageList[index1].dataRegion +
                         (7 * (J1939Tp_TransmitMessageList[index1].nextRequestedPacket -
                               1)); //lint !e679
                     J1939Tp_TransmitMessageList[index1].currentPacketSequenceNum =
                         J1939Tp_TransmitMessageList[index1].nextRequestedPacket;
                     J1939Tp_TransmitMessageList[index1].nextRequestedPacket = 0;
                  }

                  for (totalMessagesQueuedThisSession = 0;
                       (totalMessagesQueuedThisSession < J1939TP_MAX_MESSAGES_QUEUED_PER_SESSION) &&
                       J1939Tp_TransmitMessageList[index1].numPacketsLeftCtsAllows &&
                       (totalMessagesQueued < J1939TP_MAX_TOTAL_MESSAGES_QUEUED_FOR_ALL_SESSIONS);
                       totalMessagesQueuedThisSession++)
                  {
                     data[0] =
                         J1939Tp_TransmitMessageList[index1].currentPacketSequenceNum++; // packet #

                     if (J1939Tp_TransmitMessageList[index1].currentPacketSequenceNum >
                         J1939Tp_TransmitMessageList[index1].highestPacketSequenceNum)
                     {
                        J1939Tp_TransmitMessageList[index1].highestPacketSequenceNum =
                            J1939Tp_TransmitMessageList[index1].currentPacketSequenceNum;
                     }

                     // Store off the existing pointer in case we fail to queue the message
                     currentDataLocation = J1939Tp_TransmitMessageList[index1].currentDataLocation;

                     // Copy over the 7 bytes of data to include in the packet.
                     for (index2 = 1; index2 < CAN_MAX_DLC; index2++)
                     {
                        //  If there is still data in the buffer to send...
                        if (J1939Tp_TransmitMessageList[index1].currentDataLocation <
                            (J1939Tp_TransmitMessageList[index1].dataRegion +
                             J1939Tp_TransmitMessageList[index1].totalBytesToTransmit))
                        {
                           // Load data to transmit
                           data[index2] =
                               *(J1939Tp_TransmitMessageList[index1].currentDataLocation)++;
                        }
                        else
                        {
                           // Default to J1939_BYTE_UNAVAILABLE
                           data[index2] = J1939_BYTE_UNAVAILABLE;
                        }
                     }

                     // Build the message ID
                     id = J1939_BuildMessageId(
                         0, 0, J1939TP_PRIORITY,
                         J1939_BuildPgnFromPdu(J1939_TP_DATA_TRANSFER_PF,
                                               J1939Tp_TransmitMessageList[index1].destination),
                         J1939Tp_TransmitMessageList[index1].node->source_address);

                     // Send the message out to the appropriate node.
                     if (!J1939_BuildAndQueueMessage(J1939Tp_TransmitMessageList[index1].node, id,
                                                     CAN_MAX_DLC, true,
                                                     data))
                     {
                        J1939Tp_TransmitMessageList[index1].currentPacketSequenceNum--;
                        J1939Tp_TransmitMessageList[index1].currentDataLocation =
                            currentDataLocation;
                        // Leave the for(totalMessagesQueuedThisSession...) loop right now since we
                        // cannot queue more messages
                        break;
                     }

                     J1939Tp_TransmitMessageList[index1].numPacketsLeftCtsAllows--;
                     J1939Tp_TransmitMessageList[index1].elapsedTimeFromLastTpDt = 0;

                     /*  If was message entirely transmitted...  */
                     if (J1939Tp_TransmitMessageList[index1].currentDataLocation ==
                         (J1939Tp_TransmitMessageList[index1].dataRegion +
                          J1939Tp_TransmitMessageList[index1].totalBytesToTransmit))
                     {
                        J1939Tp_TransmitMessageList[index1].isWaitingForEomAck = true;

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
            deltaTime = J1939Tp_TransmitMessageList[index1].elapsedTimeFromLastTpDt;

            // If it is time to send out the BAM message
            if (deltaTime >= J1939TP_BAM_TIME)
            {
               data[0] = J1939Tp_TransmitMessageList[index1].currentPacketSequenceNum++;
               for (index2 = 1; index2 < CAN_MAX_DLC; index2++)
               {
                  // If there is still data in the buffer to send...
                  if (J1939Tp_TransmitMessageList[index1].currentDataLocation <
                      (J1939Tp_TransmitMessageList[index1].dataRegion +
                       J1939Tp_TransmitMessageList[index1].totalBytesToTransmit))
                  {
                     //  Load data into transmit object
                     data[index2] = *(J1939Tp_TransmitMessageList[index1].currentDataLocation)++;
                  }
                  else
                  {
                     // Default to J1939_BYTE_UNAVAILABLE
                     data[index2] = J1939_BYTE_UNAVAILABLE;
                  }
               }

               // Build the message ID
               id = J1939_BuildMessageId(
                   0, 0, J1939TP_PRIORITY,
                   J1939_BuildPgnFromPdu(J1939_TP_DATA_TRANSFER_PF, J1939_GLOBAL_ADDRESS),
                   J1939Tp_TransmitMessageList[index1].node->source_address);

               // Send the message out to the appropriate node.
               (void)J1939_BuildAndQueueMessage(J1939Tp_TransmitMessageList[index1].node, id,
                                                CAN_MAX_DLC, true, data);

               //  If there is nothing left to send...
               if (J1939Tp_TransmitMessageList[index1].currentDataLocation ==
                   (J1939Tp_TransmitMessageList[index1].dataRegion +
                    J1939Tp_TransmitMessageList[index1].totalBytesToTransmit))
               {
                  //  Completed BAM transmission...disable object
                  J1939Tp_NumOpenTransmitConnections--;

                  // Free up the buffer that this TP Rx object was using
                  J1939Tp_FreeBuffer(J1939Tp_TransmitMessageList[index1].dataRegion);
                  J1939Tp_TransmitMessageList[index1].state = J1939Tp_State_Available;
                  if (J1939Tp_TransmitMessageList[index1].node < J1939_NUM_NODES)
                  {
                     J1939Tp_TransmitMessageList[index1].node->J1939Tp_TransmitBam = false;
                  }
               }
               else
               {
                  // Reset the elapsed time with last tx time
                  J1939Tp_TransmitMessageList[index1].elapsedTimeFromLastTpDt = 0;
               }
            }
         }
      }
   }
   CriticalSection_Unlock();

#ifdef J1939_DEBUG
   J1939Tp_TimingAnalysis();
#endif
}

#ifndef J1939TP_RECEIVE_DISABLED
/**************************************************************************************************/
bool J1939Tp_RegisterMessageCallback(J1939_Pgn_T pgn, J1939_Node_T node,
                                             J1939Tp_Callback_T callback)
{
   uint16_t index;
   bool result = true;
   CriticalSection_Lock();

   for (index = 0;
        (index < node->J1939Tp_RegisterPgnIndex) && (index < CONFIG_J1939TP_NUM_ALLOWED_RECEIVE_PGN);
        index++)
   {
      if (node->J1939Tp_RegisterPgnList[index].pgn == pgn)
      {
         // Update function pointer & return.
         node->J1939Tp_RegisterPgnList[index].callback = callback;

         // Exit the critical section and return to avoid below checks
         CriticalSection_Unlock();
         return true;
      }
   }

   if (index < CONFIG_J1939TP_NUM_ALLOWED_RECEIVE_PGN)
   {
      // add PGN to the list and increment the index
      node->J1939Tp_RegisterPgnList[node->J1939Tp_RegisterPgnIndex].pgn = pgn;
      node->J1939Tp_RegisterPgnList[node->J1939Tp_RegisterPgnIndex++].callback = callback;
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
bool J1939Tp_RegisterExtendedLengthMessages(J1939_Pgn_T pgn, J1939_Node_T node)
{
   J1939_Counter_T index;

   if (node < J1939_NUM_NODES)
   {
      CriticalSection_Lock();
      for (index = 0;
           (index < node->J1939Tp_RegisterPgnIndex) && (index < CONFIG_J1939TP_NUM_ALLOWED_RECEIVE_PGN);
           index++)
      {
         if (node->J1939Tp_RegisterPgnList[index].pgn == pgn)
         {
            // this PGN is already in the list
            CriticalSection_Unlock();
            return true;
         }
      }

      if (index < CONFIG_J1939TP_NUM_ALLOWED_RECEIVE_PGN)
      {
         // add PGN to the list and increment the index
         if (node->J1939Tp_RegisterPgnIndex < CONFIG_J1939TP_NUM_ALLOWED_RECEIVE_PGN)
         {
            node->J1939Tp_RegisterPgnList[node->J1939Tp_RegisterPgnIndex++].pgn = pgn;
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
bool J1939Tp_TransmitSessionExists(J1939_SourceAddress_T source, J1939_Node_T node)
{
   uint16_t index;
   bool isFound = false;
   if (source != J1939_GLOBAL_ADDRESS)
   {
      CriticalSection_Lock();
      for (index = 0; index < J1939TP_NUM_TRANSMIT_SESSIONS; index++)
      {
         if (J1939Tp_TransmitMessageList[index].state != J1939Tp_State_Available)
         {
            if ((J1939Tp_TransmitMessageList[index].destination == source) &&
                (J1939Tp_TransmitMessageList[index].node == node))
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
uint8_t *J1939Tp_GetBuffer(J1939_Counter_T sizeMessage)
{
   uint8_t *buffer = NULL;
   J1939_Counter_T actualBufferSize;
   CriticalSection_Lock();
   // Check if we allocated the required buffer space.
   if (!J1939Tp_BufferAllocate(sizeMessage, &buffer, &actualBufferSize))
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
void J1939Tp_ProcessTpcmMessage(const struct can_frame *message, J1939_Node_T node)
{
   J1939_Pgn_T pgn;

   if (message)
   {
      // Route data to appropriate function based on control byte
      switch (message->data[0])
      {
#ifndef J1939TP_RECEIVE_DISABLED
      case J1939TP_CM_BAM: // for TP rx messages
      case J1939TP_CM_RTS: // for TP rx messages
         J1939Tp_ProcessBamRts(message->data[0], J1939_GetSourceAddress(message->id),
                               message->data, node);
         break;
#endif

      case J1939TP_CM_CTS: // for TP tx messages
         J1939Tp_ProcessCts(J1939_GetSourceAddress(message->id), message->data,
                            node);
         break;

      case J1939TP_CM_EOMACK: // for TP tx messages
         J1939Tp_ProcessEomAck(J1939_GetSourceAddress(message->id), message->data,
                               node);
         break;

      case J1939TP_CONNECTION_ABORT: // for TP tx messages
         J1939Tp_ProcessAbort(J1939_GetSourceAddress(message->id), message->data,
                              node);
         break;

      case J1939TP_CM_ERTS:
         pgn =
             MAKEDWORD(MAKEWORD(message->data[5], message->data[6]), MAKEWORD(message->data[7], 0));
         J1939Tp_TransmitConnectionAbort(J1939TP_AbortCode_Unknown, pgn,
                                         J1939_GetSourceAddress(message->id),
                                         node);
         break;

      default:
         break;
      }
   }
}

/**************************************************************************************************/
J1939Tp_Message_T J1939Tp_TransmitMultiPacket(J1939_Pgn_T pgn,
                                              J1939_DestinationAddress_T destination,
                                              J1939_Counter_T dataLength, uint8_t *data,
                                              J1939_Node_T node)
{
   J1939_Counter_T txSessionIndex;
   J1939_Counter_T bufferIndex;
   uint8_t txData[9];
   J1939_Arbitration_T id;
   J1939Tp_Message_T result;
   bool isValidBuffer;

   __ASSERT_NO_MSG(data != NULL);

   result = J1939Tp_Message_NotAccepted;

   // If invalid data length requested  or this is a BAM request and
   // we are already transmitting a BAM msg..
   if (!(((dataLength > J1939TP_MAX_BYTES) || (dataLength < J1939TP_MIN_BYTES)) ||
         ((destination == J1939_GLOBAL_ADDRESS) && node->J1939Tp_TransmitBam)))
   {
      // if Tx Session already exists on that node.
      if (J1939Tp_TransmitSessionExists(destination, node))
      {
         J1939Tp_FreeBuffer(data);
      }
      else
      {
         // Search through TP messages to see if we can load and send out that message
         CriticalSection_Lock();
         for (txSessionIndex = 0; txSessionIndex < J1939TP_NUM_TRANSMIT_SESSIONS; txSessionIndex++)
         {
            if (J1939Tp_TransmitMessageList[txSessionIndex].state == J1939Tp_State_Available)
            {
               // See if byte pointer matches one of our already allocated buffers.  If so, then
               // we'll use it to send the message. Else we'll allocate a buffer and copy the data
               isValidBuffer = false;
               for (bufferIndex = 0; bufferIndex < J1939TP_NUM_ALL_BUFFERS; bufferIndex++)
               {
                  if (J1939Tp_AllBuffers[bufferIndex].dataArea == data)
                  {
                     // This is a match. Use the already allocated buffer and break out of this loop
                     // In this case we know we can safely cast away the  since all transport
                     // buffers are writable
                     J1939Tp_TransmitMessageList[txSessionIndex].dataRegion =
                         (uint8_t *)data;
                     J1939Tp_TransmitMessageList[txSessionIndex].sizeDataRegion =
                         J1939Tp_AllBuffers[bufferIndex].size;
                     isValidBuffer = true;
                     break;
                  }
               }

               if (!isValidBuffer)
               {
                  isValidBuffer = J1939Tp_BufferAllocate(
                      dataLength, &J1939Tp_TransmitMessageList[txSessionIndex].dataRegion,
                      &J1939Tp_TransmitMessageList[txSessionIndex].sizeDataRegion);

                  if (isValidBuffer)
                  {
                     // Yep, we've got a TP that we can use for transmit
                     // store *packetData to associated TP PGN object
                     for (bufferIndex = 0; bufferIndex < dataLength; bufferIndex++)
                     {

                        *((J1939Tp_TransmitMessageList[txSessionIndex].dataRegion) + bufferIndex) =
                            *(data + bufferIndex);
                     }
                  }
               }

               if (isValidBuffer)
               {
                  J1939Tp_TransmitMessageList[txSessionIndex].currentDataLocation =
                      J1939Tp_TransmitMessageList[txSessionIndex].dataRegion;

                  // Determine # of 7 byte data packets to send
                  J1939Tp_TransmitMessageList[txSessionIndex].totalPackets =
                      (uint8_t)(dataLength / 7);

                  // Add a packet if there are any straggling bytes
                  if (dataLength % 7)
                  {
                     J1939Tp_TransmitMessageList[txSessionIndex].totalPackets++;
                  }

                  // Set up TP message structure
                  J1939Tp_TransmitMessageList[txSessionIndex].elapsedTimeFromLastCts = 0;
                  J1939Tp_TransmitMessageList[txSessionIndex].elapsedTimeFromLastTpDt = 0;
                  J1939Tp_TransmitMessageList[txSessionIndex].destination = destination;
                  J1939Tp_TransmitMessageList[txSessionIndex].currentPacketSequenceNum = 1;
                  J1939Tp_TransmitMessageList[txSessionIndex].nextRequestedPacket = 0;
                  J1939Tp_TransmitMessageList[txSessionIndex].highestPacketSequenceNum = 1;
                  J1939Tp_TransmitMessageList[txSessionIndex].lowestAblePacketSequenceNum = 0;
                  J1939Tp_TransmitMessageList[txSessionIndex].totalBytesToTransmit = dataLength;
                  J1939Tp_TransmitMessageList[txSessionIndex].numPacketsLeftCtsAllows = 0;
                  J1939Tp_TransmitMessageList[txSessionIndex].pgn = pgn;
                  J1939Tp_TransmitMessageList[txSessionIndex].state = J1939Tp_State_Sending;
                  J1939Tp_TransmitMessageList[txSessionIndex].isWaitingForEomAck = false;
                  J1939Tp_TransmitMessageList[txSessionIndex].isWaitingBetweenCts = false;
                  J1939Tp_TransmitMessageList[txSessionIndex].node = node;

                  // Send RTS to initiate transmit & Build message Data
                  if (destination == J1939_GLOBAL_ADDRESS)
                  {
                     txData[0] = J1939TP_CM_BAM;
                     txData[4] = J1939_BYTE_UNAVAILABLE;
                     node->J1939Tp_TransmitBam = true;
                  }
                  else
                  {
                     txData[0] = J1939TP_CM_RTS;
                     /* we can handle a request for all the packets */
                     txData[4] = J1939Tp_TransmitMessageList[txSessionIndex].totalPackets;
                  }

                  txData[1] = LOBYTE(dataLength);
                  txData[2] = HIBYTE(dataLength);
                  txData[3] = J1939Tp_TransmitMessageList[txSessionIndex].totalPackets;
                  txData[5] = LOBYTE(LOWORD(pgn));
                  txData[6] = HIBYTE(LOWORD(pgn));
                  txData[7] = LOBYTE(HIWORD(pgn));

                  // Build the message ID
                  id = J1939_BuildMessageId(
                      0, 0, J1939TP_PRIORITY,
                      J1939_BuildPgnFromPdu(J1939_TP_CONN_MANAGEMENT_PF, destination),
                      J1939Tp_TransmitMessageList[txSessionIndex].node->source_address);

                  // Send the message out to the appropriate node.
                  (void)J1939_BuildAndQueueMessage(J1939Tp_TransmitMessageList[txSessionIndex].node,
                                                   id, CAN_MAX_DLC, true,
                                                   txData);

                  result = J1939Tp_Message_Accepted;
                  J1939Tp_NumOpenTransmitConnections++;
                  break;
               }
            }
         }
         CriticalSection_Unlock();

         // Were we able to transmit?
         if (txSessionIndex >= J1939TP_NUM_TRANSMIT_SESSIONS)
         {
            // No TP Tx Message Available. Free up any allocated buffer.
            J1939Tp_FreeBuffer(data);
         }
      }
   }
   // ELSE, the message is invalid or we are already transmitting a BAM.
   else
   {
      // Free up any allocated buffer.
      J1939Tp_FreeBuffer(data);
   }

   return (result);
}

#ifndef J1939TP_RECEIVE_DISABLED
/**************************************************************************************************/
void J1939Tp_ProcessDt(J1939_SourceAddress_T source, const uint8_t *packetData,
                       J1939_DestinationAddress_T destination, J1939_Node_T node)
{
   J1939_Counter_T rxSessionIndex;
   J1939_Arbitration_T id;
   uint8_t sequenceNum;
   uint8_t data[9];
   CriticalSection_Lock();
   // Determine if TP.DT message is being listened to...
   for (rxSessionIndex = 0; rxSessionIndex < J1939TP_NUMBER_OF_TP_RX_SESSIONS; rxSessionIndex++)
   {
      // if the TP Rx resource is being used and the transmitter's address matches the address
      // we're getting the TP.DT from and it is on the same CAN node...
      if (((J1939Tp_ReceiveMessageList[rxSessionIndex].state == J1939Tp_State_Receiving) ||
           (J1939Tp_ReceiveMessageList[rxSessionIndex].state == J1939Tp_State_Waiting)) &&
          (J1939Tp_ReceiveMessageList[rxSessionIndex].source == source) &&
          (J1939Tp_ReceiveMessageList[rxSessionIndex].node == node) &&
          (J1939Tp_ReceiveMessageList[rxSessionIndex].isBamMessage ==
           (bool)(destination == J1939_GLOBAL_ADDRESS)))
      {
         break;
      }
   }

   if ((rxSessionIndex < J1939TP_NUMBER_OF_TP_RX_SESSIONS) && packetData)
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
            J1939Tp_TransmitConnectionAbort(J1939TP_AbortCode_Unknown,
                                            J1939Tp_ReceiveMessageList[rxSessionIndex].pgn, source,
                                            node);
         }

         // Free up the buffer that this TP Rx object was using
         J1939Tp_NumOpenReceiveConnections--;
         J1939Tp_FreeBuffer(J1939Tp_ReceiveMessageList[rxSessionIndex].incomingDataObject);
         J1939Tp_ReceiveMessageList[rxSessionIndex].state = J1939Tp_State_Available;
      }
      else
      {
         // If this is the last packet of the session...
         if (J1939Tp_ReceiveMessageList[rxSessionIndex].lastExpectedPacket == sequenceNum)
         {
            // Copy data over
            J1939Tp_ReceiveMessageList[rxSessionIndex].timeFromLastReceivedPacket = 0;
            J1939Tp_ReceiveMessageList[rxSessionIndex].lastReceivedPacket = sequenceNum;
            for (J1939_Counter_T index = 1;
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
               id = J1939_BuildMessageId(
                   0, 0, J1939TP_PRIORITY,
                   J1939_BuildPgnFromPdu(J1939_TP_CONN_MANAGEMENT_PF, source),
                   J1939Tp_ReceiveMessageList[rxSessionIndex].node->source_address);

               // Send the message out to the appropriate node.
               (void)J1939_BuildAndQueueMessage(J1939Tp_ReceiveMessageList[rxSessionIndex].node, id,
                                                CAN_MAX_DLC, true, data);
            }

            // Turn off data recording.
            J1939Tp_ReceiveMessageList[rxSessionIndex].state = J1939Tp_State_Holding;
            J1939Tp_NumHoldingMessages++;
            J1939Tp_NumOpenReceiveConnections--;
         }
         else
         {
            // Stuff data into the incoming data object
            J1939Tp_ReceiveMessageList[rxSessionIndex].timeFromLastReceivedPacket = 0;
            J1939Tp_ReceiveMessageList[rxSessionIndex].state = J1939Tp_State_Receiving;
            J1939Tp_ReceiveMessageList[rxSessionIndex].lastReceivedPacket = sequenceNum;

            for (J1939_Counter_T index = 1;
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
               J1939Tp_TransmitCts(source, rxSessionIndex, sequenceNum + 1);
            }
         }
      }
   }
   CriticalSection_Unlock();
}

/**************************************************************************************************/
uint8_t *J1939Tp_GetCompletedSessionBuffer(J1939_Pgn_T pgn, J1939_SourceAddress_T source,
                                                  J1939_Node_T node, J1939_Counter_T *length)
{
   J1939_Counter_T wHoldingMessages = J1939Tp_NumHoldingMessages;
   uint8_t *buffer = NULL;

   if (length)
   {
      // Default to zero in case we do not find a match.
      *length = 0;

      CriticalSection_Lock();
      for (J1939_Counter_T index = 0;
           wHoldingMessages && (index < J1939TP_NUMBER_OF_TP_RX_SESSIONS); index++)
      {
         if (J1939Tp_ReceiveMessageList[index].state == J1939Tp_State_Holding)
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
void J1939Tp_FreeCompletedBuffer(uint8_t *buffer)
{
   CriticalSection_Lock();

   for (J1939_Counter_T index = 0; (index < J1939TP_NUMBER_OF_TP_RX_SESSIONS); index++)
   {
      if (J1939Tp_ReceiveMessageList[index].incomingDataObject == buffer)
      {
         J1939Tp_FreeBuffer(buffer);
         J1939Tp_NumHoldingMessages--;
         J1939Tp_ReceiveMessageList[index].state = J1939Tp_State_Available;
         break;
      }
   }

   CriticalSection_Unlock();
}

/**************************************************************************************************/
static bool J1939Tp_IsPgnSupported(J1939_Pgn_T pgn, J1939_Node_T node)
{
   if (node < J1939_NUM_NODES)
   {
      for (J1939_Counter_T index = 0;
           (index < node->J1939Tp_RegisterPgnIndex) && (index < CONFIG_J1939TP_NUM_ALLOWED_RECEIVE_PGN);
           index++)
      {
         if (node->J1939Tp_RegisterPgnList[index].pgn == pgn)
         {
            return true;
         }
      }
   }

   return false;
}
#endif

/**************************************************************************************************/
static bool J1939Tp_BufferAllocate(J1939_Counter_T sizeMessage, uint8_t **buffer,
                                           J1939_Counter_T *sizeBuffer)
{
   bool result = false;

   __ASSERT_NO_MSG(sizeBuffer != NULL);
   __ASSERT_NO_MSG(buffer != NULL);

   if ((buffer != NULL) && (sizeBuffer != NULL))
   {
      for (J1939_Counter_T index = 0; index < J1939TP_NUM_ALL_BUFFERS; index++)
      {
         if (J1939Tp_AllBuffers[index].size >= sizeMessage)
         {
            // This portion of code must be protected against multiple calls.  If unprotected, it
            // could possibly assign a single buffer to two different requesters.
            if (!J1939Tp_AllBuffers[index].inUse)
            {
               J1939Tp_AllBuffers[index].inUse = true;
               *buffer = J1939Tp_AllBuffers[index].dataArea;
               *sizeBuffer = J1939Tp_AllBuffers[index].size;
               result = true;

#ifdef J1939_DEBUG
               if (sizeMessage > J1939Tp_AllBuffers[index].maxSize)
               {
                  J1939Tp_AllBuffers[index].maxSize = sizeMessage;
               }

               if (sizeMessage < J1939Tp_AllBuffers[index].minSize)
               {
                  J1939Tp_AllBuffers[index].minSize = sizeMessage;
               }
               J1939Tp_AllBuffers[index].totalSize += sizeMessage;
               J1939Tp_AllBuffers[index].allocations++;
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
void J1939Tp_FreeBuffer(uint8_t *buffer)
{
   for (J1939_Counter_T index = 0; index < J1939TP_NUM_ALL_BUFFERS; index++)
   {
      if (J1939Tp_AllBuffers[index].dataArea == buffer)
      {
         J1939Tp_AllBuffers[index].inUse = false;
         break;
      }
   }
}

/**************************************************************************************************/
static void J1939Tp_TransmitConnectionAbort(J1939Tp_AbortCode_T abortCode, J1939_Pgn_T pgn,
                                            J1939_SourceAddress_T source, J1939_Node_T node)
{
   J1939_Arbitration_T id;
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
   id = J1939_BuildMessageId(0, 0, J1939TP_PRIORITY,
                             J1939_BuildPgnFromPdu(J1939_TP_CONN_MANAGEMENT_PF, source),
                             node->source_address);

   // Send the message out to the appropriate node.
   (void)J1939_BuildAndQueueMessage(node, id, CAN_MAX_DLC, true, data);
}

#ifndef J1939TP_RECEIVE_DISABLED
/**************************************************************************************************/
static bool J1939Tp_RegisterUserCallback(J1939_Pgn_T pgn, uint8_t *data,
                                                 J1939_Counter_T length,
                                                 J1939_SourceAddress_T source, J1939_Node_T node)
{
   J1939Tp_Callback_T callback;

   if (node < J1939_NUM_NODES)
   {
      // See if the PGN is registered & supported on this node.
      for (J1939_Counter_T index = 0;
           (index < node->J1939Tp_RegisterPgnIndex) && (index < CONFIG_J1939TP_NUM_ALLOWED_RECEIVE_PGN);
           index++)
      {
         if (node->J1939Tp_RegisterPgnList[index].pgn == pgn)
         {
            callback = node->J1939Tp_RegisterPgnList[index].callback;
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
static bool J1939Tp_CancelOldReceiveSessions(J1939_Counter_T index,
                                                     J1939_SourceAddress_T source,
                                                     J1939_Node_T node, bool isBam)
{
   bool result = false;

   if ((J1939Tp_ReceiveMessageList[index].state != J1939Tp_State_Available) &&
       (J1939Tp_ReceiveMessageList[index].node == node) &&
       (J1939Tp_ReceiveMessageList[index].source == source) &&
       (J1939Tp_ReceiveMessageList[index].isBamMessage == (bool)isBam))
   {
      // session exists, cancel it.
      J1939Tp_ReceiveMessageList[index].state = J1939Tp_State_Available;

      // free the associated buffer
      J1939Tp_FreeBuffer(J1939Tp_ReceiveMessageList[index].incomingDataObject);

      result = true;
   }

   return result;
}
#endif

/**************************************************************************************************/
static void J1939Tp_ProcessCts(J1939_SourceAddress_T source, const uint8_t *data,
                               J1939_Node_T node)
{
   uint8_t numAllowedPackets;
   uint8_t nextPacketToSend;
   J1939_Pgn_T pgn;
   __ASSERT_NO_MSG(data != NULL);

   numAllowedPackets = data[1];
   nextPacketToSend = data[2];
   pgn = MAKEDWORD(MAKEWORD(data[5], data[6]), MAKEWORD(data[7], 0));

   CriticalSection_Lock();
   // Loop through the TP messages and find the one referred to by the CTS msg
   for (J1939_Counter_T index = 0; index < J1939TP_NUM_TRANSMIT_SESSIONS; index++)
   {
      // If this TP message has the same PGN as that indicated by the CTS message
      //   and this message is also being sent to the source of that CTS...
      if ((J1939Tp_TransmitMessageList[index].pgn == pgn) &&
          (J1939Tp_TransmitMessageList[index].destination == source) &&
          (J1939Tp_TransmitMessageList[index].state != J1939Tp_State_Available) &&
          (J1939Tp_TransmitMessageList[index].node == node))
      {
         // Capture the current time to indicate when last CTS was rxd
         J1939Tp_TransmitMessageList[index].elapsedTimeFromLastCts = 0;

         // If there aren't any packets left that the previous CTS allowed and
         //    the next packet requested is less than or equal to the highest one sent and
         //    the number of packets to send and the next one to send won't go past the end of the
         //    total number in the connection and the number of packets to send is !0 (ie this is
         //    not a hold type CTS) and the next packet to send isn't one that we assumed we had
         //    previously transmitted correctly...
         if ((J1939Tp_TransmitMessageList[index].numPacketsLeftCtsAllows == 0) &&
             (J1939Tp_TransmitMessageList[index].highestPacketSequenceNum >= nextPacketToSend) &&
             ((nextPacketToSend + numAllowedPackets) <=
              (J1939Tp_TransmitMessageList[index].totalPackets + 1)) &&
             (numAllowedPackets != 0) &&
             (nextPacketToSend >= J1939Tp_TransmitMessageList[index].lowestAblePacketSequenceNum))
         {
            J1939Tp_TransmitMessageList[index].numPacketsLeftCtsAllows = numAllowedPackets;

            // If the next requested packet is not the expected next packet...
            if (nextPacketToSend != J1939Tp_TransmitMessageList[index].currentPacketSequenceNum)
            {
               // There must have been an error so set conditions to retransmit
               // starting at the requested packet
               J1939Tp_TransmitMessageList[index].nextRequestedPacket = nextPacketToSend;
            }
            else
            {
               J1939Tp_TransmitMessageList[index].lowestAblePacketSequenceNum = nextPacketToSend;
               J1939Tp_TransmitMessageList[index].nextRequestedPacket = 0;
            }

            J1939Tp_TransmitMessageList[index].isWaitingBetweenCts = false;
         }
         else if (numAllowedPackets == 0)
         {
            J1939Tp_TransmitMessageList[index].isWaitingBetweenCts = true;
         }
         else
         {
            // Why did we receive CTS in the middle of transmitting?  We should
            // only get one when we've transmitted a chunk of packets.
            // Send out a connection abort, due to receiver's ineptitude, here!!!
            // Also disable the object.
            J1939Tp_TransmitConnectionAbort(J1939TP_AbortCode_Unknown,
                                            J1939Tp_TransmitMessageList[index].pgn,
                                            J1939Tp_TransmitMessageList[index].destination,
                                            J1939Tp_TransmitMessageList[index].node);

            // Free up the buffer that this TP Rx object was using
            J1939Tp_NumOpenTransmitConnections--;
            J1939Tp_FreeBuffer(J1939Tp_TransmitMessageList[index].dataRegion);
            J1939Tp_TransmitMessageList[index].state = J1939Tp_State_Available;
         }
         break; // no need to continue with the for loop
      }
   }
   CriticalSection_Unlock();
}

/**************************************************************************************************/
static void J1939Tp_ProcessEomAck(J1939_SourceAddress_T source, const uint8_t *data,
                                  J1939_Node_T node)
{
   J1939_Pgn_T pgn;
   __ASSERT_NO_MSG(data != NULL);

   pgn = MAKEDWORD(MAKEWORD(data[5], data[6]), MAKEWORD(data[7], 0));

   CriticalSection_Lock();
   // Loop through the TP messages and find the one referred to by the EOMACK msg
   for (J1939_Counter_T index = 0; index < J1939TP_NUM_TRANSMIT_SESSIONS; index++)
   {
      // If this TP message has the same PGN as that indicated by the EOMACK message
      //    and this message is also being sent to the source of that EOMACK
      //    and this message is on the same node
      //    and this module is actually waiting for a EOMACK...
      if ((J1939Tp_TransmitMessageList[index].pgn == pgn) &&
          (J1939Tp_TransmitMessageList[index].destination == source) &&
          (J1939Tp_TransmitMessageList[index].state != J1939Tp_State_Available) &&
          (J1939Tp_TransmitMessageList[index].node == node) &&
          (J1939Tp_TransmitMessageList[index].isWaitingForEomAck))
      {
         J1939Tp_NumOpenTransmitConnections--;
         J1939Tp_FreeBuffer(J1939Tp_TransmitMessageList[index].dataRegion);
         J1939Tp_TransmitMessageList[index].state = J1939Tp_State_Available;
         break;
      }
   }
   CriticalSection_Unlock();
}

/**************************************************************************************************/
static void J1939Tp_ProcessAbort(J1939_SourceAddress_T source, const uint8_t *data,
                                 J1939_Node_T node)
{
   J1939_Pgn_T pgn;
   __ASSERT_NO_MSG(data != NULL);

   pgn = MAKEDWORD(MAKEWORD(data[5], data[6]), MAKEWORD(data[7], 0));

#ifndef J1939TP_RECEIVE_DISABLED
   CriticalSection_Lock();
   // Loop through the TP messages and find the one referred to by the Conn_Abort msg
   for (J1939_Counter_T index = 0; index < J1939TP_NUMBER_OF_TP_RX_SESSIONS; index++)
   {
      // if the TP Rx resource is being used and the transmitter's address matches the address
      // we're getting the abort from and it is on the same CAN node...
      if ((J1939Tp_ReceiveMessageList[index].pgn == pgn) &&
          (J1939Tp_ReceiveMessageList[index].state != J1939Tp_State_Available) &&
          (J1939Tp_ReceiveMessageList[index].source == source) &&
          (J1939Tp_ReceiveMessageList[index].isBamMessage == 0) &&
          (J1939Tp_ReceiveMessageList[index].node == node))
      {
         // Free up the buffer that this TP Rx object was using
         J1939Tp_FreeBuffer(J1939Tp_ReceiveMessageList[index].incomingDataObject);
         J1939Tp_NumOpenReceiveConnections--;
         J1939Tp_ReceiveMessageList[index].state = J1939Tp_State_Available;
         break; // no need to continue with the for loop
      }
   }
   CriticalSection_Unlock();
#endif

   // We unlock then relock here to reduce latency when we shut off interrupts

   CriticalSection_Lock();
   // Loop through the TP messages and find the one referred to by the Conn_Abort msg
   for (J1939_Counter_T index = 0; index < J1939TP_NUM_TRANSMIT_SESSIONS; index++)
   {
      // If this TP message has the same PGN as that indicated by the Conn_Abort message
      //    and this message is also being sent to the source of that Conn_Abort
      //    and it occurs on the same CAN node...
      if ((J1939Tp_TransmitMessageList[index].pgn == pgn) &&
          (J1939Tp_TransmitMessageList[index].destination == source) &&
          (J1939Tp_TransmitMessageList[index].state != J1939Tp_State_Available) &&
          (J1939Tp_TransmitMessageList[index].node == node))
      {
         // Free up the buffer that this TP Rx object was using
         J1939Tp_NumOpenTransmitConnections--;
         J1939Tp_FreeBuffer(J1939Tp_TransmitMessageList[index].dataRegion);
         J1939Tp_TransmitMessageList[index].state = J1939Tp_State_Available;
         break; // no need to continue with the for loop
      }
   }
   CriticalSection_Unlock();
}

#ifndef J1939TP_RECEIVE_DISABLED
/**************************************************************************************************/
static void J1939Tp_ProcessBamRts(J1939_PduFormat_T control, J1939_SourceAddress_T source,
                                  const uint8_t *packetData, J1939_Node_T node)
{
   J1939_Counter_T totalMessageSize;
   J1939_Arbitration_T id;
   J1939_Pgn_T pgn;
   uint8_t totalPackets;
   uint8_t maxPacketsCanSend;
   J1939_Counter_T rxSessionIndex;
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
      if (maxPacketsCanSend > J1939TP_MAX_PACKETS)
      {
         maxPacketsCanSend = J1939TP_MAX_PACKETS;
      }
   }

   pgn = MAKEDWORD(MAKEWORD(packetData[5], packetData[6]), MAKEWORD(packetData[7], 0));

   CriticalSection_Lock();
   for (rxSessionIndex = 0; rxSessionIndex < J1939TP_NUMBER_OF_TP_RX_SESSIONS; rxSessionIndex++)
   {
      // if sessions exists, cancel it and start over.
      (void)J1939Tp_CancelOldReceiveSessions(rxSessionIndex, source, node,
                                             (bool)(control == J1939TP_CM_BAM));
   }

   // Okay, we've got a Request To Send message.  Let's first see if we even support it.
   if (J1939Tp_IsPgnSupported(pgn, node))
   {
#ifdef J1939_DEBUG
      J1939Tp_DebugMetrics.numFilterAcceptedMsgs++;
#endif
      // PGN is supported, assign resources
      for (rxSessionIndex = 0; rxSessionIndex < J1939TP_NUMBER_OF_TP_RX_SESSIONS; rxSessionIndex++)
      {
         if (J1939Tp_ReceiveMessageList[rxSessionIndex].state == J1939Tp_State_Available)
         {
            // attempt to assign a rx buffer to this message
            // If the total packets is invalid or
            //    the message size is invalid or
            //    space couldn't be allocated
            if ((totalPackets <= 1) || (totalMessageSize < J1939TP_MIN_BYTES) ||
                (totalMessageSize > J1939TP_MAX_BYTES) ||
                (J1939Tp_BufferAllocate(
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
                  J1939Tp_TransmitConnectionAbort(J1939TP_AbortCode_Unknown, pgn, source, node);
               }
            }
            else
            {

               // Prepare resource found with session information
               J1939Tp_ReceiveMessageList[rxSessionIndex].state = J1939Tp_State_Waiting;
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
                  id = J1939_BuildMessageId(
                      0, 0, J1939TP_PRIORITY,
                      J1939_BuildPgnFromPdu(J1939_TP_CONN_MANAGEMENT_PF, source),
                      J1939Tp_ReceiveMessageList[rxSessionIndex].node->source_address);

                  // Send the message out to the appropriate node.
                  (void)J1939_BuildAndQueueMessage(J1939Tp_ReceiveMessageList[rxSessionIndex].node,
                                                   id, CAN_MAX_DLC, true,
                                                   data);
               } // end IF (RTS/CTS point-to-point session).
               // If BAM message.
               else
               {
                  J1939Tp_ReceiveMessageList[rxSessionIndex].timeFromLastCtsSent = 0;
               }
               J1939Tp_NumOpenReceiveConnections++;
               break;
            }
         } // bystate == J1939Tp_State_AVAILABLE;
      }    // for loop

      if (rxSessionIndex >= J1939TP_NUMBER_OF_TP_RX_SESSIONS)
      {
         if (control == J1939TP_CM_RTS)
         {
            // Not a BAM message, spec says we're supposed to send back a Connection Abort.
            J1939Tp_TransmitConnectionAbort(J1939TP_AbortCode_Unknown, pgn, source, node);
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
         J1939Tp_TransmitConnectionAbort(J1939TP_AbortCode_Unknown, pgn, source, node);
      }
   }

   CriticalSection_Unlock();
}
#endif

#ifdef J1939_DEBUG
/**************************************************************************************************/
static void J1939Tp_TimingAnalysis(void)
{
   uint16_t smallBufferCount = 0;
   uint16_t mediumBufferCount = 0;
   uint16_t largeBufferCount = 0;
   uint16_t hugeBufferCount = 0;
   uint16_t index;

   J1939Tp_DebugMetrics.numTenMsTicks++;

   for (index = 0; index < J1939TP_NUM_ALL_BUFFERS; index++)
   {
      if (J1939Tp_AllBuffers[index].inUse == true)
      {
         switch (J1939Tp_AllBuffers[index].size)
         {
         case J1939TP_BUFFER_SMALL_SIZE:
            J1939Tp_DebugMetrics.numSmallBuffersUsed++;
            smallBufferCount++;
            break;

         case J1939TP_BUFFER_MEDIUM_SIZE:
            J1939Tp_DebugMetrics.numMedBuffersUsed++;
            mediumBufferCount++;
            break;

         case J1939TP_BUFFER_LARGE_SIZE:
            J1939Tp_DebugMetrics.numLargeBuffersUsed++;
            largeBufferCount++;
            break;

         case J1939TP_BUFFER_MAX_SIZE:
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
J1939_Counter_T J1939Tp_GetMaxAvailableBufferSize(void)
{
   J1939_Counter_T result = 0;
   J1939_Counter_T index;
   CriticalSection_Lock();

   for (index = 0; index < ELEMENTS(J1939Tp_AllBuffers); index++)
   {
      if (!J1939Tp_AllBuffers[index].inUse)
      {
         if (result < J1939Tp_AllBuffers[index].size)
         {
            result = J1939Tp_AllBuffers[index].size;
         }
      }
   }

   CriticalSection_Unlock();

   return (result);
}

#ifndef J1939TP_RECEIVE_DISABLED

/**************************************************************************************************/
static void J1939Tp_TransmitCts(J1939_DestinationAddress_T destination, J1939_Counter_T index,
                                uint8_t packetNum)
{
   uint8_t data[CAN_MAX_DLC];
   uint8_t temp;
   J1939_Arbitration_T id;

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
   id = J1939_BuildMessageId(0, 0, J1939TP_PRIORITY,
                             J1939_BuildPgnFromPdu(J1939_TP_CONN_MANAGEMENT_PF, destination),
                             J1939Tp_ReceiveMessageList[index].node->source_address);

   (void)J1939_BuildAndQueueMessage(J1939Tp_ReceiveMessageList[index].node, id,
                                    CAN_MAX_DLC, true, data);

   J1939Tp_ReceiveMessageList[index].packetCountDown =
       J1939Tp_ReceiveMessageList[index].maxRequestedPackets;

   J1939Tp_ReceiveMessageList[index].timeFromLastCtsSent = 0;
   J1939Tp_ReceiveMessageList[index].state = J1939Tp_State_Waiting;
}
#endif

#ifndef J1939TP_RECEIVE_DISABLED
/**************************************************************************************************/
bool J1939Tp_DataTransfer(const struct can_frame *message, J1939_Node_T node)
{
   J1939_SourceAddress_T source;
   J1939_PduSpecific_T ps;

   __ASSERT_NO_MSG(message != NULL);

   if (message)
   {
      source = J1939_GetSourceAddress(message->id);
      ps = J1939_GetPduSpecific(message->id);

      J1939Tp_ProcessDt(source, message->data, ps, node);
   }

   // Allow driver to handle message pointer.
   return false;
}
#endif

/**************************************************************************************************/
bool J1939Tp_ConnectionManagement(const struct can_frame *message, J1939_Node_T node)
{
   __ASSERT_NO_MSG(message != NULL);

   // Process the connection management message.
   J1939Tp_ProcessTpcmMessage(message, node);

   // Allow driver to handle message pointer.
   return false;
}
