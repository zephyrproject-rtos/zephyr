/* t_list -- tinydtls lists
 *
 * Copyright (C) 2012 Olaf Bergmann <bergmann@tzi.org>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/**
 * @file t_list.h
 * @brief Wrappers for list structures and functions
 */

#ifndef _DTLS_LIST_H_
#define _DTLS_LIST_H_

#include "tinydtls.h"

#ifndef WITH_CONTIKI

/* We define list structures and utility functions to be compatible
 * with Contiki list structures. The Contiki list API is part of the
 * Contiki operating system, and therefore the following licensing
 * terms apply (taken from contiki/core/lib/list.h):
 *
 * Copyright (c) 2004, Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 * Author: Adam Dunkels <adam@sics.se>
 *
 * $ Id: list.h,v 1.5 2010/09/13 13:31:00 adamdunkels Exp $
 */

typedef void **list_t;
struct list {
  struct list *next;
};

#define LIST_CONCAT(s1, s2) s1##s2

#define LIST_STRUCT(name)			\
  void *LIST_CONCAT(name, _list);		\
  list_t name

#define LIST_STRUCT_INIT(struct_ptr, name)  {				\
    (struct_ptr)->name = &((struct_ptr)->LIST_CONCAT(name,_list));	\
    (struct_ptr)->LIST_CONCAT(name,_list) = NULL;			\
  }

static inline void *
list_head(list_t list) {
  return *list;
}

static inline void
list_remove(list_t list, void *item) {
  struct list *l, *r;

  if(*list == NULL) {
    return;
  }

  r = NULL;
  for(l = *list; l != NULL; l = l->next) {
    if(l == item) {
      if(r == NULL) {
	/* First on list */
	*list = l->next;
      } else {
	/* Not first on list */
	r->next = l->next;
      }
      l->next = NULL;
      return;
    }
    r = l;
  }
}

static inline void *
list_tail(list_t list)
{
  struct list *l;

  if(*list == NULL) {
    return NULL;
  }

  for(l = *list; l->next != NULL; l = l->next);

  return l;
}

static inline void
list_add(list_t list, void *item) {
  struct list *l;

  /* Make sure not to add the same element twice */
  list_remove(list, item);

  ((struct list *)item)->next = NULL;

  l = list_tail(list);

  if(l == NULL) {
    *list = item;
  } else {
    l->next = item;
  }
}

static inline void
list_push(list_t list, void *item) {
  /* Make sure not to add the same element twice */
  list_remove(list, item);

  ((struct list *)item)->next = *list;
  *list = item;
}

static inline void *
list_pop(list_t list) {
  struct list *l;
  l = *list;
  if(l)
    list_remove(list, l);
  
  return l;
}

static inline void
list_insert(list_t list, void *previtem, void *newitem) {
  if(previtem == NULL) {
    list_push(list, newitem);
  } else {
    ((struct list *)newitem)->next = ((struct list *)previtem)->next;
    ((struct list *)previtem)->next = newitem;
  } 
}

static inline void *
list_item_next(void *item)
{
  return item == NULL? NULL: ((struct list *)item)->next;
}

#else /* WITH_CONTIKI */
#include "list.h"
#endif /* WITH_CONTIKI */

#endif /* _DTLS_LIST_H_ */
