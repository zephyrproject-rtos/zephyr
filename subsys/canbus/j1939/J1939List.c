/*
 * Copyright (c) 2026 Caleb Perkinson
 * SPDX-License-Identifier: Apache-2.0
 */
#include "J1939List.h"
#include <zephyr/canbus/j1939.h>

extern inline J1939List_Element_T J1939List_GetHeadElement(const J1939List_T *klist);
extern inline J1939List_Element_T J1939List_GetNextElement(const J1939List_T *klist,
                                                           J1939List_Element_T element);

#ifndef _C166
/// @brief Implements teh `_prior` instruction of the C167 micros.
/// @param wWord 16-bit value to shift
/// @return Number of shifts occurred
static inline J1939_Counter_T _prior(unsigned int wWord)
{
   J1939_Counter_T byCount = 0;

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
static inline void J1939List_SetAvailableBit(uint16_t index, J1939List_List_T *list)
{
   uint16_t computedIndex = index / LINKED_LIST_BITS_PER_BYTE;

   __ASSERT_NO_MSG(list != NULL);

   list[computedIndex] =
       (J1939List_List_T)(list[computedIndex] |
                          (J1939List_List_T)(1 << (index % LINKED_LIST_BITS_PER_BYTE)));
}

/**************************************************************************************************/
static inline void J1939List_ClearAvailableBit(uint16_t index, J1939List_List_T *list)
{
   __ASSERT_NO_MSG(list != NULL);

   list[index / LINKED_LIST_BITS_PER_BYTE] &=
       (J1939List_Element_T) ~(J1939List_Element_T)(1 << (index % LINKED_LIST_BITS_PER_BYTE));
}

/**************************************************************************************************/
static inline J1939List_Element_T J1939List_GetBit(uint16_t wWord)
{
   return (J1939List_Element_T)(15 - _prior((unsigned int)wWord));
}

/**************************************************************************************************/
void J1939List_Init(J1939List_T *list, J1939List_Size_T size)
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
J1939List_Element_T J1939List_GetNewElement(J1939List_T *list)
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
            result = J1939List_GetBit(list->available[index]);

            /* Clear the bit in the available bit array */
            J1939List_ClearAvailableBit(result, &list->available[index]);

            /* byReturn is the bit position from the current word - get the bit position from
               the beginning of the array */
            result += (uint16_t)(index * LINKED_LIST_BITS_PER_BYTE);
            break;
         }
      }
   }

   return (J1939List_Element_T)result;
   //lint -e818 Suppress const warning.
}

/**************************************************************************************************/
void J1939List_InsertAfter(J1939List_T *list, J1939List_Element_T newElement,
                           J1939List_Element_T oldElement)
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
void J1939List_FreeElement(J1939List_T *list, J1939List_Element_T element)
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

         J1939List_SetAvailableBit(element, list->available);
         list->list[element] = LINKED_LIST_NO_ELEMENT;
      }
   }
}
