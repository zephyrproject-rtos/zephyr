/*
 * Copyright (c) 2013, Swedish Institute of Computer Science
 * Copyright (c) 2010, Vrije Universiteit Brussel
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
 *
 * Authors: Simon Duquennoy <simonduq@sics.se>
 *          Joris Borms <joris.borms@vub.ac.be>
 */

#include "contiki.h"

#include <stddef.h>
#include <string.h>
#include "lib/memb.h"
#include "lib/list.h"
#include "net/nbr-table.h"

/* List of link-layer addresses of the neighbors, used as key in the tables */
typedef struct nbr_table_key {
  struct nbr_table_key *next;
  linkaddr_t lladdr;
} nbr_table_key_t;

/* For each neighbor, a map of the tables that use the neighbor.
 * As we are using uint8_t, we have a maximum of 8 tables in the system */
static uint8_t used_map[NBR_TABLE_MAX_NEIGHBORS];
/* For each neighbor, a map of the tables that lock the neighbor */
static uint8_t locked_map[NBR_TABLE_MAX_NEIGHBORS];
/* The maximum number of tables */
#define MAX_NUM_TABLES 8
/* A list of pointers to tables in use */
static struct nbr_table *all_tables[MAX_NUM_TABLES];
/* The current number of tables */
static unsigned num_tables;

/* The neighbor address table */
MEMB(neighbor_addr_mem, nbr_table_key_t, NBR_TABLE_MAX_NEIGHBORS);
LIST(nbr_table_keys);

/*---------------------------------------------------------------------------*/
/* Get a key from a neighbor index */
static nbr_table_key_t *
key_from_index(int index)
{
  return index != -1 ? &((nbr_table_key_t *)neighbor_addr_mem.mem)[index] : NULL;
}
/*---------------------------------------------------------------------------*/
/* Get an item from its neighbor index */
static nbr_table_item_t *
item_from_index(nbr_table_t *table, int index)
{
  return table != NULL && index != -1 ? (char *)table->data + index * table->item_size : NULL;
}
/*---------------------------------------------------------------------------*/
/* Get the neighbor index of an item */
static int
index_from_key(nbr_table_key_t *key)
{
  return key != NULL ? key - (nbr_table_key_t *)neighbor_addr_mem.mem : -1;
}
/*---------------------------------------------------------------------------*/
/* Get the neighbor index of an item */
static int
index_from_item(nbr_table_t *table, const nbr_table_item_t *item)
{
  return table != NULL && item != NULL ? ((int)((char *)item - (char *)table->data)) / table->item_size : -1;
}
/*---------------------------------------------------------------------------*/
/* Get an item from its key */
static nbr_table_item_t *
item_from_key(nbr_table_t *table, nbr_table_key_t *key)
{
  return item_from_index(table, index_from_key(key));
}
/*---------------------------------------------------------------------------*/
/* Get the key af an item */
static nbr_table_key_t *
key_from_item(nbr_table_t *table, const nbr_table_item_t *item)
{
  return key_from_index(index_from_item(table, item));
}
/*---------------------------------------------------------------------------*/
/* Get the index of a neighbor from its link-layer address */
static int
index_from_lladdr(const linkaddr_t *lladdr)
{
  nbr_table_key_t *key;
  /* Allow lladdr-free insertion, useful e.g. for IPv6 ND.
   * Only one such entry is possible at a time, indexed by linkaddr_null. */
  if(lladdr == NULL) {
    lladdr = &linkaddr_null;
  }
  key = list_head(nbr_table_keys);
  while(key != NULL) {
    if(lladdr && linkaddr_cmp(lladdr, &key->lladdr)) {
      return index_from_key(key);
    }
    key = list_item_next(key);
  }
  return -1;
}
/*---------------------------------------------------------------------------*/
/* Get bit from "used" or "locked" bitmap */
static int
nbr_get_bit(uint8_t *bitmap, nbr_table_t *table, nbr_table_item_t *item)
{
  int item_index = index_from_item(table, item);
  if(table != NULL && item_index != -1) {
    return (bitmap[item_index] & (1 << table->index)) != 0;
  } else {
    return 0;
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
/* Set bit in "used" or "locked" bitmap */
static int
nbr_set_bit(uint8_t *bitmap, nbr_table_t *table, nbr_table_item_t *item, int value)
{
  int item_index = index_from_item(table, item);
  if(table != NULL && item_index != -1) {
    if(value) {
      bitmap[item_index] |= 1 << table->index;
    } else {
      bitmap[item_index] &= ~(1 << table->index);
    }
    return 1;
  } else {
    return 0;
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
static nbr_table_key_t *
nbr_table_allocate(void)
{
  nbr_table_key_t *key;
  int least_used_count = 0;
  nbr_table_key_t *least_used_key = NULL;

  key = memb_alloc(&neighbor_addr_mem);
  if(key != NULL) {
    return key;
  } else { /* No more space, try to free a neighbor.
            * The replacement policy is the following: remove neighbor that is:
            * (1) not locked
            * (2) used by fewest tables
            * (3) oldest (the list is ordered by insertion time)
            * */
    /* Get item from first key */
    key = list_head(nbr_table_keys);
    while(key != NULL) {
      int item_index = index_from_key(key);
      int locked = locked_map[item_index];
      /* Never delete a locked item */
      if(!locked) {
        int used = used_map[item_index];
        int used_count = 0;
        /* Count how many tables are using this item */
        while(used != 0) {
          if((used & 1) == 1) {
            used_count++;
          }
          used >>= 1;
        }
        /* Find least used item */
        if(least_used_key == NULL || used_count < least_used_count) {
          least_used_key = key;
          least_used_count = used_count;
          if(used_count == 0) { /* We won't find any least used item */
            break;
          }
        }
      }
      key = list_item_next(key);
    }
    if(least_used_key == NULL) {
      /* We haven't found any unlocked item, allocation fails */
      return NULL;
    } else {
      /* Reuse least used item */
      int i;
      for(i = 0; i<MAX_NUM_TABLES; i++) {
        if(all_tables[i] != NULL && all_tables[i]->callback != NULL) {
          /* Call table callback for each table that uses this item */
          nbr_table_item_t *removed_item = item_from_key(all_tables[i], least_used_key);
          if(nbr_get_bit(used_map, all_tables[i], removed_item) == 1) {
            all_tables[i]->callback(removed_item);
          }
        }
      }
      /* Empty used map */
      used_map[index_from_key(least_used_key)] = 0;
      /* Remove neighbor from list */
      list_remove(nbr_table_keys, least_used_key);
      /* Return associated key */
      return least_used_key;
    }
  }
}
/*---------------------------------------------------------------------------*/
/* Register a new neighbor table. To be used at initialization by modules
 * using a neighbor table */
int
nbr_table_register(nbr_table_t *table, nbr_table_callback *callback)
{
  if(num_tables < MAX_NUM_TABLES) {
    table->index = num_tables++;
    table->callback = callback;
    all_tables[table->index] = table;
    return 1;
  } else {
    /* Maximum number of tables exceeded */
    return 0;
  }
}
/*---------------------------------------------------------------------------*/
/* Returns the first item of the current table */
nbr_table_item_t *
nbr_table_head(nbr_table_t *table)
{
  /* Get item from first key */
  nbr_table_item_t *item = item_from_key(table, list_head(nbr_table_keys));
  /* Item is the first neighbor, now check is it is in the current table */
  if(nbr_get_bit(used_map, table, item)) {
    return item;
  } else {
    return nbr_table_next(table, item);
  }
}
/*---------------------------------------------------------------------------*/
/* Iterates over the current table */
nbr_table_item_t *
nbr_table_next(nbr_table_t *table, nbr_table_item_t *item)
{
  do {
    void *key = key_from_item(table, item);
    key = list_item_next(key);
    /* Loop until the next item is in the current table */
    item = item_from_key(table, key);
  } while(item && !nbr_get_bit(used_map, table, item));
  return item;
}
/*---------------------------------------------------------------------------*/
/* Add a neighbor indexed with its link-layer address */
nbr_table_item_t *
nbr_table_add_lladdr(nbr_table_t *table, const linkaddr_t *lladdr)
{
  int index;
  nbr_table_item_t *item;
  nbr_table_key_t *key;

  /* Allow lladdr-free insertion, useful e.g. for IPv6 ND.
   * Only one such entry is possible at a time, indexed by linkaddr_null. */
  if(lladdr == NULL) {
    lladdr = &linkaddr_null;
  }

  if((index = index_from_lladdr(lladdr)) == -1) {
     /* Neighbor not yet in table, let's try to allocate one */
    key = nbr_table_allocate();

    /* No space available for new entry */
    if(key == NULL) {
      return NULL;
    }

    /* Add neighbor to list */
    list_add(nbr_table_keys, key);

    /* Get index from newly allocated neighbor */
    index = index_from_key(key);

    /* Set link-layer address */
    linkaddr_copy(&key->lladdr, lladdr);
  }

  /* Get item in the current table */
  item = item_from_index(table, index);

  /* Initialize item data and set "used" bit */
  memset(item, 0, table->item_size);
  nbr_set_bit(used_map, table, item, 1);

  return item;
}
/*---------------------------------------------------------------------------*/
/* Get an item from its link-layer address */
void *
nbr_table_get_from_lladdr(nbr_table_t *table, const linkaddr_t *lladdr)
{
  void *item = item_from_index(table, index_from_lladdr(lladdr));
  return nbr_get_bit(used_map, table, item) ? item : NULL;
}
/*---------------------------------------------------------------------------*/
/* Removes a neighbor from the current table (unset "used" bit) */
int
nbr_table_remove(nbr_table_t *table, void *item)
{
  int ret = nbr_set_bit(used_map, table, item, 0);
  nbr_set_bit(locked_map, table, item, 0);
  return ret;
}
/*---------------------------------------------------------------------------*/
/* Lock a neighbor for the current table (set "locked" bit) */
int
nbr_table_lock(nbr_table_t *table, void *item)
{
  return nbr_set_bit(locked_map, table, item, 1);
}
/*---------------------------------------------------------------------------*/
/* Release the lock on a neighbor for the current table (unset "locked" bit) */
int
nbr_table_unlock(nbr_table_t *table, void *item)
{
  return nbr_set_bit(locked_map, table, item, 0);
}
/*---------------------------------------------------------------------------*/
/* Get link-layer address of an item */
linkaddr_t *
nbr_table_get_lladdr(nbr_table_t *table, const void *item)
{
  nbr_table_key_t *key = key_from_item(table, item);
  return key != NULL ? &key->lladdr : NULL;
}
