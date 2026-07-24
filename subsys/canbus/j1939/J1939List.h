/*
 * Copyright (c) 2026 Deere & Company
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _LINKED_LIST_H_
#define _LINKED_LIST_H_

#include <stddef.h>
#include <stdint.h>
#include <zephyr/sys/util.h>

typedef uint8_t j1939_list_list_t;

typedef uint8_t j1939_list_size_t;

typedef uint8_t j1939_list_element_t;

/*==========================================================================
 * To use a linked list,
 *
 * The MAX size is 250 - if you need a list bigger, this list isn't the right thing to use anyway.
 *
 * list should point to a uint8_t array the size of the list
 * available should point to a uint8_t array of size LINKED_LIST_AVAILABLE_ARRAY_SIZE(x)
 *    where x = the number of elements in the list
 *========================================================================*/
typedef struct J1939List_S
{
   j1939_list_list_t *list;         /* Pointer to the list array */
   j1939_list_element_t *available; /* Pointer to available elements - a 1 means available */
   j1939_list_element_t head;       /* The first element in the list */
   j1939_list_size_t size;          /* Number of elements in the list */
} j1939_list_t;

#define LINKED_LIST_NO_ELEMENT ((j1939_list_element_t)-1)
#define LINKED_LIST_BITS_PER_BYTE (8)

/* x = number of elements in list */
#define LINKED_LIST_AVAILABLE_ARRAY_SIZE(x)                                                        \
   ((j1939_counter_t)((((x)-1) / LINKED_LIST_BITS_PER_BYTE) + 1))

/// @brief Initialize a list
/// @param list List to initialize
/// @param size Number of elements in the list
void j1939_list_init(j1939_list_t *list, j1939_list_size_t size);

/// @brief Search the items in the list for an unused element, mark as used and return the value.
/// @param list List to search
/// @return Index of the new element
j1939_list_element_t j1939_list_get_new_element(j1939_list_t *list);

/// @brief Insert a new element after old element
/// @param list List to modify
/// @param newElement Element to add to list
/// @param oldElement Element to add new element after
void j1939_list_insert_after(j1939_list_t *list, j1939_list_element_t newElement,
                           j1939_list_element_t oldElement);

/// @brief Release an element from the list
/// @param list List to modify
/// @param element Item to release
void j1939_list_free_element(j1939_list_t *list, j1939_list_element_t element);

//lint -esym(14, j1939_list_get_head_element) inline functions cause previously defined warnings
//lint -esym(695, j1939_list_get_head_element) lint wants extern before inline, but compiler doesn't like it
inline j1939_list_element_t j1939_list_get_head_element(const j1939_list_t *list)
{
   __ASSERT_NO_MSG(list != NULL);
   return list->head;
}

//lint -esym(14, j1939_list_get_next_element) inline functions cause previously defined warnings
//lint -esym(695, j1939_list_get_next_element) lint wants extern before inline, but compiler doesn't like it
inline j1939_list_element_t j1939_list_get_next_element(const j1939_list_t *list,
                                                    j1939_list_element_t element)
{
   __ASSERT_NO_MSG(list != NULL);
   return list->list[element];
}

#endif
