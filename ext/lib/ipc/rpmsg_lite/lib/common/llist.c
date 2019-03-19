/*
 * Copyright (c) 2014, Mentor Graphics Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of Mentor Graphics Corporation nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/**************************************************************************
 * FILE NAME
 *
 *       llist.c
 *
 * COMPONENT
 *
 *         OpenAMP stack.
 *
 * DESCRIPTION
 *
 *       Source file for basic linked list service.
 *
 **************************************************************************/
#include "llist.h"

#define LIST_NULL ((void *)0)
/*!
 * add_to_list
 *
 * Places new element at the start of the list.
 *
 * @param head - list head
 * @param node - new element to add
 *
 */
void add_to_list(struct llist **head, struct llist *node)
{
    if (node == LIST_NULL)
    {
        return;
    }

    if (*head != LIST_NULL)
    {
        /* Place the new element at the start of list. */
        node->next = *head;
        node->prev = LIST_NULL;
        (*head)->prev = node;
        *head = node;
    }
    else
    {
        /* List is empty - assign new element to list head. */
        *head = node;
        (*head)->next = LIST_NULL;
        (*head)->prev = LIST_NULL;
    }
}

/*!
 * remove_from_list
 *
 * Removes the given element from the list.
 *
 * @param head    - list head
 * @param element - element to remove from list
 *
 */
void remove_from_list(struct llist **head, struct llist *node)
{
    if ((*head == LIST_NULL) || (node == LIST_NULL))
    {
        return;
    }

    if (node == *head)
    {
        /* First element has to be removed. */
        *head = (*head)->next;
    }
    else if (node->next == LIST_NULL)
    {
        /* Last element has to be removed. */
        node->prev->next = node->next;
    }
    else
    {
        /* Intermediate element has to be removed. */
        node->prev->next = node->next;
        node->next->prev = node->prev;
    }
}
