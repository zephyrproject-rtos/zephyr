/*
 * Copyright (c) 2026 Deere & Company
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _J1939_AC_H_
#define _J1939_AC_H_

#include <zephyr/canbus/j1939.h>

/**
 * @brief Initializes the address claim module
 */
void j1939_ac_init(void);

/**
 * @brief Periodic function for address claim handling.  Handles timeouts, etc
 */
void j1939_ac_task(void);

/**
 * @brief Called when a request PGN for address claimed is received
 *
 * @param node Node on which the request for address claimed was received
 */
void j1939_ac_process_request(j1939_node_t node);

/**
 * @brief Allows the caller to determine if a node has been able to claim it's address
 *
 * @param node Address to check
 *
 * @return bool - true if it has been claimed, false otherwiser
 */
bool j1939_ac_is_successful(j1939_node_t node);

/**
 * @brief PF routing table function for address claim messages
 *
 * @param message pointer to message to be processed.
 *
 * @return Always returns false. This allows the low level CAN driver to pass the message onto other
 * services or otherwise deal with the message
 */
bool j1939_ac_is_received(const struct can_frame *message, j1939_node_t node);

/**
 * @brief Converts name into a uint8_t array
 *
 * @param node pointer to a node structure
 * @param array array to fill in.  Must be at least 8 bytes long
 */
void j1939_ac_name_config_to_byte_array(j1939_node_t node, uint8_t *array);

/**
 * @brief Checks to see if a source address is currently in use by another module on a particular
 * CAN J1939 address node.  This function ONLY looks at other modules, not our own source address
 *
 * @param address Address to look for
 * @param node Node to look on
 *
 * @return true - Address is on the specified J1939 address node. false - Address is not on the
 * specifiednode.
 */
bool j1939_ac_address_has_been_claimed(j1939_source_address_t address, j1939_node_t node);

/**
 * @brief Returns the number of claimed addresses we have seen for a specified J1939 address node.
 * The return value from this function will go up by one for each module it has seen claim an
 * address in a power cycle.
 *
 * @param node Node to look on
 *
 * @return Number of external modules that have claimed an address this power cycle
 */
uint8_t j1939_ac_get_claimed_address_count(j1939_node_t node);

/**
 * @brief Allows the caller to request the J1939 name for a specified table entry on a specified
 * J1939 address node.  The data will be filled into  name
 *
 * @param node J1939 address node to get the information from
 * @param index index into the recorded bus information table to retrieve
 * @param name structure to fill in with data
 *
 * @return true if the specified index on the specified J1939 address node has address claim
 * information, false if any passed in value is invalid or the index/node does not have information
 */
#if defined(J1939_RECORD_ADDRESS_CLAIMED_NAMES)
bool j1939_ac_get_name_info_by_table_index(j1939_node_t node, j1939_counter_t index,
					   j1939_name_t *name);
#endif

/**
 * @brief Compares the names contained in the two passed in structures.  Returns true if they match
 * exactly.
 *
 * @param name1 pointer to the first name to compare
 * @param name2 pointer to the second name to compare
 *
 * @return true if the names match, false otherwise
 */
bool j1939_ac_is_name_match(j1939_name_t *name1, j1939_name_t *name2);

/**
 * @brief Retrieves the source address associated with the specified J1939 address node and
 * recorded bus information table index
 *
 * @param node J1939 address node to get the information for
 * @param index index to retrieve the information for
 *
 * @return The source address at the specified J1939 address node and index if the passed in values
 * are valid, or J1939_NULL_ADDRESS if they are not, or the index doesn't contain information
 */
j1939_source_address_t j1939_ac_get_source_addressFromTableIndex(j1939_node_t node,
								 j1939_counter_t index);

/**
 * @brief Allows the caller to determine which index a specified source address is located at in our
 * recorded bus address table.  This is useful for things like determining in what order modules
 * claimed an address.  Will return 0xFF if the SA is not in the table or if the J1939 address node
 * is out of range
 *
 * @param source source address to look for
 * @param node J1939 address node to look at
 *
 * @return See description
 */
uint8_t j1939_ac_get_claimed_table_index_from_source_address(j1939_source_address_t source,
							     j1939_node_t node);

/**
 * @brief Return the source address of the specified J1939 address node.
 *
 * @param kwNode Node to get the source address of
 *
 * @return Source address of the specified J1939 address node or J1939_NULL_ADDRESS if the J1939
 * address node is invalid
 */
j1939_source_address_t j1939_ac_get_source_address(j1939_node_t node);

/**
 * @brief Gets the current state of the address claim state machine for the specified J1939 address
 * node. Returns J1939_AC_STATE_CANNOT_CLAIM if the J1939 address node is out of range.
 *
 * @param node J1939 address node to retrieve the state for
 *
 * @return See description
 */
j1939_ac_state_t j1939_ac_get_state(j1939_node_t node);

/**
 * @brief Allows the caller to retrieve the J1939 name for the specified J1939 address node
 *
 * @param node J1939 address node to retrieve the information for
 * @param name name structure to fill in
 *
 * @return true on success, false otherwise
 */
bool j1939_ac_get_name_config_node(j1939_node_t node, j1939_name_t *name);

/**
 * @brief Allows the caller to retrieve the J1939 name for the specified J1939 address node
 *
 * @param node J1939 address node to retrieve the information for
 * @param data pointer to the array to fill in.  MUST be at least 8 bytes long
 *
 * @return true on success, false otherwise
 */
bool j1939_ac_get_name_array_node(j1939_node_t node, uint8_t *data);

/**
 * @brief Allows the caller to set a new J1939 name for the specified J1939 address node
 *
 * @param node J1939 address node to set the information for
 * @param name name structure to set the new name to
 *
 * @return true on success, false otherwise
 */
bool j1939_ac_set_name_config_node(j1939_node_t node, j1939_name_t *name);

#ifdef J1939AC_SELF_CONFIGURABLE
/**
 * @brief Function to set a source address in the list of source addresses we use when we lose
 * conention.
 *
 * @param node J1939 address node to set for
 * @param index list index to set
 * @param source source address to use
 */
void j1939_ac_set_arbitrary_list_entry(j1939_node_t node, j1939_counter_t index,
				       j1939_source_address_t source);
#endif

/** Initializes any application specific address claim handling */
void j1939_ac_app_init(void);

/**
 * @brief Application specific handling for address claim.  This function will be called
 * periodically as well as when any address claimed PGN is received
 *
 * @param message Will be NULL when called periodically, will point to the CAN message containing
 * the address claimed PGN otherwise.
 */
void j1939_ac_app_task(const struct can_frame *message);

/**
 * @brief Generates the name for this module and the specified node
 *
 * @param node Node to generate for
 */
void j1939_ac_app_generate_name(j1939_node_t node);

/**
 * @brief This function is called by the address claim module at startup to obtain the first source
 * address the node will attempt to claim.  This function must be able to determine the desired
 * default source address prior to InitCANJ1939() being called. It is therefore called before the
 * rest of the address claim module is initialized, including before j1939_ac_app_init()
 *
 * @param node CAN node to get the default source address for
 *
 * @return Default source address for the specified node
 */
j1939_source_address_t j1939_ac_app_get_default_source_address(j1939_node_t node);

#endif
