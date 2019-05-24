/*
 * Copyright (c) 2015, 2017 Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 *  ======== List.c ========
 */
#include <ti/drivers/dpl/HwiP.h>
#include <ti/drivers/utils/List.h>

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

/*
 *  ======== List_clearList ========
 */
void List_clearList(List_List *list)
{
    uintptr_t key;

    key = HwiP_disable();

    list->head = list->tail = NULL;

    HwiP_restore(key);
}



/*
 *  ======== List_get ========
 */
List_Elem *List_get(List_List *list)
{
    List_Elem *elem;
    uintptr_t key;

    key = HwiP_disable();

    elem = list->head;

    /* See if the List was empty */
    if (elem != NULL) {
        list->head = elem->next;
        if (elem->next != NULL) {
            elem->next->prev = NULL;
        }
        else {
            list->tail = NULL;
        }
    }

    HwiP_restore(key);

    return (elem);
}


/*
 *  ======== List_insert ========
 */
void List_insert(List_List *list, List_Elem *newElem, List_Elem *curElem)
{
    uintptr_t key;

    key = HwiP_disable();

    newElem->next = curElem;
    newElem->prev = curElem->prev;
    if (curElem->prev != NULL) {
        curElem->prev->next = newElem;
    }
    else {
        list->head = newElem;
    }
    curElem->prev = newElem;

    HwiP_restore(key);
}


/*
 *  ======== List_put ========
 */
void List_put(List_List *list, List_Elem *elem)
{
    uintptr_t key;

    key = HwiP_disable();

    elem->next = NULL;
    elem->prev = list->tail;
    if (list->tail != NULL) {
        list->tail->next = elem;
    }
    else {
        list->head = elem;
    }

    list->tail = elem;

    HwiP_restore(key);
}

/*
 *  ======== List_putHead ========
 */
void List_putHead(List_List *list, List_Elem *elem)
{
    uintptr_t key;

    key = HwiP_disable();

    elem->next = list->head;
    elem->prev = NULL;
    if (list->head != NULL) {
        list->head->prev = elem;
    }
    else {
        list->tail = elem;
    }

    list->head = elem;

    HwiP_restore(key);
}

/*
 *  ======== List_remove ========
 */
void List_remove(List_List *list, List_Elem *elem)
{
    uintptr_t key;

    key = HwiP_disable();

    /* Handle the case where the elem to remove is the last one */
    if (elem->next == NULL) {
        list->tail = elem->prev;
    }
    else {
        elem->next->prev = elem->prev;
    }

    /* Handle the case where the elem to remove is the first one */
    if (elem->prev == NULL) {
        list->head = elem->next;
    }
    else {
        elem->prev->next = elem->next;
    }

    HwiP_restore(key);
}
