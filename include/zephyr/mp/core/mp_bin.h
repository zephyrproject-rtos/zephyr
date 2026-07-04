/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Main header for mp_bin.
 */

#ifndef ZEPHYR_INCLUDE_MP_CORE_MP_BIN_H_
#define ZEPHYR_INCLUDE_MP_CORE_MP_BIN_H_

/**
 * @defgroup mp_bin Bins
 * @ingroup mp_core
 * @brief Container elements that hold and manage child elements.
 * @{
 */

#include <stdint.h>

#include <zephyr/sys/dlist.h>

#include <zephyr/mp/core/mp_bus.h>
#include <zephyr/mp/core/mp_element.h>

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
 * @param self Pointer to the @ref mp_element to initialize as a bin
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
 * @param bin Pointer to the @ref mp_bin to add elements to
 * @param element First @ref mp_element to add
 * @param ... Additional mp_element pointers, terminated by NULL
 *
 * @return 0 on success, negative errno on failure
 */
int mp_bin_add(struct mp_bin *bin, struct mp_element *element, ...);

/**
 * @brief Bin state change function
 *
 * Handles state changes for the bin by propagating the state change to all
 * child elements in the appropriate order. The bin manages the topology
 * and ensures proper sequencing of state changes.
 *
 * @param element Pointer to the @ref mp_element (bin) changing state
 * @param transition The state transition being performed
 *
 * @return State change return value indicating success, failure, or async operation
 */
enum mp_state_change_return mp_bin_change_state_func(struct mp_element *element,
						     enum mp_state_change transition);

/** @} */

#endif /* ZEPHYR_INCLUDE_MP_CORE_MP_BIN_H_ */
