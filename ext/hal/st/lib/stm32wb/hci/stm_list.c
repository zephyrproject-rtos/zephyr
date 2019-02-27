/**
 ******************************************************************************
  * @file    stm_list.c
  * @author  MCD Application Team
  * @brief   TCircular Linked List Implementation.
  ******************************************************************************
  * @attention
 *
 * <h2><center>&copy; Copyright (c) 2018 STMicroelectronics.
 * All rights reserved.</center></h2>
 *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
 *
 ******************************************************************************
 */


/******************************************************************************
 * Include Files
 ******************************************************************************/
#include "utilities_common.h"

#include "stm_list.h"

/******************************************************************************
 * Function Definitions
 ******************************************************************************/
void LST_init_head (tListNode * listHead)
{
  listHead->next = listHead;
  listHead->prev = listHead;
}

uint8_t LST_is_empty (tListNode * listHead)
{
  uint32_t primask_bit;
  uint8_t return_value;

  primask_bit = __get_PRIMASK();  /**< backup PRIMASK bit */
  __disable_irq();                  /**< Disable all interrupts by setting PRIMASK bit on Cortex*/
  if(listHead->next == listHead)
  {
    return_value = TRUE;
  }
  else
  {
    return_value = FALSE;
  }
  __set_PRIMASK(primask_bit);     /**< Restore PRIMASK bit*/

  return return_value;
}

void LST_insert_head (tListNode * listHead, tListNode * node)
{
  uint32_t primask_bit;

  primask_bit = __get_PRIMASK();  /**< backup PRIMASK bit */
  __disable_irq();                  /**< Disable all interrupts by setting PRIMASK bit on Cortex*/

  node->next = listHead->next;
  node->prev = listHead;
  listHead->next = node;
  (node->next)->prev = node;

  __set_PRIMASK(primask_bit);     /**< Restore PRIMASK bit*/
}


void LST_insert_tail (tListNode * listHead, tListNode * node)
{
  uint32_t primask_bit;

  primask_bit = __get_PRIMASK();  /**< backup PRIMASK bit */
  __disable_irq();                  /**< Disable all interrupts by setting PRIMASK bit on Cortex*/

  node->next = listHead;
  node->prev = listHead->prev;
  listHead->prev = node;
  (node->prev)->next = node;

  __set_PRIMASK(primask_bit);     /**< Restore PRIMASK bit*/
}


void LST_remove_node (tListNode * node)
{
  uint32_t primask_bit;

  primask_bit = __get_PRIMASK();  /**< backup PRIMASK bit */
  __disable_irq();                  /**< Disable all interrupts by setting PRIMASK bit on Cortex*/

  (node->prev)->next = node->next;
  (node->next)->prev = node->prev;

  __set_PRIMASK(primask_bit);     /**< Restore PRIMASK bit*/
}


void LST_remove_head (tListNode * listHead, tListNode ** node )
{
  uint32_t primask_bit;

  primask_bit = __get_PRIMASK();  /**< backup PRIMASK bit */
  __disable_irq();                  /**< Disable all interrupts by setting PRIMASK bit on Cortex*/

  *node = listHead->next;
  LST_remove_node (listHead->next);

  __set_PRIMASK(primask_bit);     /**< Restore PRIMASK bit*/
}


void LST_remove_tail (tListNode * listHead, tListNode ** node )
{
  uint32_t primask_bit;

  primask_bit = __get_PRIMASK();  /**< backup PRIMASK bit */
  __disable_irq();                  /**< Disable all interrupts by setting PRIMASK bit on Cortex*/

  *node = listHead->prev;
  LST_remove_node (listHead->prev);

  __set_PRIMASK(primask_bit);     /**< Restore PRIMASK bit*/
}


void LST_insert_node_after (tListNode * node, tListNode * ref_node)
{
  uint32_t primask_bit;

  primask_bit = __get_PRIMASK();  /**< backup PRIMASK bit */
  __disable_irq();                  /**< Disable all interrupts by setting PRIMASK bit on Cortex*/

  node->next = ref_node->next;
  node->prev = ref_node;
  ref_node->next = node;
  (node->next)->prev = node;

  __set_PRIMASK(primask_bit);     /**< Restore PRIMASK bit*/
}


void LST_insert_node_before (tListNode * node, tListNode * ref_node)
{
  uint32_t primask_bit;

  primask_bit = __get_PRIMASK();  /**< backup PRIMASK bit */
  __disable_irq();                  /**< Disable all interrupts by setting PRIMASK bit on Cortex*/

  node->next = ref_node;
  node->prev = ref_node->prev;
  ref_node->prev = node;
  (node->prev)->next = node;

  __set_PRIMASK(primask_bit);     /**< Restore PRIMASK bit*/
}


int LST_get_size (tListNode * listHead)
{
  int size = 0;
  tListNode * temp;
  uint32_t primask_bit;

  primask_bit = __get_PRIMASK();  /**< backup PRIMASK bit */
  __disable_irq();                  /**< Disable all interrupts by setting PRIMASK bit on Cortex*/

  temp = listHead->next;
  while (temp != listHead)
  {
    size++;
    temp = temp->next;
  }

  __set_PRIMASK(primask_bit);     /**< Restore PRIMASK bit*/

  return (size);
}

void LST_get_next_node (tListNode * ref_node, tListNode ** node)
{
  uint32_t primask_bit;

  primask_bit = __get_PRIMASK();  /**< backup PRIMASK bit */
  __disable_irq();                  /**< Disable all interrupts by setting PRIMASK bit on Cortex*/

  *node = ref_node->next;

  __set_PRIMASK(primask_bit);     /**< Restore PRIMASK bit*/
}


void LST_get_prev_node (tListNode * ref_node, tListNode ** node)
{
  uint32_t primask_bit;

  primask_bit = __get_PRIMASK();  /**< backup PRIMASK bit */
  __disable_irq();                  /**< Disable all interrupts by setting PRIMASK bit on Cortex*/

  *node = ref_node->prev;

  __set_PRIMASK(primask_bit);      /**< Restore PRIMASK bit*/
}
