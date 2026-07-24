/*
 * Copyright (c) 2026 Deere & Company
 * SPDX-License-Identifier: Apache-2.0
 */
#include "J1939Ac.h"
#include "J1939Timer.h"
#include <Can_Transmit.h>
#include <J1939Timer.h>

/*==========================================================================
 * Possible Future Improvements
 *========================================================================*/
// 1. Store all j1939_name_t structures as the packed 8 byte array instead of
// in the unpacked
//    structure.  This would save 2*CONFIG_J1939_NODES_COUNT*6*J1939_MAX_DEVICES_IN_SYSTEM bytes
//    of RAM at the cost of runtime conversion between the two.  We have not yet made this change
//    because we have been more processor limited than RAM limited.
//

extern struct j1939_dt_node_cfg* j1939_nodes[];

#define J1939_AC_WAIT_TIME ((j1939_timer_t)250) // Milliseconds
#define J1939_PR_ADDRESS_CLAIMED (J1939_Priority_6)
#define J1939_PF_ADDRESS_CLAIMED ((j1939_pdu_format_t)0xEE)
#define J1939_NAME_LENGTH ((j1939_counter_t)8)

typedef struct j1939_ac_bus_info_S
{
   j1939_source_address_t source; // Source address associated with the name pointer below
   j1939_name_t name;
} j1939_ac_bus_info_T;


static bool j1939_ac_is_requested[CONFIG_J1939_NODES_COUNT];
static j1939_ac_bus_info_T j1939_ac_bus_info[CONFIG_J1939_NODES_COUNT];

#ifdef J1939AC_SELF_CONFIGURABLE
// If we are arbitrary address capable, then we need a list of the addresses we will attempt to
// claim, in the order we will attempt to claim them, when we lose arbitration.  The application
// developer is responsible for filling in this information in j1939_ac_app_init()
// using the j1939_ac_set_arbitrary_list_entry() function
static j1939_counter_t
    J1939Ac_ArbitraryAddressListIndex[CONFIG_J1939_NODES_COUNT]; // Keeps track of the index of
                                                        // the next address to use
static j1939_source_address_t
    J1939Ac_ArbitraryAddressCapableSourceAddressList[CONFIG_J1939_NODES_COUNT]
                                                    [J1939AC_ALT_ADDRESS_LIST_LENGTH];
#endif

static void j1939_ac_cannot_claim_delay(j1939_node_t node);
static void j1939_ac_address_claim_start(j1939_node_t node);
static void j1939_ac_address_claim_waiting(j1939_node_t node);
static void j1939_ac_lost_contention(j1939_node_t node);
static void j1939_ac_common_contention_handling(j1939_node_t node);
static bool j1939_ac_check_address_contention(j1939_node_t node, const uint8_t *data);
static void j1939_ac_claim_address(j1939_node_t node);
static void j1939_ac_update_recorded_bus_info(j1939_source_address_t source,
                                          const uint8_t *nameData, j1939_node_t node);

#ifdef J1939AC_SELF_CONFIGURABLE
static j1939_source_address_t j1939_ac_get_next_alternate_address(j1939_node_t node);
#endif

/**************************************************************************************************/
void j1939_ac_init(void)
{
   for (uint16_t node_index = 0; node_index < CONFIG_J1939_NODES_COUNT; node_index++)
   {
        j1939_node_t dt_node = j1939_nodes[node_index];
      // Retrieve the default source address we will claim from the application specific module
      j1939_ac_bus_info[node_index].source = j1939_ac_app_get_default_source_address(dt_node);

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

      j1939_ac_app_generate_name(dt_node);

      // Disable Transmit queue(s) until we have claimed our address
      j1939_disable_virtual_mode_transmit(dt_node);

      // Prepare to claim the address
      dt_node->node_state = J1939_AC_STATE_WAITING_STARTUP_INIT;
      j1939_ac_is_requested[node_index] = false;

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

   j1939_ac_app_init();
}

/**************************************************************************************************/
void j1939_ac_task(void)
{
   // Loop through all the nodes
   for (uint16_t node_index = 0; node_index < CONFIG_J1939_NODES_COUNT; node_index++)
   {
        j1939_node_t node = j1939_nodes[node_index];
      switch (node->node_state)
      {
      case J1939_AC_STATE_WAITING_STARTUP_INIT:
#if defined(CAN_AUTOBAUD_ENABLE)
         if (!Can_Autobaud_IsInProgress(j1939_get_can_node(node)))
         {
            // We have a valid baud rate.  We can start address claim
            node->node_state = J1939_AC_STATE_START;
         }
#else
         node->node_state = J1939_AC_STATE_START;
#endif
         break;

      case J1939_AC_STATE_START:
         j1939_ac_address_claim_start(node);
         break;

      case J1939_AC_STATE_WAITING:
         j1939_ac_address_claim_waiting(node);
         break;

      case J1939_AC_STATE_CLAIMED:
         // Don't change anything
         break;

      case J1939_AC_STATE_LOST_CONTENTION:
         j1939_ac_lost_contention(node);
         break;

      case J1939_AC_STATE_WAITING_CANNOT_CLAIM:
         j1939_ac_cannot_claim_delay(node);
         break;

      case J1939_AC_STATE_CANNOT_CLAIM:
         // Don't change anything
         break;

      default:
         // Shouldn't ever get here.  If we do we go to the cannot claim state.  If someone
         // requests address claim again, we will reset and try to claim agin
         node->node_state = J1939_AC_STATE_CANNOT_CLAIM;
         break;
      }
   }

   // Handle any application specific operations.  Pass NULL since we are calling
   // from the periodic function.
   j1939_ac_app_task(NULL);
}

/***********************************************************************************************
DESCRIPTION:   Initiates an address claim for the specified node

ARGUMENTS:     node - Node to initiate an address claim for

RETURN:        None
***********************************************************************************************/
static void j1939_ac_claim_address(j1939_node_t node)
{
   j1939_arbitration_t id;
   uint8_t name[CAN_MAX_DLC];

   // We do not need a range check on node since it is guaranteed to remain in range previously
   // in the call chain to this function
   // This function assumes that we are in a critical section when called.  The caller is
   // responsible for handling this

   id = j1939_build_message_id(0, 0, J1939_PR_ADDRESS_CLAIMED,
                             j1939_build_pgn_from_pdu(J1939_PF_ADDRESS_CLAIMED, J1939_GLOBAL_ADDRESS),
                             node->source_address);

   j1939_ac_name_config_to_byte_array(node, name);

   if (j1939_build_and_queue_message(node, id, J1939_NAME_LENGTH, true, name))
   {
      // If this is the first time the node is claiming its address, set the timestamp
      if (node->node_state == J1939_AC_STATE_START)
      {
         node->ac_timestamp = j1939_timer_get_time();
      }

      Can_Transmit_SendQueues();
   }
}

/**************************************************************************************************/
void j1939_ac_process_request(j1939_node_t node)
{
    // If were in state J1939_AC_STATE_CLAIMED, we can send our address claimed out,
    // otherwise we need to set a flag to send out an address claimed once we reach the
    // J1939_AC_STATE_CLAIMED state.
    CriticalSection_Lock();
    if (node->node_state == J1939_AC_STATE_CLAIMED)
    {
        // Send out an address claimed.  Queues will always be enabled if we are
        // J1939_AC_STATE_CLAIMED
        j1939_ac_claim_address(node);
    }
    else if (node->node_state == J1939_AC_STATE_CANNOT_CLAIM)
    {
        // If we cannot claim an address, we still have to respond to the request.
        j1939_enable_virtual_mode_transmit(node);
        j1939_ac_claim_address(node);
        j1939_disable_virtual_mode_transmit(node);
    }
    else
    {
        // Set a flag to send address claimed once we reach that state
    //  TODO - implement this
    //  j1939_ac_is_requested[node] = true;
    }
    CriticalSection_Unlock();
}

/**************************************************************************************************/
bool j1939_ac_is_successful(j1939_node_t node)
{
    // not range checking since it is configured by device tree
    return (node->node_state == J1939_AC_STATE_CLAIMED);
}

/***********************************************************************************************
DESCRIPTION:   Checks for address contention

ARGUMENTS:     node - Node to check for contention on
               data - Name data to check with

RETURN:        bool - true if we lost contention, false if we won
***********************************************************************************************/
static bool j1939_ac_check_address_contention(j1939_node_t node, const uint8_t *data)
{
   j1939_counter_t index = 7; // Names are 8 bytes long, so start at the last index
   bool result = false;
   bool continueOn = true;
   uint8_t name[CAN_MAX_DLC];

   // We do not need the critical section here, since it is locked in the caller.
   // We do not need a range check on node since it is guaranteed to remain in range previously
   // in the call chain to this function

   if (data != NULL)
   {
      j1939_ac_name_config_to_byte_array(node, name);

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
static void j1939_ac_update_recorded_bus_info(j1939_source_address_t source,
                                          const uint8_t *nameData, j1939_node_t node)
{
#if defined(J1939_RECORD_ADDRESS_CLAIMED_NAMES)
   j1939_counter_t index;
   j1939_name_t newName;
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
   newName.ecuInstance = (j1939_ecu_instance_t)(nameData[4] & 0x07);
   newName.functionInstance = (j1939_function_instance_t)((nameData[4] >> 3) & 0x1F);
   newName.function = (j1939_function_t)nameData[5];
   newName.vehicleSystem = (j1939_vehicle_system_t)((nameData[6] >> 1) & 0x7F);
   newName.vehicleSystemInstance = (j1939_vehicle_system_instance_t)(nameData[7] & 0x0F);
   newName.industryGroup = (j1939_industry_group_t)((nameData[7] >> 4) & 0x07);
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
      if (j1939_ac_is_name_match(&newName, &J1939Ac_RecordedBusInfo[node][index].name))
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
   j1939_counter_t index;
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
bool j1939_ac_is_name_match(j1939_name_t *name1, j1939_name_t *name2)
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
bool j1939_ac_address_has_been_claimed(j1939_source_address_t address, j1939_node_t node)
{
   bool result = false;

    for (j1939_counter_t index = 0; index < node->recorded_bus_info_count; index++)
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
void j1939_ac_name_config_to_byte_array(j1939_node_t node, uint8_t *array)
{
   if (node && array)
   {
      array[0] = (uint8_t)LOBYTE(LOWORD(node->id_number));
      array[1] = (uint8_t)HIBYTE(LOWORD(node->id_number));
      array[2] = (uint8_t)(((uint8_t)(((j1939_manufacturer_code_t)node->manufacturer_code & 0x07)
                                                    << 5)) |
                                  (LOBYTE(HIWORD(node->id_number)) & 0x1F));
      array[3] = (uint8_t)((j1939_manufacturer_code_t)(node->manufacturer_code & 0x7FF) >> 3);
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
uint8_t j1939_ac_get_claimed_address_count(j1939_node_t node)
{
   return node->recorded_bus_info_count;
}

#if defined(J1939_RECORD_ADDRESS_CLAIMED_NAMES)
/**************************************************************************************************/
bool j1939_ac_get_name_info_by_table_index(j1939_node_t node, j1939_counter_t index,
                                             j1939_name_t *name)
{
   bool result = false;
   if ((node < CONFIG_J1939_NODES_COUNT) && (index < node->recorded_bus_info_count) && (name != NULL))
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
j1939_source_address_t j1939_ac_get_source_addressFromTableIndex(j1939_node_t node,
                                                             j1939_counter_t index)
{
   j1939_source_address_t source = J1939_NULL_ADDRESS;

   if ((node < CONFIG_J1939_NODES_COUNT) && (index < node->recorded_bus_info_count))
   {
      source = node->recorded_bus_info[index].source;
   }

   return source;
}

/**************************************************************************************************/
uint8_t j1939_ac_get_claimed_table_index_from_source_address(j1939_source_address_t source,
                                                              j1939_node_t node)
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
bool j1939_ac_is_received(const struct can_frame *message)
{
   j1939_node_t node;
   j1939_source_address_t source;
//    TODO - need to implement
//    if (message)
//    {
//       node = message->node;
//       if (node < CONFIG_J1939_NODES_COUNT)
//       {
//          source = j1939_get_source_address(message->id);

//          CriticalSection_Lock();

//          // If the source address of the address claimed message matches the source address of the
//          // J1939 address node it came in on, do contention checks
//          if ((source == j1939_ac_bus_info[node].source) &&
//              (j1939_ac_bus_info[node].source != J1939_NULL_ADDRESS))
//          {
//             // Check name contention with message just received
//             if (j1939_ac_check_address_contention(node, message->data))
//             {
//                // We lost contention - go to J1939_AC_STATE_LOST_CONTENTION
//                J1939Ac_State[node] = J1939_AC_STATE_LOST_CONTENTION;
//             }
//             else
//             {
//                // Resend original address claimed message
//                if (!j1939_is_virtual_node_transmit_enabled(node))
//                {
//                   j1939_enable_virtual_mode_transmit(node);
//                   j1939_ac_claim_address(node);
//                   j1939_disable_virtual_mode_transmit(node);
//                }
//                else
//                {
//                   j1939_ac_claim_address(node);
//                }
//             }
//          }
//          else
//          {
//             j1939_ac_update_recorded_bus_info(source, message->data, node);
//          }
//          CriticalSection_Unlock();
//       }
//    }
   // Handle any application specific operations
   j1939_ac_app_task(message);

   // Return false so we don't destroy the message.  This allows other functions to also process
   // the message
   return false;
}

/**************************************************************************************************/
j1939_source_address_t j1939_ac_get_source_address(j1939_node_t node)
{
    if (node < ELEMENTS(j1939_ac_bus_info))
    {
    //   return (j1939_ac_bus_info[node].source);
        return (j1939_source_address_t)(node->source_address);
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
static void j1939_ac_cannot_claim_delay(j1939_node_t node)
{
   // We do not need a range check on node since it is guaranteed to remain in range previously
   // in the call chain to this function

   // Note that this handles overflow as we want it to.
   if (j1939_timer_get_time() > node->ac_timestamp)
   {
      CriticalSection_Lock();

      // Enable Tx Queue for this node.
      j1939_enable_virtual_mode_transmit(node);

      // Send address claimed from a null address (cannot claim)
    //   TODO - need to implement
    //   j1939_ac_bus_info[node].source = J1939_NULL_ADDRESS;
      j1939_ac_claim_address(node);

      // Change state
      node->node_state = J1939_AC_STATE_CANNOT_CLAIM;

      // Disable Tx Queue for this node
      j1939_disable_virtual_mode_transmit(node);

      CriticalSection_Unlock();
   }
}

/***********************************************************************************************
DESCRIPTION:   Handles the start of an address claim sequence

ARGUMENTS:     node - J1939 node to handle

RETURN:        None
***********************************************************************************************/
static void j1939_ac_address_claim_start(j1939_node_t node)
{
   // We do not need a range check on node since it is guaranteed to remain in range previously
   // in the call chain to this function

   CriticalSection_Lock();

   // Enable the tx queue
   j1939_enable_virtual_mode_transmit(node);

   // Send the Address Claimed message and mark the start time
   j1939_ac_claim_address(node);
   node->node_state = J1939_AC_STATE_WAITING;

   // Disable Tx Queue for this J1939 address node.
   j1939_disable_virtual_mode_transmit(node);

   CriticalSection_Unlock();
}

/***********************************************************************************************
DESCRIPTION:   Handles the waiting case of an address claim sequence

ARGUMENTS:     node - J1939 address node to handle

RETURN:        None
***********************************************************************************************/
static void j1939_ac_address_claim_waiting(j1939_node_t node)
{
   // We do not need a range check on node since it is guaranteed to remain in range previously
   // in the call chain to this function

   // Wait at least 250 ms - the first part will take care of rolling over
   if ((j1939_timer_get_time() - node->ac_timestamp) > J1939_AC_WAIT_TIME)
   {
      CriticalSection_Lock();

      // Enable Tx Queue for this J1939 address node.
      j1939_enable_virtual_mode_transmit(node);

      // Send out an address claimed if we received a request during our
      // J1939_AC_STATE_WAITING Period
    //   TODO - need to implement
    //   if (j1939_ac_is_requested[node])
    //   {
    //      j1939_ac_claim_address(node);
    //      j1939_ac_is_requested[node] = false;
    //   }

      node->node_state = J1939_AC_STATE_CLAIMED;

#ifdef USE_LOW_LEVEL_J1939_FILTERING
      UpdateJ1939LowLevelFiltering(node, j1939_ac_bus_info[node].source);
#endif

      CriticalSection_Unlock();
   }
}

/***********************************************************************************************
DESCRIPTION:   Process the contention lost case for the specified J1939 address node

ARGUMENTS:     node - Node to handle for

RETURN:        None
***********************************************************************************************/
static void j1939_ac_lost_contention(j1939_node_t node)
{
   // We do not need a range check on node since it is guaranteed to remain in range previously
   // in the call chain to this function

#if defined(J1939AC_SELF_CONFIGURABLE)
   j1939_source_address_t tempAddress;

   CriticalSection_Lock();
   tempAddress = j1939_ac_get_next_alternate_address(node);

   if (tempAddress == J1939_NULL_ADDRESS)
   {
      // We have run out of alternate addresses.  Do the contention handling
      j1939_ac_common_contention_handling(node);
   }
   else
   {
      // Start over to claim a new address.
      j1939_ac_bus_info[node].source = tempAddress;
      J1939Ac_State[node] = J1939_AC_STATE_START;
   }
   CriticalSection_Unlock();
#else
   CriticalSection_Lock();
   j1939_ac_common_contention_handling(node);
   CriticalSection_Unlock();
#endif
}

/***********************************************************************************************
DESCRIPTION:   Handles the common conention handling that occurs whether or not you have
               a configurable address

ARGUMENTS:     node - Node to handle

RETURN:        None
***********************************************************************************************/
static void j1939_ac_common_contention_handling(j1939_node_t node)
{
   j1939_timer_t wTempTime;
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
   wTempTime = j1939_timer_get_time();

   // Calculate the time at which we want to send the CANNOT CLAIM message.  Overflow is
   // desired behavior for this calculation when it occurs.  Per section per section 4.2.2.3 of
   // the J1939-81 document, this time should be between zero and 153 milliseconds.  To ensure
   // we send it out in less than 153 milliseconds, we actually generate a time between zero
   // and 145 milliseconds here.
   // TODO - need to implement
//    w16BitName = (uint16_t)(j1939_ac_bus_info[node].name.idNumber & 0xFFFF);
   node->ac_timestamp = (j1939_timer_t)(wTempTime + ((wTempTime ^ w16BitName) % 145));

   // Change state
   node->node_state = J1939_AC_STATE_WAITING_CANNOT_CLAIM;
}

/**************************************************************************************************/
j1939_ac_state_t j1939_ac_get_state(j1939_node_t node)
{
   j1939_ac_state_t eState = J1939_AC_STATE_CANNOT_CLAIM;

   if (node < CONFIG_J1939_NODES_COUNT)
   {
      eState = node->node_state;
   }

   return eState;
}

/**************************************************************************************************/
bool j1939_ac_get_name_config_node(j1939_node_t node, j1939_name_t *kname)
{
   bool result = false;
   if ((node < CONFIG_J1939_NODES_COUNT) && (kname != NULL))
   {
      CriticalSection_Lock();
    //   TODO - need to implement
    //   *kname = j1939_ac_bus_info[node].name;
      CriticalSection_Unlock();
      result = true;
   }

   return result;
}

/**************************************************************************************************/
bool j1939_ac_get_name_array_node(j1939_node_t node, uint8_t *data)
{
   bool result = false;
   if ((node < CONFIG_J1939_NODES_COUNT) && (data != NULL))
   {
      CriticalSection_Lock();
    //   TODO - need to implement
    //   j1939_ac_name_config_to_byte_array(&j1939_ac_bus_info[node].name, data);
      CriticalSection_Unlock();

      result = true;
   }

   return result;
}

/**************************************************************************************************/
bool j1939_ac_set_name_config_node(j1939_node_t node, j1939_name_t *name)
{
   bool result = false;
   if ((node < CONFIG_J1939_NODES_COUNT) && (name != NULL))
   {
      CriticalSection_Lock();
    //   TODO - need to implement
    //   j1939_ac_bus_info[node].name = *name;
      CriticalSection_Unlock();
      result = true;
   }

   return result;
}

#ifdef J1939AC_SELF_CONFIGURABLE
/**************************************************************************************************/
static j1939_source_address_t j1939_ac_get_next_alternate_address(j1939_node_t node)
{
   j1939_source_address_t source = J1939_NULL_ADDRESS;

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
   } while ((source != J1939_NULL_ADDRESS) && j1939_ac_address_has_been_claimed(source, node));

   return source;
}

/**************************************************************************************************/
void j1939_ac_set_arbitrary_list_entry(j1939_node_t node, j1939_counter_t index,
                                   j1939_source_address_t source)
{
   if ((node < CONFIG_J1939_NODES_COUNT) && (index < J1939AC_ALT_ADDRESS_LIST_LENGTH))
   {
      J1939Ac_ArbitraryAddressCapableSourceAddressList[node][index] = source;
   }
}
#endif
