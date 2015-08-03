/* netq.h -- Simple packet queue
 *
 * Copyright (C) 2010--2012 Olaf Bergmann <bergmann@tzi.org>
 *
 * This file is part of the library tinyDTLS. Please see the file
 * LICENSE for terms of use.
 */

#include "dtls_config.h"
#include "debug.h"
#include "netq.h"

#ifdef HAVE_ASSERT_H
#include <assert.h>
#else
#ifndef assert
#warning "assertions are disabled"
#  define assert(x)
#endif
#endif

#include "t_list.h"

#ifndef WITH_CONTIKI
#include <stdlib.h>

static inline netq_t *
netq_malloc_node(size_t size) {
  return (netq_t *)malloc(sizeof(netq_t) + size);
}

static inline void
netq_free_node(netq_t *node) {
  free(node);
}

/* FIXME: implement Contiki's list functions using utlist.h */

#else /* WITH_CONTIKI */
#include "memb.h"

MEMB(netq_storage, netq_t, NETQ_MAXCNT);

static inline netq_t *
netq_malloc_node(size_t size) {
  return (netq_t *)memb_alloc(&netq_storage);
}

static inline void
netq_free_node(netq_t *node) {
  memb_free(&netq_storage, node);
}

void
netq_init() {
  memb_init(&netq_storage);
}
#endif /* WITH_CONTIKI */

int 
netq_insert_node(list_t queue, netq_t *node) {
  netq_t *p;

  assert(queue);
  assert(node);

  p = (netq_t *)list_head(queue);
  while(p && p->t <= node->t && list_item_next(p))
    p = list_item_next(p);

  if (p)
    list_insert(queue, p, node);
  else
    list_push(queue, node);

  return 1;
}

netq_t *
netq_head(list_t queue) {
  if (!queue)
    return NULL;

  return list_head(queue);
}

netq_t *
netq_next(netq_t *p) {
  if (!p)
    return NULL;

  return list_item_next(p);
}

void
netq_remove(list_t queue, netq_t *p) {
  if (!queue || !p)
    return;

  list_remove(queue, p);
}

netq_t *netq_pop_first(list_t queue) {
  if (!queue)
    return NULL;

  return list_pop(queue);
}

netq_t *
netq_node_new(size_t size) {
  netq_t *node;
  node = netq_malloc_node(size);

#ifndef NDEBUG
  if (!node)
    dtls_warn("netq_node_new: malloc\n");
#endif

  if (node)
    memset(node, 0, sizeof(netq_t));

  return node;  
}

void 
netq_node_free(netq_t *node) {
  if (node)
    netq_free_node(node);
}

void 
netq_delete_all(list_t queue) {
  netq_t *p;
  if (queue) {
    while((p = list_pop(queue)))
      netq_free_node(p); 
  }
}

