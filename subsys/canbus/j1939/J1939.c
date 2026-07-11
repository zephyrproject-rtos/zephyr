/*
 * Copyright (c) 2026 Deere & Company
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/canbus/j1939.h>
#include <zephyr/drivers/can.h>
#include "J1939Ac.h"
#include "J1939Tp.h"
#include "J1939Timer.h"
#include <Can_Transmit.h>
#include <J1939_Cfg.h>

// All configured nodes from devicetree
extern struct j1939_dt_node_cfg* j1939_nodes[];

#ifdef J1939_ENABLE_RECEIVED_PGN_SUPPORT
typedef struct J1939_ReceivePgnHandler_S
{
   J1939_Pgn_T pgn;
   J1939_ReceivedPgnCallback_T handler;
} J1939_ReceivePgnHandler_T;
#endif

static J1939_Timer_T J1939_LastCurrentTime;

#ifdef J1939_ENABLE_RECEIVED_PGN_SUPPORT
static J1939_ReceivePgnHandler_T J1939_ReceivedPgnHandlers[J1939_NUM_NODES]
                                                          [J1939_RECEIVED_PGN_LIST_MAX];
static J1939_Counter_T J1939_ReceivedPgnRecordIndex[J1939_NUM_NODES];
#endif

#if defined(J1939DM1_RECEIVE) || defined(J1939DM2_RECEIVE)
static J1939_Timer_T J1939_UpdateDtcTimer;
#endif
#ifdef J1939_PERIODIC_ROUTINE
static J1939_Timer_T J1939_PeriodicRoutineTimer;
#endif

/// @brief Determine by which means the message should be routed. Keeps the rest of the code cleaner
/// without having to be aware of loopback state
/// @param message Message to be processed
/// @param can_dev CAN device the message was received on
/// @return True if CAN will not free memory, false if CAN will free memory
static bool J1939_RouteCanMessages(const struct can_frame *message, const struct device *can_dev);

/// @brief CAN RX callback entrypoint used by can_add_rx_filter().
static void J1939_RxFilterCallback(const struct device *dev, struct can_frame *frame,
                                   void *user_data);

/// @brief Returns true when this node is the first node configured on its CAN bus.
static bool J1939_IsFirstNodeOnBus(size_t node_index);

#if defined(J1939_LOOPBACK_ENABLE)
/// @brief When LoopBack feature is enabled, this function is used by the callback which will
/// process recieved message and pass the information to the proper processing routines when the
/// messages are dequeued.
/// @param message Message to be processed
/// @param can_dev CAN device the message was received on
/// @return True if CAN will not free memory, false if CAN will free memory
static inline bool J1939_RouteWithLoopback(const struct can_frame *message, const struct device *can_dev);
#else
/// @brief When LoopBack feature is disabled, this function is used by the callback which will
/// process recieved message and pass the information to the proper processing routines when the
/// messages are dequeued.
/// @param message Message to be processed
/// @param can_dev CAN device the message was received on
/// @return True if CAN will not free memory, false if CAN will free memory
static inline bool J1939_RouteWithoutLoopback(const struct can_frame *message, const struct device *can_dev);
#endif

/// @brief Runs the specific funciton in J1939_App_RoutingTable for the PGN in the current message
/// @param message Message to be processed
/// @param pf PF of message being processed
/// @param node J1939 node associated with the message being processed
/// @return True if CAN will not free the memory, false if CAN will free message memory
static inline bool J1939_RunMessageCallback(const struct can_frame *message,
                                             J1939_PduFormat_T pf,
                                             J1939_Node_T node);

/**************************************************************************************************/
void J1939_Init(void)
{
#if defined(CANTIME_FREE_RUNNING_TIMER)
   J1939Timer_Init();
#endif

// TODO: do we really need "virtual nodes", can't we just send multiple SAs from the same physical node?  
//This was originally added to support multiple SAs from the same physical node, but it adds a lot of complexity and overhead.  
//If we can just send multiple SAs from the same physical node, we can eliminate this complexity and overhead.
   // Initialize J1939 Interface for all nodes
//    for (J1939_Node_T index = 0; index < J1939_NUM_NODES; index++)
//    {
//       J1939_DisableVirtualModeTransmit((J1939_Node_T)index);
//    }

   // Set our initial number of PGN request messages to zero
   for (uint8_t node_index = 0; node_index < J1939_DT_NUM_NODES; node_index++)
   {
      J1939_Node_T node = j1939_nodes[node_index];
      node->J1939_RequestedPgnCount = 0;
      for (uint8_t index = 0U; index < CONFIG_J1939_MAX_PGN_REQUEST_MESSAGES; index++)
      {
         node->J1939_PgnRequestList[index].isUsed = false;
         node->J1939_PgnRequestList[index].source = J1939_GLOBAL_ADDRESS;
         node->J1939_PgnRequestList[index].pgn = (J1939_Pgn_T)0;
         node->J1939_PgnRequestList[index].isRequested = false;
      }

#ifdef J1939_ENABLE_RECEIVED_PGN_SUPPORT
      for (J1939_Counter_T index = 0; index < J1939_RECEIVED_PGN_LIST_MAX; index++)
      {
         node->J1939_ReceivedPgnHandlers[index].pgn = 0;
         node->J1939_ReceivedPgnHandlers[index].handler = NULL;
      }

      node->J1939_ReceivedPgnRecordIndex = 0;
#endif
   }

#ifdef J1939_NAME_MANAGEMENT
   J1939Nm_Init();
#endif

   // Initialize Address claimed. Whether we do or not we must still init!
   J1939Ac_Init();

#ifdef CONFIG_J1939_TRANSPORT_PROTOCOL
   // Initialize transport protocol.
   J1939Tp_Init();
#endif

#ifdef J1939_MEMORY_ACCESS
   // Initialize Memory Access
   J1939Ma_Init();
#endif

#ifdef J1939_DIAGNOSTICS
   // Initialize Diagnostics
   J1939Diag_Init();
#endif

#if defined(J1939DM4_TRANSMIT) || defined(J1939DM4_RECEIVE)
   J1939Dm4_Init();
#endif

   // Init the elapsed time counter.  The first call returns an invalid value, so we ignore it
   J1939_LastCurrentTime = 0;
   (void)J1939Timer_Elapse(&J1939_LastCurrentTime);

   // Initialize the timers for the periodic routine and the DTC counter
#ifdef J1939_PERIODIC_ROUTINE
   J1939_PeriodicRoutineTimer = 0;
#endif

#if defined(J1939DM1_RECEIVE) || defined(J1939DM2_RECEIVE)
   J1939_UpdateDtcTimer = 0;
#endif

   J1939_App_Init();

   // Initialize callback such that we get all messages when they are dequeued. Send to the
   // Routing function for distribution to the J1939 services.
   struct can_filter filter = {
      .mask = 0,
      .flags = CAN_FILTER_IDE
   };

//    TODO - I feel like there should be a compile time way to do this, leaving for now
//    loop through all j1939 nodes and register the callback for each physical bus
   for (size_t node_index = 0; node_index < J1939_NUM_NODES; node_index++)
   {
      const struct device *can_dev = j1939_nodes[node_index]->can_dev;

      if ((can_dev == NULL) || !J1939_IsFirstNodeOnBus(node_index))
      {
         continue;
      }

      int filter_id = can_add_rx_filter(can_dev, J1939_RxFilterCallback, NULL, &filter);

      if (filter_id < 0)
      {
         printk("j1939: add filter failed for %s: %d\n", can_dev->name, filter_id);
      }
   }
}

/**************************************************************************************************/
bool J1939_TransmitPgnRequest(J1939_Pgn_T pgn, J1939_DestinationAddress_T destination,
                                      J1939_Node_T node)
{
   uint8_t data[CAN_MAX_DLC] = {0};

   // Need to make sure the PGN we request is valid
   if (!J1939_IsPgnValid(pgn))
   {
      return false;
   }

   data[0] = LOBYTE(LOWORD(pgn));
   data[1] = HIBYTE(LOWORD(pgn));
   data[2] = LOBYTE(HIWORD(pgn));

   // This is one of the few J1939 messages we can send out with only 3 bytes.
   return J1939_TransmitPgn(J1939_Priority_6, J1939_REQUEST_PGN, destination, data, 3, node);
}

/**************************************************************************************************/
bool J1939_IsPgnRequested(J1939_Pgn_T pgn, J1939_SourceAddress_T *source, J1939_Node_T node)
{
      // Note: We don't need multi-thread protection here because of how J1939_RegisterRequestPgn()
      // is implemented
      for (uint8_t index = 0; index < node->J1939_RequestedPgnCount; index++)
      {
         if (index < CONFIG_J1939_MAX_PGN_REQUEST_MESSAGES)
         {
            if ((pgn == node->J1939_PgnRequestList[index].pgn) &&
                (node->J1939_PgnRequestList[index].isUsed))
            {
               if (source)
               {
                  *source = node->J1939_PgnRequestList[index].source;
               }

               if (node->J1939_PgnRequestList[index].isRequested)
               {
                  // we reset the request, because we assume the application will respond.
                  node->J1939_PgnRequestList[index].isRequested = false;
                  return true;
               }
               else
               {
                  // The PGN is in the list, but is not requested.
                  return false;
               }
            }
         }
      }

   // PGN is not in the list. It may be requested, it will either be nacked or automatically
   // sent out as data from the transmit list.
   return false;
}

/**************************************************************************************************/
void J1939_Task(void)
{
   // Find amount of time which has gone by since last time this function was called.
   J1939_Timer_T elapsedCanTime = J1939Timer_Elapse(&J1939_LastCurrentTime);

   (void)elapsedCanTime;

   J1939Ac_Task();

#ifdef CONFIG_J1939_TRANSPORT_PROTOCOL
   // The following two functions process the transport protocol.
#ifndef J1939TP_RECEIVE_DISABLED
   J1939Tp_UpdateReceiveMessageTimes(elapsedCanTime);
#endif
   J1939Tp_UpdateSendMessageTimes(elapsedCanTime);
#endif

   for (uint16_t node_index = 0; node_index < J1939_DT_NUM_NODES; node_index++)
   {
    // TODO - this is a bit of a hack to get the node information into the various processing functions.
      J1939_Node_T node = j1939_nodes[node_index];
#ifdef J1939_MEMORY_ACCESS
      J1939Ma_Task(node);

#ifdef J1939MA_TRANSMIT_READ_WRITE_REQUEST
      J1939Ma_ProcessTransmitCommands(node);
#endif
#endif

#ifdef J1939_DIAGNOSTICS
#ifdef J1939DM1_TRANSMIT
      J1939Dm1_Transmit(node);
#endif

#ifdef J1939DM13_ENABLE
      J1939Dm13_Process(node);
#endif
#endif
   }

#if defined(J1939DM1_RECEIVE)
   J1939_UpdateDtcTimer = (J1939_UpdateDtcTimer + elapsedCanTime);
   if (J1939_UpdateDtcTimer >= J1939DM1_DTC_UPDATE_MS)
   {
      J1939Dm1_UpdateActiveDtcTable(J1939_UpdateDtcTimer);
      J1939_UpdateDtcTimer = 0;
   }
#endif

   J1939_App_Task();

#ifdef J1939_TRANSMIT_QUEUE_HANDLING
   Can_Transmit_SendQueues();
#endif
}

/**************************************************************************************************/
void J1939_Acknowledge(const J1939_Pgn_T pgn, J1939_Response_T control,
                       const J1939_DestinationAddress_T destination, J1939_Node_T node)
{
   J1939_Arbitration_T id;
   J1939_SourceAddress_T source = node->source_address;
   uint8_t data[CAN_MAX_DLC];

   // Build the response ID
   id = J1939_BuildMessageId(
       0, 0, J1939_Priority_6,
       J1939_BuildPgnFromPdu(J1939_PGN_ACK_PF, (J1939_PduSpecific_T)destination), source);

   // Build the response message
   data[0] = (uint8_t)control;
   data[1] = J1939_BYTE_UNAVAILABLE;
   data[2] = J1939_BYTE_UNAVAILABLE;
   data[3] = J1939_BYTE_UNAVAILABLE;
   data[4] = J1939_BYTE_UNAVAILABLE;
   data[5] = LOBYTE(LOWORD(pgn));
   data[6] = HIBYTE(LOWORD(pgn));
   data[7] = LOBYTE(HIWORD(pgn));

   // Send out the message
   (void)J1939_BuildAndQueueMessage(node, id, 8, true, data);
}

/**************************************************************************************************/
bool J1939_FlagPgnRequest(J1939_Pgn_T pgn, J1939_SourceAddress_T source, J1939_Node_T node)
{
   bool result = false;

      for (uint8_t index = 0; index < node->J1939_RequestedPgnCount; index++)
      {
         if (index < CONFIG_J1939_MAX_PGN_REQUEST_MESSAGES)
         {
            if ((pgn == node->J1939_PgnRequestList[index].pgn) &&
                node->J1939_PgnRequestList[index].isUsed)
            {
               node->J1939_PgnRequestList[index].source = source;
               node->J1939_PgnRequestList[index].isRequested = true;
               result = true;
               break;
            }
         }
      }

   return result;
}

/**************************************************************************************************/
bool J1939_RegisterRequestPgn(J1939_Pgn_T pgn, J1939_Node_T node)
{
   J1939_Counter_T index;
   bool result = false;
   bool setCurrentEntry = false;

   if (!J1939_IsPgnValid(pgn))
   {
      // Invalid PGN
      return false;
   }

    for (index = 0; index < CONFIG_J1939_MAX_PGN_REQUEST_MESSAGES; index++)
    {
        // Find if the PGN is in the list and update it or put it in the first unused space.
        if (pgn == node->J1939_PgnRequestList[index].pgn)
        {
            setCurrentEntry = true;
            break;
        }
        else if (!node->J1939_PgnRequestList[index].isUsed)
        {
            // We need to add a PGN to the list, increment the count.
            node->J1939_RequestedPgnCount++;
            setCurrentEntry = true;
            break;
        }
    }

    // Update the entry at the current index value.  Note that we update all three fields
    // regardless of if this is an update or a new entry.  This is probably not necessary in
    // the case of an update, but we left it that way to preserve behavior just in case
    // someone was relying on it for something
    if (setCurrentEntry && (index < CONFIG_J1939_MAX_PGN_REQUEST_MESSAGES))
    {
        node->J1939_PgnRequestList[index].pgn = pgn;
        node->J1939_PgnRequestList[index].isUsed = true;
        node->J1939_PgnRequestList[index].isRequested = false;
        result = true;
    }

   // No room in list
   return result;
}

/**************************************************************************************************/
bool J1939_TransmitPgn(J1939_Priority_T priority, J1939_Pgn_T pgn,
                       J1939_DestinationAddress_T destination, uint8_t *data,
                       J1939_Counter_T count, J1939_Node_T node)
{
   J1939_Arbitration_T id;
   J1939_Pgn_T tempPgn = pgn;
   bool dataPage;
   bool extendedDataPage;
   bool result;

   (void)data;
   (void)tempPgn; // Not used with some configurations, silence compiler warning

   // The PGN should not have a destination address embedded in it.
   if (J1939_IsPgnValid(pgn))
   {
      // If PF < 240(0xF0), we must make sure the Id contains the destination
      // address information, or add it in.  We have already checked the
      // passed in PGN to make sure it is a valid PGN.
      if ((pgn & 0xFF00) < 0xF000)
      {
         // Add in the destination information for our message ID only if
         // eight or less bytes of data to send.
         pgn = pgn + (J1939_Pgn_T)destination;
      }

      // Extract the Data Page bits from the PGN
      dataPage = J1939_GetDataPageFromPgn(pgn);
      extendedDataPage = J1939_GetExtendedDataPageFromPgn(pgn);

      // First check how many bytes to send.  If more than 8 we use transport
      // protocol.  Otherwise we queue it up.
      if (count <= CAN_MAX_DLC)
      {
         // Create the identifier for the message
        id = J1939_BuildMessageId(dataPage, extendedDataPage, priority, pgn,
                                   node->source_address);

         // Send the message out to the appropriate node.
         result = J1939_BuildAndQueueMessage(node, id, count, true, data);

#ifdef CONFIG_J1939_TRANSPORT_PROTOCOL
         // Attempt to release the buffer in case the caller allocated a transport buffer
         // Release the transport buffer.
         J1939Tp_FreeBuffer(data);
#endif
      }
      else
      {
#ifdef CONFIG_J1939_TRANSPORT_PROTOCOL
         // Send out the message via transport protocol.
         if (J1939Tp_Message_Accepted ==
             J1939Tp_TransmitMultiPacket(tempPgn, destination, count, data, node))
         {
            result = true;
         }
         else
         {
            result = false;
         }

         // Release the transport buffer.
         J1939Tp_FreeBuffer(data);
#else
         // No transport layer, cannot send multipacket. Drop down and return false.
         result = false;
#endif
      }
   }
   else
   {
#ifdef CONFIG_J1939_TRANSPORT_PROTOCOL
      // Attempt to release the buffer in case the caller allocated a transport buffer
      // Release the transport buffer.
      J1939Tp_FreeBuffer(data);
#endif

      // Invalid PGN - We should not transmit an invalid PGN
      result = false;
   }

   return result;
}

/**************************************************************************************************/
bool J1939_RequestPgn(const struct can_frame *message, J1939_Node_T node)
{
   J1939_Pgn_T pgn;
   J1939_SourceAddress_T requestSource;
   J1939_PduSpecific_T ps;
   J1939_PduFormat_T pf;
   J1939_SourceAddress_T source;

   if (message)
   {
      ps = J1939_GetPduSpecific(message->id);
      pf = J1939_GetPduFormat(message->id);
      source = J1939_GetSourceAddress(message->id);

      // Determine the source address, if it was sent globally then set the source address to global
      if (ps == J1939_GLOBAL_ADDRESS)
      {
         requestSource = J1939_GLOBAL_ADDRESS;
      }
      else
      {
         requestSource = source;
      }

      // Build up the ID.
      pgn = MAKEDWORD(MAKEWORD(message->data[0], message->data[1]), MAKEWORD(message->data[2], 0));

      switch (pgn)
      {
      case J1939_ADDRESS_CLAIMED_PGN:
         // Handle the address claim request
         J1939Ac_ProcessRequest(node);
         break;

#ifdef J1939DM1_TRANSMIT
      case J1939_DM1_PGN:
         // Send DM1 active errors
         J1939Dm1_Response(source, ps, message->node);
         break;
#endif

#ifdef J1939DM2_TRANSMIT
      case J1939_DM2_PGN:
         // Send DM2 prev active dtcs.
         J1939Dm2_ProcessRequest(source, ps, message->node);
         break;
#endif

#ifdef J1939DM3_ENABLE
      case J1939_DM3_PGN:
         // Clear prev active dtcs & send Ack.
         J1939Dm3_Task(message);
         break;
#endif

#if defined(J1939DM4_TRANSMIT)
      case J1939_DM4_PGN:
         J1939Dm4_ProcessRequest(source, ps, message->node);
         break;
#endif

#ifdef J1939DM5_ENABLE
      case J1939_DM5_PGN:
         // Diagnostic readiness. Number of active and prev active dtcs etc...
         J1939Dm5Process(source, message->node);
         break;
#endif

#ifdef J1939DM11_ENABLE
      case J1939_DM11_PGN:
         // clear active dtcs & send Ack.
         J1939Dm11_Task(message);
         break;
#endif

      default:
         // Check if the PGN is in the request list. If so set the boolean for the PGN
         // request to true.
         if (!J1939_FlagPgnRequest(pgn, requestSource, node))
         {
            // The PGN was not in the request list. If we are here, we do not support the
            // request.  J1939 says we should Nack the PGN request if it was not to a global
            // destination address.
            if (!((pf >= J1939_PDUF_240) || (ps == J1939_GLOBAL_ADDRESS)))
            {
               J1939_Acknowledge(pgn, J1939_Response_Nack, (J1939_DestinationAddress_T)source,
                                 node);
            }
         }
         break;

      } //lint !e764   // If there are no features enabled, suppress when lint warns that there are
        // no cases
   }

   // Allow driver to handle message pointer.
   return false;
}

/**************************************************************************************************/
bool J1939_IsPgnValid(J1939_Pgn_T pgn)
{
   return (!(((pgn & 0xFF00) < 0xF000) && (pgn & 0x00FF)));
}

/**************************************************************************************************/
// static inline bool J1939_IsMessageForMe(const struct can_frame *message) //lint !e528
// {
//    bool result = false;
//    J1939_PduSpecific_T ps;
//    J1939_PduFormat_T pf;

//    __ASSERT_NO_MSG(message != NULL);

//    ps = J1939_GetPduSpecific(message->id);
//    pf = J1939_GetPduFormat(message->id);

//    if ((pf >= J1939_PDUF_240) || (ps == J1939_GLOBAL_ADDRESS))
//    {
//       result = true;
//    }
//    else if (J1939Ac_GetState(message->node) == J1939Ac_State_Claimed)
//    {
//       // Don't look for messages to our source address until we have successfully claimed it
//       if (ps == (J1939_PduSpecific_T)J1939Ac_GetSourceAddress(message->node))
//       {
//          result = true;
//       }
//    }

//    return result;
// }

/**************************************************************************************************/
static bool J1939_RouteCanMessages(const struct can_frame *message, const struct device *can_dev)
{
   /*lint -esym(838, result) ignore issues with not using previous set version of this variable*/
   bool result = false; // Allow Driver to free the message memory by default

#if defined(J1939_LOOPBACK_ENABLE)
   result = J1939_RouteWithLoopback(message, can_dev);
#else
   result = J1939_RouteWithoutLoopback(message, can_dev);
#endif

   return result;
}

/**************************************************************************************************/
static void J1939_RxFilterCallback(const struct device *dev, struct can_frame *frame,
                                   void *user_data)
{
   (void)user_data;

   (void)J1939_RouteCanMessages(frame, dev);
}

/**************************************************************************************************/
static bool J1939_IsFirstNodeOnBus(size_t node_index)
{
   for (size_t index = 0; index < node_index; index++)
   {
      if (j1939_nodes[index]->can_dev == j1939_nodes[node_index]->can_dev)
      {
         return false;
      }
   }

   return true;
}

/**************************************************************************************************/
bool J1939_DefaultRoute(const struct can_frame *message, J1939_Node_T node)
{
#ifdef J1939_ENABLE_RECEIVED_PGN_SUPPORT

   __ASSERT_NO_MSG(message != NULL);

   // Extract the PGN
   J1939_Pgn_T pgn = J1939_GetPgn(message->id);
//    uint8_t node = message->node;
   uint16_t node = 0; // TODO: Replace with actual node retrieval

   // Search through the list and see if the PGN is in the list
   // NOTE: Multi-thread protection not needed because of how J1939_RegisterReceivePgnCallback()
   // is implemented
   for (J1939_Counter_T index = 0; index < J1939_ReceivedPgnRecordIndex[node]; index++)
   {
      if (pgn == J1939_ReceivedPgnHandlers[node][index].pgn)
      {
         if (J1939_ReceivedPgnHandlers[node][index].handler)
         {
            (void)J1939_ReceivedPgnHandlers[node][index].handler(message);
         }
      }
   }
#else
   // Supress any warnings.
   (void)message;
#endif

   // Allow driver to handle message pointer.
   return false;
}

/**************************************************************************************************/
bool J1939_PduF254Process(const struct can_frame *message, J1939_Node_T node)
{
#if defined(J1939DM1_RECEIVE) || defined(J1939DM2_RECEIVE) || defined(J1939DM4_RECEIVE)
   J1939_PduSpecific_T ps;
   J1939_SourceAddress_T source;

   __ASSERT_NO_MSG(message != NULL);

   if (message && (message->node < CAN_NUM_NODES))
   {
      ps = J1939_GetPduSpecific(message->arbitration);
      source = J1939_GetSourceAddress(message->arbitration);

      // These variables are not used in all cases so we need to suppress a warning
      (void)ps;
      (void)source;

#ifdef J1939DM1_RECEIVE
      if (ps == J1939_ACTIVE_DTC_PS)
      {
         // Send address and message data to DM1 processing function.
         J1939Dm1_Receive(source, (Can_DataByte_T *)message->data, 6, message->node);

#ifndef J1939_PASSTHROUGH_FOR_254_MESSAGES
         return false;
#endif
      }
#endif

#ifdef J1939DM2_RECEIVE
      if (ps == J1939_PREVIOUS_ACTIVE_PS)
      {
         J1939Dm2_ProcessReceived(source, (Can_DataByte_T *)message->data, 6, message->node);

#ifndef J1939_PASSTHROUGH_FOR_254_MESSAGES
         return false;
#endif
      }
#endif

#if defined(J1939DM4_RECEIVE)
      if (ps == J1939_FREEZE_FRAME_PS)
      {
         // Send address and message data to DM4 processing function.
         J1939Dm4_ProcessReceived(source, message->data, message->length, message->node);

#ifndef J1939_PASSTHROUGH_FOR_254_MESSAGES
         return false;
#endif
      }
#endif
   }

#endif

   // Let the application specific routines handle it
   /*lint -esym(613, message)*/
   return J1939_App_Pf254Process(message, node);
}

#ifdef J1939_ENABLE_RECEIVED_PGN_SUPPORT
/**************************************************************************************************/
bool J1939_RegisterReceivePgnCallback(J1939_Pgn_T pgn, J1939_ReceivedPgnCallback_T handler,
                                              J1939_Node_T node)
{
   bool result = false;

   if (handler != NULL)
   {
      if (J1939_ReceivedPgnRecordIndex[node] < J1939_RECEIVED_PGN_LIST_MAX)
      {
         J1939_ReceivedPgnHandlers[node][J1939_ReceivedPgnRecordIndex[node]].pgn = pgn;
         J1939_ReceivedPgnHandlers[node][J1939_ReceivedPgnRecordIndex[node]].handler = handler;
         J1939_ReceivedPgnRecordIndex[node]++;
         result = true;
      }
   }

   return result;
}
#endif

/**************************************************************************************************/
J1939_Pgn_T J1939_GetPgn(J1939_Arbitration_T messageId)
{
   J1939_Pgn_T result;

   // Per the J1939 standard, a PGN is a 24-bit value with the bits defined as:
   // [0-7] Group Extension
   // [8-15] PDU Format
   // [16] Data Page
   // [17] Extended Data Page
   // [18-23] Zero

   // Extract the lower 18 bits
   result = ((messageId >> 8) & 0x3FFFF);

   // If the PF is less than 0xF0, then the group extension should be set to zero
   if (J1939_GetPduFormat(result) < J1939_PDUF_240)
   {
      result &= 0x3FF00;
   }

   return (result);
}

/**************************************************************************************************/
J1939_Pgn_T J1939_BuildPgn(bool extendedDataPage, bool dataPage,
                           J1939_PduFormat_T pduf, J1939_GroupExtension_T groupExtension)
{
   J1939_Pgn_T result;

   result = MAKEWORD(groupExtension, pduf);
   result |= (((uint32_t)dataPage) << 16);
   result |= (((uint32_t)extendedDataPage) << 17);

   if (pduf < J1939_PDUF_240)
   {
      result &= 0x3FF00;
   }

   return (result);
}

/**************************************************************************************************/
void J1939_EnableVirtualModeTransmit(J1939_Node_T node)
{
    node->transmission_enabled = true;
}

/**************************************************************************************************/
void J1939_DisableVirtualModeTransmit(J1939_Node_T node)
{
    node->transmission_enabled = false;
}

/**************************************************************************************************/
bool J1939_IsVirtualNodeTransmitEnabled(J1939_Node_T node)
{
   return node->transmission_enabled;
}

/**************************************************************************************************/
bool J1939_BuildAndQueueMessage(J1939_Node_T node, J1939_Arbitration_T arbitration,
                                uint16_t dataLength, bool isExtendedMessage,
                                const uint8_t *data)
{
    bool result = false;
    result = Can_Transmit_BuildAndQueueMessage(
        node->can_dev, arbitration, (uint16_t)dataLength, isExtendedMessage, false, data);
    return result;
}

#if defined(J1939_LOOPBACK_ENABLE)
/**************************************************************************************************/
static inline bool J1939_RouteWithLoopback(const struct can_frame *message)
{
   // Msg object to replace physical node to address node when running handler
   struct can_frame msg;
   bool result = false;
   J1939_Node_T addressNode;
   uint16_t physicalNode;
   J1939_PduFormat_T pf = J1939_GetPduFormat(message->arbitration);
   J1939_PduSpecific_T ps = J1939_GetPduSpecific(message->arbitration);

   J1939_Node_T loopbackNode;

   // Is this a broadcast message?
   if (pf >= J1939_PDUF_240 || ps == J1939_GLOBAL_ADDRESS)
   {
      // Get address node that is sending loopBack message
      if (message->TransmitLoopbackMessage)
      {
         loopbackNode =
             J1939_GetJ1939NodeFromSourceAddress(J1939_GetSourceAddress(message->arbitration));
         physicalNode = J1939_GetCanNode(loopbackNode);
      }
      else
      {
         // If this is not a loopback message set values to transmit to all addressNodes
         loopbackNode = J1939_NUM_NODES + 1;
         physicalNode = message->node;
      }
      // Transmit to all address nodes
      for (addressNode = 0; addressNode < J1939_NUM_NODES; addressNode++)
      {
         // Linked to the physical node of the loopback addressNode
         if (J1939_GetCanNode(addressNode) == physicalNode)
         {
            // Except for the transmitting loopback not itself
            if (addressNode != loopbackNode)
            {
               msg = *message;
               msg.node = addressNode;
               result = J1939_RunMessageCallback(&msg, pf);
            }
         }
      }
   }
   // Is a message for a specific address node?
   else if (J1939_GetJ1939NodeFromSourceAddress(ps) < J1939_NUM_NODES)
   {
      msg = *message;
      msg.node = J1939_GetJ1939NodeFromSourceAddress(ps);
      result = J1939_RunMessageCallback(&msg, pf);
   }

   return result;
}
#else

static inline bool J1939_GetNode(uint8_t sourceAddress, J1939_Node_T *node)
{
   for (size_t node_index = 0; node_index < J1939_NUM_NODES; node_index++)
   {
      if (j1939_nodes[node_index]->source_address == sourceAddress)
      {
         *node = j1939_nodes[node_index];
         return true;
      }
   }

   return false; // Return false if not found
}

/**************************************************************************************************/
static inline bool J1939_RouteWithoutLoopback(const struct can_frame *message, const struct device *can_dev)
{
   bool result = false;
   J1939_PduSpecific_T ps = J1939_GetPduSpecific(message->id);
   J1939_PduFormat_T pf = J1939_GetPduFormat(message->id);

   J1939_Node_T node;

   // Determine if the message is for us or global and what we should do with it.
   if ((J1939_GetNode(ps, &node) ||         // put this first to ensure node gets filled in
       (pf >= J1939_PDUF_240) ||
       (ps == J1939_GLOBAL_ADDRESS)))
   {
      result = J1939_RunMessageCallback(message, pf, node);
   }

   return result;
}
#endif

/**************************************************************************************************/
static inline bool J1939_RunMessageCallback(const struct can_frame *message,
                                             J1939_PduFormat_T pf,
                                             J1939_Node_T node)
{
   bool result = false;
   // Call the function in the routing table.  This eliminates lots of checks with a case
   // statement since the call is made based on the PF of the message.
#ifdef J1939_NEVER_EATS_MESSAGES
   // If the J1939 layer doesn't eat messages, we get a compiler warning if we assign to
   // result, so we cast to void in that case to eliminate it
   (void)J1939_App_RoutingTable[pf](message, node);
#else
   result = J1939_App_RoutingTable[pf](message, node);
#endif

   return result;
}
