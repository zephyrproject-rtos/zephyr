/*
 * Copyright (c) 2026 Deere & Company
 * SPDX-License-Identifier: Apache-2.0
 */
#include "J1939List.h"
#include <zephyr/canbus/j1939.h>

extern inline j1939_list_element_t j1939_list_get_head_element(const j1939_list_t *klist);
extern inline j1939_list_element_t j1939_list_get_next_element(const j1939_list_t *klist,
                                                           j1939_list_element_t element);

#ifndef _C166
/// @brief Implements teh `_prior` instruction of the C167 micros.
/// @param wWord 16-bit value to shift
/// @return Number of shifts occurred
static inline j1939_counter_t _prior(unsigned int wWord)
{
   j1939_counter_t byCount = 0;

   // This implements the _prior instruction available on the C16x micros.
   if (wWord != 0)
   {
      while (((wWord & 0x8000) != 0x8000) && (byCount != 15))
      {
         wWord = wWord << 1;
         byCount++;
      }
   }

   return byCount;
}
#else
unsigned int _prior(unsigned int);
#endif

/**************************************************************************************************/
static inline void j1939_list_set_available_bit(uint16_t index, j1939_list_list_t *list)
{
   uint16_t computedIndex = index / LINKED_LIST_BITS_PER_BYTE;

   __ASSERT_NO_MSG(list != NULL);

   list[computedIndex] =
       (j1939_list_list_t)(list[computedIndex] |
                          (j1939_list_list_t)(1 << (index % LINKED_LIST_BITS_PER_BYTE)));
}

/**************************************************************************************************/
static inline void j1939_list_clear_available_bit(uint16_t index, j1939_list_list_t *list)
{
   __ASSERT_NO_MSG(list != NULL);

   list[index / LINKED_LIST_BITS_PER_BYTE] &=
       (j1939_list_element_t) ~(j1939_list_element_t)(1 << (index % LINKED_LIST_BITS_PER_BYTE));
}

/**************************************************************************************************/
static inline j1939_list_element_t j1939_list_get_bit(uint16_t wWord)
{
   return (j1939_list_element_t)(15 - _prior((unsigned int)wWord));
}

/**************************************************************************************************/
void j1939_list_init(j1939_list_t *list, j1939_list_size_t size)
{
   uint16_t index;

   if (list)
   {
      for (index = 0; index < size; index++)
      {
         list->list[index] = LINKED_LIST_NO_ELEMENT;
      }

      /* Set the available bits to 1 */
      for (index = 0; index < LINKED_LIST_AVAILABLE_ARRAY_SIZE(size); index++)
      {
         list->available[index] = LINKED_LIST_NO_ELEMENT;
      }

      /* If the list size is not a multiple of 8, set the "right" number of bits in the last byte
       */
      if ((size % LINKED_LIST_BITS_PER_BYTE) != 0)
      {
         list->available[index - 1] >>=
             (LINKED_LIST_BITS_PER_BYTE - (size % LINKED_LIST_BITS_PER_BYTE));
      }

      list->size = size;
      list->head = LINKED_LIST_NO_ELEMENT;
   }
}

/**************************************************************************************************/
j1939_list_element_t j1939_list_get_new_element(j1939_list_t *list)
{
   uint16_t index;
   uint16_t result = (uint16_t)-1; /* If there's none available, return -1 */

   if (list && list->available)
   {
      for (index = 0; index < LINKED_LIST_AVAILABLE_ARRAY_SIZE(list->size); index++)
      {
         if (list->available[index] != 0)
         {
            /* Get an available bit */
            result = j1939_list_get_bit(list->available[index]);

            /* Clear the bit in the available bit array */
            j1939_list_clear_available_bit(result, &list->available[index]);

            /* byReturn is the bit position from the current word - get the bit position from
               the beginning of the array */
            result += (uint16_t)(index * LINKED_LIST_BITS_PER_BYTE);
            break;
         }
      }
   }

   return (j1939_list_element_t)result;
   //lint -e818 Suppress const warning.
}

/**************************************************************************************************/
void j1939_list_insert_after(j1939_list_t *list, j1939_list_element_t newElement,
                           j1939_list_element_t oldElement)
{
   if (list)
   {
      if (oldElement == LINKED_LIST_NO_ELEMENT) /* Insert at the first thing in the list */
      {
         list->list[newElement] = list->head;
         list->head = newElement;
      }
      else
      {
         list->list[newElement] = list->list[oldElement];
         list->list[oldElement] = newElement;
      }
   }
}

/**************************************************************************************************/
void j1939_list_free_element(j1939_list_t *list, j1939_list_element_t element)
{
   uint16_t index;

   if (list)
   {
      if (element < list->size)
      {
         /* Find the element in the list */
         if (list->head == element)
         {
            /* Link the next object to the previous object */
            list->head = list->list[element];
         }
         else
         {
            for (index = list->head;                          /* Init to after the head */
                 list->list[index] != LINKED_LIST_NO_ELEMENT; /* While this isn't the end */
                 index = list->list[index])                   /* Traverse the list */
            {
               if (list->list[index] == element)
               {
                  /* Link the next object to the previous object */
                  list->list[index] = list->list[element];
                  break;
               }
            }
         }

         j1939_list_set_available_bit(element, list->available);
         list->list[element] = LINKED_LIST_NO_ELEMENT;
      }
   }
}
