/*
 * Copyright (c) 2026 Deere & Company
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _LINKED_LIST_H_
#define _LINKED_LIST_H_

#include <stddef.h>
#include <stdint.h>
#include <zephyr/sys/util.h>

typedef uint8_t J1939List_List_T;

typedef uint8_t J1939List_Size_T;

typedef uint8_t J1939List_Element_T;

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
   J1939List_List_T *list;         /* Pointer to the list array */
   J1939List_Element_T *available; /* Pointer to available elements - a 1 means available */
   J1939List_Element_T head;       /* The first element in the list */
   J1939List_Size_T size;          /* Number of elements in the list */
} J1939List_T;

#define LINKED_LIST_NO_ELEMENT ((J1939List_Element_T)-1)
#define LINKED_LIST_BITS_PER_BYTE (8)

/* x = number of elements in list */
#define LINKED_LIST_AVAILABLE_ARRAY_SIZE(x)                                                        \
   ((J1939_Counter_T)((((x)-1) / LINKED_LIST_BITS_PER_BYTE) + 1))

/// @brief Initialize a list
/// @param list List to initialize
/// @param size Number of elements in the list
void J1939List_Init(J1939List_T *list, J1939List_Size_T size);

/// @brief Search the items in the list for an unused element, mark as used and return the value.
/// @param list List to search
/// @return Index of the new element
J1939List_Element_T J1939List_GetNewElement(J1939List_T *list);

/// @brief Insert a new element after old element
/// @param list List to modify
/// @param newElement Element to add to list
/// @param oldElement Element to add new element after
void J1939List_InsertAfter(J1939List_T *list, J1939List_Element_T newElement,
                           J1939List_Element_T oldElement);

/// @brief Release an element from the list
/// @param list List to modify
/// @param element Item to release
void J1939List_FreeElement(J1939List_T *list, J1939List_Element_T element);

//lint -esym(14, J1939List_GetHeadElement) inline functions cause previously defined warnings
//lint -esym(695, J1939List_GetHeadElement) lint wants extern before inline, but compiler doesn't like it
inline J1939List_Element_T J1939List_GetHeadElement(const J1939List_T *list)
{
   __ASSERT_NO_MSG(list != NULL);
   return list->head;
}

//lint -esym(14, J1939List_GetNextElement) inline functions cause previously defined warnings
//lint -esym(695, J1939List_GetNextElement) lint wants extern before inline, but compiler doesn't like it
inline J1939List_Element_T J1939List_GetNextElement(const J1939List_T *list,
                                                    J1939List_Element_T element)
{
   __ASSERT_NO_MSG(list != NULL);
   return list->list[element];
}

#endif
