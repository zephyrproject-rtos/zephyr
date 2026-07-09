/*
 * Copyright (c) 2026 Deere & Company
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _J1939_AC_H_
#define _J1939_AC_H_

#include <zephyr/canbus/j1939.h>
#include <J1939_Cfg.h>


/***********************************************************************************************
DESCRIPTION:   Initializes the address claim module

ARGUMENTS:     None

RETURN:        None
***********************************************************************************************/
void J1939Ac_Init(void);

/***********************************************************************************************
DESCRIPTION:   Periodic function for address claim handling.  Handles timeouts, etc

ARGUMENTS:     None

RETURN:        None
***********************************************************************************************/
void J1939Ac_Task(void);

/***********************************************************************************************
DESCRIPTION:   Called when a request PGN for address claimed is received

ARGUMENTS:     node - Node on which the request for address claimed was received

RETURN:        None
***********************************************************************************************/
void J1939Ac_ProcessRequest(J1939_Node_T node);

/***********************************************************************************************
DESCRIPTION:   Allows the caller to determine if a node has been able to claim it's address

ARGUMENTS:     node - Address to check

RETURN:        bool - true if it has been claimed, false otherwiser
***********************************************************************************************/
bool J1939Ac_IsSuccessful(J1939_Node_T node);

/***********************************************************************************************
DESCRIPTION:   PF routing table function for address claim messages

ARGUMENTS:     message - pointer to message to be processed.

RETURN:        Always returns false. This allows the low level CAN driver to pass the message
               onto other services or otherwise deal with the message
***********************************************************************************************/
bool J1939Ac_IsReceived(const struct can_frame *message);

/***********************************************************************************************
DESCRIPTION:   Converts name into a uint8_t array

ARGUMENTS:     node - pointer to a node structure
               array - array to fill in.  Must be at least 8 bytes long

RETURN:        None
***********************************************************************************************/
void J1939Ac_NameConfigToByteArray(J1939_Node_T node, uint8_t *array);

/***********************************************************************************************
DESCRIPTION:   Checks to see if a source address is currently in use by another module on a
               particular CAN J1939 address node.  This function ONLY looks at other modules, not
our own source address

ARGUMENTS:     address - Address to look for
               node - Node to look on

RETURN:        true - Address is on the specified J1939 address node.
               false - Address is not on the specifiednode.
***********************************************************************************************/
bool J1939Ac_AddressHasBeenClaimed(J1939_SourceAddress_T address, J1939_Node_T node);

/***********************************************************************************************
DESCRIPTION:   Returns the number of claimed addresses we have seen for a specified J1939 address
node. The return value from this function will go up by one for each module it has seen claim an
address in a power cycle.

ARGUMENTS:     node - Node to look on

RETURN:        Number of external modules that have claimed an address this power cycle
***********************************************************************************************/
uint8_t J1939Ac_GetClaimedAddressCount(J1939_Node_T node);

/***********************************************************************************************
DESCRIPTION:   Allows the caller to request the J1939 name for a specified table entry on a
               specified J1939 address node.  The data will be filled into  name

ARGUMENTS:     node - J1939 address node to get the information from
               index - index into the recorded bus information table to retrieve
               name - structure to fill in with data

RETURN:        true if the specified index on the specified J1939 address node has address claim
information, false if any passed in value is invalid or the index/node does not have information
***********************************************************************************************/
#if defined(J1939_RECORD_ADDRESS_CLAIMED_NAMES)
bool J1939Ac_GetNameInfoByTableIndex(J1939_Node_T node, J1939_Counter_T index,
                                             J1939_Name_T *name);
#endif

/***********************************************************************************************
DESCRIPTION:   Compares the names contained in the two passed in structures.  Returns true if
               they match exactly.

ARGUMENTS:     name1 - pointer to the first name to compare
               name2 - pointer to the second name to compare

RETURN:        true if the names match, false otherwise
***********************************************************************************************/
bool J1939Ac_IsNameMatch(J1939_Name_T *name1, J1939_Name_T *name2);

/***********************************************************************************************
DESCRIPTION:   Retrieves the source address associated with the the specified J1939 address node and
recorded bus information table index

ARGUMENTS:     node - J1939 address node to get the information for
               index - index to retrieve the information for

RETURN:        The source address at the specified J1939 address node and index if the passed in
values are valid, or J1939_NULL_ADDRESS if they are not, or the index doesn't contain information
***********************************************************************************************/
J1939_SourceAddress_T J1939Ac_GetSourceAddressFromTableIndex(J1939_Node_T node,
                                                             J1939_Counter_T index);

/***********************************************************************************************
DESCRIPTION:   Allows the caller to determine which index a specified source address is located
               at in our recorded bus address table.  This is useful for things like determining
               in what order modules claimed an address.  Will return 0xFF if the SA is not in the
               table or if the J1939 address node is out of range

ARGUMENTS:     source - source address to look for
               node - J1939 address node to look at

RETURN:        See description
***********************************************************************************************/
uint8_t J1939Ac_GetClaimedTableIndexFromSourceAddress(J1939_SourceAddress_T source,
                                                              J1939_Node_T node);

/***********************************************************************************************
DESCRIPTION:   Return the source address of the specified J1939 address node.

ARGUMENTS:     kwNode - Node to get the source address of

RETURN:        Source address of the specified J1939 address node or J1939_NULL_ADDRESS if the J1939
address node is invalid
***********************************************************************************************/
J1939_SourceAddress_T J1939Ac_GetSourceAddress(J1939_Node_T node);

/***********************************************************************************************
DESCRIPTION:   Gets the current state of the address claim state machine for the specified J1939
address node. Returns J1939Ac_State_CannotClaim if the J1939 address node is out of range.

ARGUMENTS:     node - J1939 address node to retrieve the state for

RETURN:        See description
***********************************************************************************************/
J1939Ac_State_T J1939Ac_GetState(J1939_Node_T node);

/***********************************************************************************************
DESCRIPTION:   Allows the caller to retrieve the J1939 name for the specified J1939 address node

ARGUMENTS:     node - J1939 address node to retrieve the information for
               name - name structure to fill in

RETURN:        true on success, false otherwise
***********************************************************************************************/
bool J1939Ac_GetNameConfigNode(J1939_Node_T node, J1939_Name_T *name);

/***********************************************************************************************
DESCRIPTION:   Allows the caller to retrieve the J1939 name for the specified J1939 address node

ARGUMENTS:     node - J1939 address node to retrieve the information for
               data - pointer to the array to fill in.  MUST be at least 8 bytes long

RETURN:        true on success, false otherwise
***********************************************************************************************/
bool J1939Ac_GetNameArrayNode(J1939_Node_T node, uint8_t *data);

/***********************************************************************************************
DESCRIPTION:   Allows the caller to set a new J1939 name for the specified J1939 address node

ARGUMENTS:     node - J1939 address node to set the information for
               name - name structure to set the new name to

RETURN:        true on success, false otherwise
***********************************************************************************************/
bool J1939Ac_SetNameConfigNode(J1939_Node_T node, J1939_Name_T *name);

#ifdef J1939AC_SELF_CONFIGURABLE
/***********************************************************************************************
DESCRIPTION:   Function to set a source address in the list of source addresses we use when
               we lose conention.

ARGUMENTS:     node - J1939 address node to set for
               index - list index to set
               source - source address to use

RETURN:        None
***********************************************************************************************/
void J1939Ac_SetArbitraryListEntry(J1939_Node_T node, J1939_Counter_T index,
                                   J1939_SourceAddress_T source);
#endif

/// Initializes any application specific address claim handling
void J1939Ac_App_Init(void);

/***********************************************************************************************
DESCRIPTION:   Application specific handling for address claim.  This function will be called
               periodically as well as when any address claimed PGN is received

ARGUMENTS:     message - Will be NULL when called periodically, will point to the CAN messsage
                              containing the address claimed PGN otherwise.
***********************************************************************************************/
void J1939Ac_App_Task(const struct can_frame *message);

/***********************************************************************************************
DESCRIPTION:   Generates the name for this module and the specified node

ARGUMENTS:     node - Node to generate for

RETURN:        None
***********************************************************************************************/
void J1939Ac_App_GenerateName(J1939_Node_T node);

/***********************************************************************************************
DESCRIPTION:   This function is called by the address claim module at startup to obtain the
               first source address the node will attempt to claim.  This function must be able
               to determine the desired default source address prior to InitCANJ1939() being called.
               It is therefore called before the rest of the address claim module is initialized,
               including before J1939Ac_App_Init()

ARGUMENTS:     node - CAN node to get the default source address for

RETURN:        Default source address for the specified node
***********************************************************************************************/
J1939_SourceAddress_T J1939Ac_App_GetDefaultSourceAddress(J1939_Node_T node);

#endif
