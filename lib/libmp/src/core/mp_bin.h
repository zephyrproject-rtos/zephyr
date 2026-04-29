/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Main header for mp_bin.
 */

#ifndef __MP_BIN_H__
#define __MP_BIN_H__

#include <stdint.h>

#include <zephyr/sys/dlist.h>

#include "mp_element.h"

#define MP_BIN(self) ((struct mp_bin *)self)

/**
 * @brief Bin structure
 *
 * A container element that can hold multiple child elements.
 *
 * A bin manages the state changes of its children and handles the topology
 * of the pipeline elements within it.
 */
struct mp_bin {
	/** Base element structure */
	struct mp_element element;
	/** Message bus to communicate with application */
	struct mp_bus bus;
	/** Number of children in the bin */
	int children_num;
	/** List of children elements in the bin */
	sys_dlist_t children;
};

/**
 * @brief Initialize a bin
 *
 * Initializes the bin structure and sets up the necessary function pointers
 * and data structures.
 *
 * @param self Pointer to the @ref struct mp_element to initialize as a bin
 */
void mp_bin_init(struct mp_element *self);

/**
 * @brief Add elements to a bin
 *
 * Adds the given element(s) to the bin.
 *
 * An element can only be added to one bin. Element names must be unique within the bin.
 *
 * The function accepts a variable number of elements, terminated by NULL.
 *
 * If the element's pads are linked to other pads, the pads will be unlinked
 * before the element is added to the bin.
 *
 * @param bin Pointer to the @ref struct mp_bin to add elements to
 * @param element First @ref struct mp_element to add
 * @param ... Additional mp_element pointers, terminated by NULL
 *
 * @retval true All elements were successfully added to the bin
 * @retval false One or more elements could not be added
 */
bool mp_bin_add(struct mp_bin *bin, struct mp_element *element, ...);

/**
 * @brief Bin state change function
 *
 * Handles state changes for the bin by propagating the state change to all
 * child elements in the appropriate order. The bin manages the topology
 * and ensures proper sequencing of state changes.
 *
 * @param element Pointer to the @ref struct mp_element (bin) changing state
 * @param transition The state transition being performed
 *
 * @return State change return value indicating success, failure, or async operation
 */
enum mp_state_change_return mp_bin_change_state_func(struct mp_element *element,
						     enum mp_state_change transition);

#endif /* __MP_BIN_H__ */
