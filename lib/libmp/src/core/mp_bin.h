/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Main header for MpBin.
 */

#ifndef __MP_BIN_H__
#define __MP_BIN_H__

#include <stdint.h>

#include <zephyr/sys/dlist.h>

#include "mp_element.h"

typedef struct _MpBin MpBin;

#define MP_BIN(self) ((MpBin *)self)

/**
 * @brief Bin structure
 *
 * A container element that can hold multiple child elements.
 *
 * A bin manages the state changes of its children and handles the topology
 * of the pipeline elements within it.
 */
struct _MpBin {
	/** Base element structure */
	MpElement element;
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
 * @param self Pointer to the @ref MpElement to initialize as a bin
 */
void mp_bin_init(MpElement *self);

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
 * @param bin Pointer to the @ref MpBin to add elements to
 * @param element First @ref MpElement to add
 * @param ... Additional MpElement pointers, terminated by NULL
 *
 * @retval true All elements were successfully added to the bin
 * @retval false One or more elements could not be added
 */
bool mp_bin_add(MpBin *bin, MpElement *element, ...);

/**
 * @brief Bin state change function
 *
 * Handles state changes for the bin by propagating the state change to all
 * child elements in the appropriate order. The bin manages the topology
 * and ensures proper sequencing of state changes.
 *
 * @param element Pointer to the @ref MpElement (bin) changing state
 * @param transition The state transition being performed
 *
 * @return State change return value indicating success, failure, or async operation
 */
MpStateChangeReturn mp_bin_change_state_func(MpElement *element, MpStateChange transition);

#endif /* __MP_BIN_H__ */
