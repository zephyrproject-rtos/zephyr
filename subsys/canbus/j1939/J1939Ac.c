/*
 * Copyright (c) 2026 Caleb Perkinson
 * SPDX-License-Identifier: Apache-2.0
 */
#include "J1939Ac.h"
#include "J1939Timer.h"
#include <Can_Transmit.h>
#include <J1939Timer.h>

/*==========================================================================
 * Possible Future Improvements
 *========================================================================*/
// 1. Store all J1939_Name_T structures as the packed 8 byte array instead of
// in the unpacked
//    structure.  This would save 2*J1939_NUM_NODES*6*J1939_MAX_DEVICES_IN_SYSTEM bytes
//    of RAM at the cost of runtime conversion between the two.  We have not yet made this change
//    because we have been more processor limited than RAM limited.
//

extern struct j1939_dt_node_cfg j1939_nodes[];

#define J1939_AC_WAIT_TIME ((J1939_Timer_T)250) // Milliseconds
#define J1939_PR_ADDRESS_CLAIMED (J1939_Priority_6)
#define J1939_PF_ADDRESS_CLAIMED ((J1939_PduFormat_T)0xEE)
#define J1939_NAME_LENGTH ((J1939_Counter_T)8)

typedef struct J1939Ac_BusInfo_S
{
   J1939_SourceAddress_T source; // Source address associated with the name pointer below
   J1939_Name_T name;
} J1939Ac_BusInfo_T;


static bool J1939Ac_IsRequested[J1939_NUM_NODES];
static J1939Ac_BusInfo_T J1939Ac_BusInfo[J1939_NUM_NODES];

#ifdef J1939AC_SELF_CONFIGURABLE
// If we are arbitrary address capable, then we need a list of the addresses we will attempt to
// claim, in the order we will attempt to claim them, when we lose arbitration.  The application
// developer is responsible for filling in this information in J1939Ac_App_Init()
// using the J1939Ac_SetArbitraryListEntry() function
static J1939_Counter_T
    J1939Ac_ArbitraryAddressListIndex[J1939_NUM_NODES]; // Keeps track of the index of
                                                        // the next address to use
static J1939_SourceAddress_T
    J1939Ac_ArbitraryAddressCapableSourceAddressList[J1939_NUM_NODES]
                                                    [J1939AC_ALT_ADDRESS_LIST_LENGTH];
#endif

static void J1939Ac_CannotClaimDelay(J1939_Node_T node);
static void J1939Ac_AddressClaimStart(J1939_Node_T node);
static void J1939Ac_AddressClaimWaiting(J1939_Node_T node);
static void J1939Ac_LostContention(J1939_Node_T node);
static void J1939Ac_CommonContentionHandling(J1939_Node_T node);
static bool J1939Ac_CheckAddressContention(J1939_Node_T node, const uint8_t *data);
static void J1939Ac_ClaimAddress(J1939_Node_T node);
static void J1939Ac_UpdateRecordedBusInfo(J1939_SourceAddress_T source,
                                          const uint8_t *nameData, J1939_Node_T node);

#ifdef J1939AC_SELF_CONFIGURABLE
static J1939_SourceAddress_T J1939Ac_GetNextAlternateAddress(J1939_Node_T node);
#endif

/**************************************************************************************************/
void J1939Ac_Init(void)
{
   for (uint16_t node_index = 0; node_index < J1939_DT_NUM_NODES; node_index++)
   {
        J1939_Node_T dt_node = &(j1939_nodes[node_index]);
      // Retrieve the default source address we will claim from the application specific module
      J1939Ac_BusInfo[node_index].source = J1939Ac_App_GetDefaultSourceAddress(dt_node);

#ifdef J1939AC_SELF_CONFIGURABLE
      J1939Ac_ArbitraryAddressListIndex[node_index] = 0;

      for (uint16_t index = 0; index < J1939AC_ALT_ADDRESS_LIST_LENGTH; index++)
      {
         // Initialize the list of alternate source addresses for address claim to the NULL address
         // This is done here, just so the application developer doesn't forget to do it in the
         // application file
         J1939Ac_ArbitraryAddressCapableSourceAddressList[node_index][index] = J1939_NULL_ADDRESS;
      }
#endif

      J1939Ac_App_GenerateName(dt_node);

      // Disable Transmit queue(s) until we have claimed our address
      J1939_DisableVirtualModeTransmit(dt_node);

      // Prepare to claim the address
      dt_node->node_state = J1939Ac_State_WaitingStartupInit;
      J1939Ac_IsRequested[node_index] = false;

      dt_node->recorded_bus_info_count = 0;

      // Initialize the J1939 NAME/source address information
      for (uint16_t index = 0; index < CONFIG_J1939_MAX_NODES_IN_SYSTEM; index++)
      {
         dt_node->recorded_bus_info[index].source = J1939_NULL_ADDRESS;

#if defined(J1939_RECORD_ADDRESS_CLAIMED_NAMES)
         dt_node->recorded_bus_info[index].name.idNumber = 0;
         dt_node->recorded_bus_info[index].name.mfgCode = 0;
         dt_node->recorded_bus_info[index].name.ecuInstance = 0;
         dt_node->recorded_bus_info[index].name.functionInstance = 0;
         dt_node->recorded_bus_info[index].name.function = 0;
         dt_node->recorded_bus_info[index].name.vehicleSystem = 0;
         dt_node->recorded_bus_info[index].name.vehicleSystemInstance = 0;
         dt_node->recorded_bus_info[index].name.industryGroup = 0;
         dt_node->recorded_bus_info[index].name.isSelfConfig = false;
         dt_node->recorded_bus_info[index].name.reservedBit = 0;
#endif
      }
   }

   J1939Ac_App_Init();
}

/**************************************************************************************************/
void J1939Ac_Task(void)
{
   // Loop through all the nodes
   for (uint16_t node_index = 0; node_index < J1939_NUM_NODES; node_index++)
   {
        J1939_Node_T node = &j1939_nodes[node_index];
      switch (node->node_state)
      {
      case J1939Ac_State_WaitingStartupInit:
#if defined(CAN_AUTOBAUD_ENABLE)
         if (!Can_Autobaud_IsInProgress(J1939_GetCanNode(node)))
         {
            // We have a valid baud rate.  We can start address claim
            node->node_state = J1939Ac_State_Start;
         }
#else
         node->node_state = J1939Ac_State_Start;
#endif
         break;

      case J1939Ac_State_Start:
         J1939Ac_AddressClaimStart(node);
         break;

      case J1939Ac_State_Waiting:
         J1939Ac_AddressClaimWaiting(node);
         break;

      case J1939Ac_State_Claimed:
         // Don't change anything
         break;

      case J1939Ac_State_LostContention:
         J1939Ac_LostContention(node);
         break;

      case J1939Ac_State_WaitingCannotClaim:
         J1939Ac_CannotClaimDelay(node);
         break;

      case J1939Ac_State_CannotClaim:
         // Don't change anything
         break;

      default:
         // Shouldn't ever get here.  If we do we go to the cannot claim state.  If someone
         // requests address claim again, we will reset and try to claim agin
         node->node_state = J1939Ac_State_CannotClaim;
         break;
      }
   }

   // Handle any application specific operations.  Pass NULL since we are calling
   // from the periodic function.
   J1939Ac_App_Task(NULL);
}

/***********************************************************************************************
DESCRIPTION:   Initiates an address claim for the specified node

ARGUMENTS:     node - Node to initiate an address claim for

RETURN:        None
***********************************************************************************************/
static void J1939Ac_ClaimAddress(J1939_Node_T node)
{
   J1939_Arbitration_T id;
   uint8_t name[CAN_MAX_DLC];

   // We do not need a range check on node since it is guaranteed to remain in range previously
   // in the call chain to this function
   // This function assumes that we are in a critical section when called.  The caller is
   // responsible for handling this

   id = J1939_BuildMessageId(0, 0, J1939_PR_ADDRESS_CLAIMED,
                             J1939_BuildPgnFromPdu(J1939_PF_ADDRESS_CLAIMED, J1939_GLOBAL_ADDRESS),
                             node->source_address);

   J1939Ac_NameConfigToByteArray(node, name);

   if (J1939_BuildAndQueueMessage(node, id, J1939_NAME_LENGTH, true, name))
   {
      // If this is the first time the node is claiming its address, set the timestamp
      if (node->node_state == J1939Ac_State_Start)
      {
         node->ac_timestamp = J1939Timer_GetTime();
      }

      Can_Transmit_SendQueues();
   }
}

/**************************************************************************************************/
void J1939Ac_ProcessRequest(J1939_Node_T node)
{
    // If were in state J1939Ac_State_Claimed, we can send our address claimed out,
    // otherwise we need to set a flag to send out an address claimed once we reach the
    // J1939Ac_State_Claimed state.
    CriticalSection_Lock();
    if (node->node_state == J1939Ac_State_Claimed)
    {
        // Send out an address claimed.  Queues will always be enabled if we are
        // J1939Ac_State_Claimed
        J1939Ac_ClaimAddress(node);
    }
    else if (node->node_state == J1939Ac_State_CannotClaim)
    {
        // If we cannot claim an address, we still have to respond to the request.
        J1939_EnableVirtualModeTransmit(node);
        J1939Ac_ClaimAddress(node);
        J1939_DisableVirtualModeTransmit(node);
    }
    else
    {
        // Set a flag to send address claimed once we reach that state
    //  TODO - implement this
    //  J1939Ac_IsRequested[node] = true;
    }
    CriticalSection_Unlock();
}

/**************************************************************************************************/
bool J1939Ac_IsSuccessful(J1939_Node_T node)
{
    // not range checking since it is configured by device tree
    return (node->node_state == J1939Ac_State_Claimed);
}

/***********************************************************************************************
DESCRIPTION:   Checks for address contention

ARGUMENTS:     node - Node to check for contention on
               data - Name data to check with

RETURN:        bool - true if we lost contention, false if we won
***********************************************************************************************/
static bool J1939Ac_CheckAddressContention(J1939_Node_T node, const uint8_t *data)
{
   J1939_Counter_T index = 7; // Names are 8 bytes long, so start at the last index
   bool result = false;
   bool continueOn = true;
   uint8_t name[CAN_MAX_DLC];

   // We do not need the critical section here, since it is locked in the caller.
   // We do not need a range check on node since it is guaranteed to remain in range previously
   // in the call chain to this function

   if (data != NULL)
   {
      J1939Ac_NameConfigToByteArray(node, name);

      // Start with the most significant byte and work our way down.
      while (continueOn)
      {
         if (name[index] > data[index])
         {
            // We lose the contention because our name is higher
            result = true;
            continueOn = false;
         }
         else if (name[index] < data[index])
         {
            // We win because our name is lower
            result = false;
            continueOn = false;
         }
         else
         {
            // The values are equal, we have to check the next byte of the name
            if (index > 0)
            {
               index--;
            }
            else
            {
               // Names are identical
               continueOn = false;
               result = true;
            }
         }
      }
   }

   return result;
}

/***********************************************************************************************
DESCRIPTION:   Adds address claimed information to the recorded bus information table
               Note that this function may be VERY expensive.  We must be locked in a critical
               section prior to this call, and will remain locked until we return.  Because of the
               required name comparison, we may remain locked for an extended period of time.  This
               will occur any time a new address claim is received.

ARGUMENTS:     source - Address to add
               nameData - J1939 name information(8 byte array)
               node - Node to add for

RETURN:        None
***********************************************************************************************/
static void J1939Ac_UpdateRecordedBusInfo(J1939_SourceAddress_T source,
                                          const uint8_t *nameData, J1939_Node_T node)
{
#if defined(J1939_RECORD_ADDRESS_CLAIMED_NAMES)
   J1939_Counter_T index;
   J1939_Name_T newName;
   bool addName = true;

   // NOTES:
   // Not checking nameData for valid pointer here because it is gaurenteed by the function
   // that is calling this one.
   // Not range checking node because it was done in the caller.
   // Not using a critical section since we are locking in the caller.

   // Note that there are some masks that don't appear to be strictly necessary on most
   // architectures.  They are needed on architectures that have bytes that are longer than 8 bits
   newName.idNumber =
       MAKEDWORD(MAKEWORD(nameData[0], nameData[1]), MAKEWORD(nameData[2] & 0x1F, 0));
   newName.mfgCode = ((nameData[2] & 0xFF) >> 5) + (((uint16_t)(nameData[3] & 0xFF)) << 3);
   newName.ecuInstance = (J1939_EcuInstance_T)(nameData[4] & 0x07);
   newName.functionInstance = (J1939_FunctionInstance_T)((nameData[4] >> 3) & 0x1F);
   newName.function = (J1939_Function_T)nameData[5];
   newName.vehicleSystem = (J1939_VehicleSystem_T)((nameData[6] >> 1) & 0x7F);
   newName.vehicleSystemInstance = (J1939_VehicleSystemInstance_T)(nameData[7] & 0x0F);
   newName.industryGroup = (J1939_IndustryGroup_T)((nameData[7] >> 4) & 0x07);
   newName.reservedBit = 0;

   if (nameData[7] & 0x80)
   {
      newName.isSelfConfig = true;
   }
   else
   {
      newName.isSelfConfig = false;
   }

   // Record the name/source address information.  If the new name data matches an existing
   // table entry, update the source address for that entry
   for (index = 0; index < J1939Ac_RecordedBusInfoCount[node]; index++)
   {
      if (J1939Ac_IsNameMatch(&newName, &J1939Ac_RecordedBusInfo[node][index].name))
      {
         // We found a matching name.  We only need to update the source address
         J1939Ac_RecordedBusInfo[node][index].source = source;
         addName = false;

         // We break out here for faster performance
         break;
      }
   }

   if (addName)
   {
      if (J1939Ac_RecordedBusInfoCount[node] < CONFIG_J1939_MAX_NODES_IN_SYSTEM)
      {
         J1939Ac_RecordedBusInfo[node][J1939Ac_RecordedBusInfoCount[node]].source = source;
         J1939Ac_RecordedBusInfo[node][J1939Ac_RecordedBusInfoCount[node]].name = newName;
         J1939Ac_RecordedBusInfoCount[node]++;
      }
   }
#else
   J1939_Counter_T index;
   bool fAdd = true;

   (void)nameData;

   for (index = 0; index < node->recorded_bus_info_count; index++)
   {
      if (source == node->recorded_bus_info[index].source)
      {
         fAdd = false;
         break;
      }
   }

   if (fAdd)
   {
      if (node->recorded_bus_info_count < CONFIG_J1939_MAX_NODES_IN_SYSTEM)
      {
         node->recorded_bus_info[node->recorded_bus_info_count].source = source;
         node->recorded_bus_info_count++;
      }
   }
#endif
}

/**************************************************************************************************/
bool J1939Ac_IsNameMatch(J1939_Name_T *name1, J1939_Name_T *name2)
{
   bool result = true;

   if (name1 && name2)
   {
      result = (name1->idNumber == name2->idNumber) && (name1->mfgCode == name2->mfgCode) &&
               (name1->ecuInstance == name2->ecuInstance) &&
               (name1->functionInstance == name2->functionInstance) &&
               (name1->function == name2->function) &&
               (name1->vehicleSystem == name2->vehicleSystem) &&
               (name1->vehicleSystemInstance == name2->vehicleSystemInstance) &&
               (name1->industryGroup == name2->industryGroup) &&
               (name1->isSelfConfig == name2->isSelfConfig) &&
               (name1->reservedBit == name2->reservedBit);
   }
   else
   {
      result = false;
   }

   return result;
}

/**************************************************************************************************/
bool J1939Ac_AddressHasBeenClaimed(J1939_SourceAddress_T address, J1939_Node_T node)
{
   bool result = false;

    for (J1939_Counter_T index = 0; index < node->recorded_bus_info_count; index++)
    {
        if (address == node->recorded_bus_info[index].source)
        {
            result = true;
            // We break out here for faster performance
            break;
        }
    }

   return result;
}

/**************************************************************************************************/
void J1939Ac_NameConfigToByteArray(J1939_Node_T node, uint8_t *array)
{
   if (node && array)
   {
      array[0] = (uint8_t)LOBYTE(LOWORD(node->id_number));
      array[1] = (uint8_t)HIBYTE(LOWORD(node->id_number));
      array[2] = (uint8_t)(((uint8_t)(((J1939_ManufacturerCode_T)node->manufacturer_code & 0x07)
                                                    << 5)) |
                                  (LOBYTE(HIWORD(node->id_number)) & 0x1F));
      array[3] = (uint8_t)((J1939_ManufacturerCode_T)(node->manufacturer_code & 0x7FF) >> 3);
      array[4] =
          (uint8_t)(((node->function_instance & 0x1F) << 3) | (node->ecu_instance & 0x07));
      array[5] = (uint8_t)node->function;
      array[6] = (uint8_t)((node->vehicle_system & 0x7F) << 1);
      array[7] = (uint8_t)(((node->industry_group & 0x07) << 4) |
                                  (node->vehicle_system_instance & 0x0F));

      if (node->arbitrary_address_capable)
      {
         array[7] |= 0x80;
      }

    //   reserved bit is always 0
      array[6] |= (uint8_t)(0 & 0x01);
   }
}

/**************************************************************************************************/
uint8_t J1939Ac_GetClaimedAddressCount(J1939_Node_T node)
{
   return node->recorded_bus_info_count;
}

#if defined(J1939_RECORD_ADDRESS_CLAIMED_NAMES)
/**************************************************************************************************/
bool J1939Ac_GetNameInfoByTableIndex(J1939_Node_T node, J1939_Counter_T index,
                                             J1939_Name_T *name)
{
   bool result = false;
   if ((node < J1939_NUM_NODES) && (index < node->recorded_bus_info_count) && (name != NULL))
   {
      CriticalSection_Lock();
      *name = node->recorded_bus_info[index].name;
      CriticalSection_Unlock();
      result = true;
   }

   return (result);
}
#endif

/**************************************************************************************************/
J1939_SourceAddress_T J1939Ac_GetSourceAddressFromTableIndex(J1939_Node_T node,
                                                             J1939_Counter_T index)
{
   J1939_SourceAddress_T source = J1939_NULL_ADDRESS;

   if ((node < J1939_NUM_NODES) && (index < node->recorded_bus_info_count))
   {
      source = node->recorded_bus_info[index].source;
   }

   return source;
}

/**************************************************************************************************/
uint8_t J1939Ac_GetClaimedTableIndexFromSourceAddress(J1939_SourceAddress_T source,
                                                              J1939_Node_T node)
{
    CriticalSection_Lock();
    for (uint8_t index = 0; index < node->recorded_bus_info_count; index++)
    {
        if (node->recorded_bus_info[index].source == source)
        {
            CriticalSection_Unlock();
            return index;
        }
    }
    CriticalSection_Unlock();

   // No entry found or out of range node, return 0xFF to indicate this.
   return 0xFF;
}

/**************************************************************************************************/
bool J1939Ac_IsReceived(const struct can_frame *message)
{
   J1939_Node_T node;
   J1939_SourceAddress_T source;
//    TODO - need to implement
//    if (message)
//    {
//       node = message->node;
//       if (node < J1939_NUM_NODES)
//       {
//          source = J1939_GetSourceAddress(message->id);

//          CriticalSection_Lock();

//          // If the source address of the address claimed message matches the source address of the
//          // J1939 address node it came in on, do contention checks
//          if ((source == J1939Ac_BusInfo[node].source) &&
//              (J1939Ac_BusInfo[node].source != J1939_NULL_ADDRESS))
//          {
//             // Check name contention with message just received
//             if (J1939Ac_CheckAddressContention(node, message->data))
//             {
//                // We lost contention - go to J1939Ac_State_LostContention
//                J1939Ac_State[node] = J1939Ac_State_LostContention;
//             }
//             else
//             {
//                // Resend original address claimed message
//                if (!J1939_IsVirtualNodeTransmitEnabled(node))
//                {
//                   J1939_EnableVirtualModeTransmit(node);
//                   J1939Ac_ClaimAddress(node);
//                   J1939_DisableVirtualModeTransmit(node);
//                }
//                else
//                {
//                   J1939Ac_ClaimAddress(node);
//                }
//             }
//          }
//          else
//          {
//             J1939Ac_UpdateRecordedBusInfo(source, message->data, node);
//          }
//          CriticalSection_Unlock();
//       }
//    }
   // Handle any application specific operations
   J1939Ac_App_Task(message);

   // Return false so we don't destroy the message.  This allows other functions to also process
   // the message
   return false;
}

/**************************************************************************************************/
J1939_SourceAddress_T J1939Ac_GetSourceAddress(J1939_Node_T node)
{
    if (node < ELEMENTS(J1939Ac_BusInfo))
    {
    //   return (J1939Ac_BusInfo[node].source);
        return (J1939_SourceAddress_T)(node->source_address);
    }
    else
    {
        return J1939_NULL_ADDRESS;
    }
}

/***********************************************************************************************
DESCRIPTION:   Handles the case when we cannot claim an address

ARGUMENTS:     Node we are looking at

RETURN:        None
***********************************************************************************************/
static void J1939Ac_CannotClaimDelay(J1939_Node_T node)
{
   // We do not need a range check on node since it is guaranteed to remain in range previously
   // in the call chain to this function

   // Note that this handles overflow as we want it to.
   if (J1939Timer_GetTime() > node->ac_timestamp)
   {
      CriticalSection_Lock();

      // Enable Tx Queue for this node.
      J1939_EnableVirtualModeTransmit(node);

      // Send address claimed from a null address (cannot claim)
    //   TODO - need to implement
    //   J1939Ac_BusInfo[node].source = J1939_NULL_ADDRESS;
      J1939Ac_ClaimAddress(node);

      // Change state
      node->node_state = J1939Ac_State_CannotClaim;

      // Disable Tx Queue for this node
      J1939_DisableVirtualModeTransmit(node);

      CriticalSection_Unlock();
   }
}

/***********************************************************************************************
DESCRIPTION:   Handles the start of an address claim sequence

ARGUMENTS:     node - J1939 node to handle

RETURN:        None
***********************************************************************************************/
static void J1939Ac_AddressClaimStart(J1939_Node_T node)
{
   // We do not need a range check on node since it is guaranteed to remain in range previously
   // in the call chain to this function

   CriticalSection_Lock();

   // Enable the tx queue
   J1939_EnableVirtualModeTransmit(node);

   // Send the Address Claimed message and mark the start time
   J1939Ac_ClaimAddress(node);
   node->node_state = J1939Ac_State_Waiting;

   // Disable Tx Queue for this J1939 address node.
   J1939_DisableVirtualModeTransmit(node);

   CriticalSection_Unlock();
}

/***********************************************************************************************
DESCRIPTION:   Handles the waiting case of an address claim sequence

ARGUMENTS:     node - J1939 address node to handle

RETURN:        None
***********************************************************************************************/
static void J1939Ac_AddressClaimWaiting(J1939_Node_T node)
{
   // We do not need a range check on node since it is guaranteed to remain in range previously
   // in the call chain to this function

   // Wait at least 250 ms - the first part will take care of rolling over
   if ((J1939Timer_GetTime() - node->ac_timestamp) > J1939_AC_WAIT_TIME)
   {
      CriticalSection_Lock();

      // Enable Tx Queue for this J1939 address node.
      J1939_EnableVirtualModeTransmit(node);

      // Send out an address claimed if we received a request during our
      // J1939Ac_State_Waiting Period
    //   TODO - need to implement
    //   if (J1939Ac_IsRequested[node])
    //   {
    //      J1939Ac_ClaimAddress(node);
    //      J1939Ac_IsRequested[node] = false;
    //   }

      node->node_state = J1939Ac_State_Claimed;

#ifdef USE_LOW_LEVEL_J1939_FILTERING
      UpdateJ1939LowLevelFiltering(node, J1939Ac_BusInfo[node].source);
#endif

      CriticalSection_Unlock();
   }
}

/***********************************************************************************************
DESCRIPTION:   Process the contention lost case for the specified J1939 address node

ARGUMENTS:     node - Node to handle for

RETURN:        None
***********************************************************************************************/
static void J1939Ac_LostContention(J1939_Node_T node)
{
   // We do not need a range check on node since it is guaranteed to remain in range previously
   // in the call chain to this function

#if defined(J1939AC_SELF_CONFIGURABLE)
   J1939_SourceAddress_T tempAddress;

   CriticalSection_Lock();
   tempAddress = J1939Ac_GetNextAlternateAddress(node);

   if (tempAddress == J1939_NULL_ADDRESS)
   {
      // We have run out of alternate addresses.  Do the contention handling
      J1939Ac_CommonContentionHandling(node);
   }
   else
   {
      // Start over to claim a new address.
      J1939Ac_BusInfo[node].source = tempAddress;
      J1939Ac_State[node] = J1939Ac_State_Start;
   }
   CriticalSection_Unlock();
#else
   CriticalSection_Lock();
   J1939Ac_CommonContentionHandling(node);
   CriticalSection_Unlock();
#endif
}

/***********************************************************************************************
DESCRIPTION:   Handles the common conention handling that occurs whether or not you have
               a configurable address

ARGUMENTS:     node - Node to handle

RETURN:        None
***********************************************************************************************/
static void J1939Ac_CommonContentionHandling(J1939_Node_T node)
{
   J1939_Timer_T wTempTime;
   uint16_t w16BitName;

   // We do not need the critical section here, since it is locked in the caller.
   // We do not need a range check on node since it is guaranteed to remain in range previously
   // in the call chain to this function

   // CAN timer is used for a psuedo-random delay, before sending CANNOT CLAIM ADDRESS
   // We don't need to worry about overflow in the calculations below.  We use the low
   // 16-bits of the Manufacturer ID in the J1939 name for this module to help generate the
   // pseudo-random time as well.  This helps reduce the chances that two modules both
   // generate the same pseudo-random number if they contend at startup.  This algorithm was
   // tested in Visual Studio for all possible 16-bit NAME and 16 bit timer combinations and
   // was shown to generate a reasonable good distribution.
   wTempTime = J1939Timer_GetTime();

   // Calculate the time at which we want to send the CANNOT CLAIM message.  Overflow is
   // desired behavior for this calculation when it occurs.  Per section per section 4.2.2.3 of
   // the J1939-81 document, this time should be between zero and 153 milliseconds.  To ensure
   // we send it out in less than 153 milliseconds, we actually generate a time between zero
   // and 145 milliseconds here.
   // TODO - need to implement
//    w16BitName = (uint16_t)(J1939Ac_BusInfo[node].name.idNumber & 0xFFFF);
   node->ac_timestamp = (J1939_Timer_T)(wTempTime + ((wTempTime ^ w16BitName) % 145));

   // Change state
   node->node_state = J1939Ac_State_WaitingCannotClaim;
}

/**************************************************************************************************/
J1939Ac_State_T J1939Ac_GetState(J1939_Node_T node)
{
   J1939Ac_State_T eState = J1939Ac_State_CannotClaim;

   if (node < J1939_NUM_NODES)
   {
      eState = node->node_state;
   }

   return eState;
}

/**************************************************************************************************/
bool J1939Ac_GetNameConfigNode(J1939_Node_T node, J1939_Name_T *kname)
{
   bool result = false;
   if ((node < J1939_NUM_NODES) && (kname != NULL))
   {
      CriticalSection_Lock();
    //   TODO - need to implement
    //   *kname = J1939Ac_BusInfo[node].name;
      CriticalSection_Unlock();
      result = true;
   }

   return result;
}

/**************************************************************************************************/
bool J1939Ac_GetNameArrayNode(J1939_Node_T node, uint8_t *data)
{
   bool result = false;
   if ((node < J1939_NUM_NODES) && (data != NULL))
   {
      CriticalSection_Lock();
    //   TODO - need to implement
    //   J1939Ac_NameConfigToByteArray(&J1939Ac_BusInfo[node].name, data);
      CriticalSection_Unlock();

      result = true;
   }

   return result;
}

/**************************************************************************************************/
bool J1939Ac_SetNameConfigNode(J1939_Node_T node, J1939_Name_T *name)
{
   bool result = false;
   if ((node < J1939_NUM_NODES) && (name != NULL))
   {
      CriticalSection_Lock();
    //   TODO - need to implement
    //   J1939Ac_BusInfo[node].name = *name;
      CriticalSection_Unlock();
      result = true;
   }

   return result;
}

#ifdef J1939AC_SELF_CONFIGURABLE
/**************************************************************************************************/
static J1939_SourceAddress_T J1939Ac_GetNextAlternateAddress(J1939_Node_T node)
{
   J1939_SourceAddress_T source = J1939_NULL_ADDRESS;

   do
   {
      if (J1939Ac_ArbitraryAddressListIndex[node] < J1939AC_ALT_ADDRESS_LIST_LENGTH)
      {
         source = J1939Ac_ArbitraryAddressCapableSourceAddressList
             [node][J1939Ac_ArbitraryAddressListIndex[node]];
         J1939Ac_ArbitraryAddressListIndex[node]++;
      }
      else
      {
         // We have reached the end of our array and therefore have no source addresses.  Break
         // out of the loop and return the null address
         source = J1939_NULL_ADDRESS;
         break;
      }
   } while ((source != J1939_NULL_ADDRESS) && J1939Ac_AddressHasBeenClaimed(source, node));

   return source;
}

/**************************************************************************************************/
void J1939Ac_SetArbitraryListEntry(J1939_Node_T node, J1939_Counter_T index,
                                   J1939_SourceAddress_T source)
{
   if ((node < J1939_NUM_NODES) && (index < J1939AC_ALT_ADDRESS_LIST_LENGTH))
   {
      J1939Ac_ArbitraryAddressCapableSourceAddressList[node][index] = source;
   }
}
#endif
