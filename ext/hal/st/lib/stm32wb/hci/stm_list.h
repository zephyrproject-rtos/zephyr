/**
  ******************************************************************************
  * @file    stm_list.h
  * @author  MCD Application Team
  * @brief   Header file for linked list library.
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


#ifndef _STM_LIST_H_
#define _STM_LIST_H_

/* Includes ------------------------------------------------------------------*/

typedef  struct _tListNode {
    struct _tListNode * next;
    struct _tListNode * prev;
} tListNode;

void LST_init_head (tListNode * listHead);

uint8_t LST_is_empty (tListNode * listHead);

void LST_insert_head (tListNode * listHead, tListNode * node);

void LST_insert_tail (tListNode * listHead, tListNode * node);

void LST_remove_node (tListNode * node);

void LST_remove_head (tListNode * listHead, tListNode ** node );

void LST_remove_tail (tListNode * listHead, tListNode ** node );

void LST_insert_node_after (tListNode * node, tListNode * ref_node);

void LST_insert_node_before (tListNode * node, tListNode * ref_node);

int LST_get_size (tListNode * listHead);

void LST_get_next_node (tListNode * ref_node, tListNode ** node);

void LST_get_prev_node (tListNode * ref_node, tListNode ** node);

#endif /* _STM_LIST_H_ */
