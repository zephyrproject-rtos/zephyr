/** @file routing_table.c
 *  @brief Reverse and forward tables constructed by AODV.
 *
 *  @bug No known bugs.
 */

/* -- Includes -- */
#include <zephyr.h>
#include <misc/slist.h>
#include <string.h>
#include "routing_table.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_MESH_DEBUG_ROUTING)
#include "common/log.h"

/*DEFINITIONS*/
#define NUMBER_OF_ENTRIES 20    /* Maximum number of entries in the table */
#define ALLIGNED 4              /* Memory Allignment */
#define ALLOCATION_INTERVAL 100 /* maximum time taken to allocate from slab */
#define ENTRY_SIZE sizeof(struct bt_mesh_route_entry)
#define LIFETIME  K_SECONDS(12)
#define RREQ_INTERVAL_WAIT K_MSEC(3000)


/** @brief Linked list holding pointers to the valid entries of the routing tables. */
sys_slist_t valid_list;
/** @brief Linked list holding pointers to the invalid entries of the routing tables. */
sys_slist_t invalid_list;
/** @brief Linked list holding pointers to the invalid entries of the routing tables after link drop. */
sys_slist_t invalid_rerr_list;

K_SEM_DEFINE(valid_list_sem, 1, 1); 	/* Binary semaphore for valid list */
K_SEM_DEFINE(invalid_list_sem, 1, 1); /* Binary semaphore for invalid list */
K_SEM_DEFINE(invalid_rerr_list_sem, 1, 1); /* Binary semaphore for invalid rerr list */
struct k_mem_slab routing_table_slab; /* Memory slab for all entries */
K_MEM_SLAB_DEFINE(routing_table_slab, ENTRY_SIZE, NUMBER_OF_ENTRIES, ALLIGNED);

/* FUNCTIONS */

/** @brief Initializing the valid and invalid lists */
void bt_mesh_routing_table_init()
{
	sys_slist_init(&valid_list);
	sys_slist_init(&invalid_list);
	sys_slist_init(&invalid_rerr_list);
}

/* Create Entry Functions */

/**
 *	@brief Create entry in the valid list.
 *
 *	@param entry_data: Pointer to pointer to structure of type bt_mesh_route_entry
 *										 holding data to be stored.
 *
 *	@return True when allocation succeeds, False when no space is available.
 */
 bool bt_mesh_create_entry_valid(struct bt_mesh_route_entry **entry_location)
 {
 	/* if space found in slab, Allocate new node */
 	if (k_mem_slab_alloc(&routing_table_slab, (void **)&(*entry_location), ALLOCATION_INTERVAL) == 0) {
 		memset((*entry_location), 0, ENTRY_SIZE);                  /* Initializing with zeros */
 		k_sem_take(&valid_list_sem, K_FOREVER);                 /*take semaphore */
 		sys_slist_append(&valid_list, &(*entry_location)->node);   /*insert node in linkedlist */
 		k_sem_give(&valid_list_sem);
 	} else    {
 		BT_ERR("Memory Allocation timeout");
 		return false;
 	}
 	/* Start the lifetime timer */
 	k_timer_init (&(*entry_location)->lifetime, bt_mesh_delete_entry_valid, NULL);
 	k_timer_start(&(*entry_location)->lifetime, LIFETIME, 0);
 	return true;
 }

/**
 *	@brief Create entry in the invalid list.
 *
 *	@param entry_data: Pointer to pointer to structure of type bt_mesh_route_entry
 *										 holding data to be stored.
 *
 *	@return True when allocation succeeds, False when no space is available.
 */
 bool bt_mesh_create_entry_invalid(struct bt_mesh_route_entry **entry_location)
 {
 	/*if space found in slab, Allocate new node */
 	if (k_mem_slab_alloc(&routing_table_slab, (void **)&(*entry_location), ALLOCATION_INTERVAL) == 0) {
 		memset((*entry_location), 0, ENTRY_SIZE);                  /* Initializing with zeros */
 		k_sem_take(&invalid_list_sem, K_FOREVER);               /*take semaphore */
 		sys_slist_append(&invalid_list, &(*entry_location)->node); /*insert node in linkedlist */
 		k_sem_give(&invalid_list_sem);
 	} else    {
 		BT_ERR("Memory Allocation timeout");
 		return false;
 	}

 	/* Start the lifetime timer */
 	k_timer_init (&(*entry_location)->lifetime, bt_mesh_delete_entry_invalid, NULL);
 	k_timer_start(&(*entry_location)->lifetime, LIFETIME, 0);
 	return true;
 }

/**
 *	@brief Create entry in the invalid rerr list.
 *
 *	@param entry_data: Pointer to pointer to structure of type bt_mesh_route_entry
 *										 holding data to be stored.
 *
 *	@return True when allocation succeeds, False when no space is available.
 */
 bool bt_mesh_create_entry_invalid_rerr(struct bt_mesh_route_entry **entry_location)
 {
 	/*if space found in slab, Allocate new node */
 	if (k_mem_slab_alloc(&routing_table_slab, (void **)&(*entry_location), ALLOCATION_INTERVAL) == 0) {
 		memset((*entry_location), 0, ENTRY_SIZE);                  /* Initializing with zeros */
 		k_sem_take(&invalid_rerr_list_sem, K_FOREVER);               /*take semaphore */
 		sys_slist_append(&invalid_rerr_list, &(*entry_location)->node); /*insert node in linkedlist */
 		k_sem_give(&invalid_rerr_list_sem);
 	} else    {
 		BT_ERR("Memory Allocation timeout");
 		return false;
 	}
 	/* Start the lifetime timer */
 	k_timer_init (&(*entry_location)->lifetime, bt_mesh_delete_entry_invalid_rerr, NULL);
 	k_timer_start(&(*entry_location)->lifetime, LIFETIME, 0);
 	return true;
 }

/**
 *	@brief Create entry in the invalid list.
 *
 *	@param entry_data: Pointer to pointer to structure of type bt_mesh_route_entry
 *										 holding data to be stored.
 *	@param timer_id: Pointer to structure of type k_timer holding lifetime.
 *
 *	@return True when allocation succeeds, False when no space is available.
 */
 bool bt_mesh_create_entry_invalid_with_cb(struct bt_mesh_route_entry **entry_location, \
 				  void (*timer_cb)(struct k_timer *timer_id))
 {

 	/* if space found in slab, Allocate new node */
 	if (k_mem_slab_alloc(&routing_table_slab, (void **)&(*entry_location), ALLOCATION_INTERVAL) == 0) {
 		memset(*(entry_location), 0, ENTRY_SIZE);                  /* Initializing with zeros */
 		k_sem_take(&invalid_list_sem, K_FOREVER);               /*take semaphore */
 		sys_slist_append(&invalid_list, &(*entry_location)->node); /*insert node in linkedlist */
 		k_sem_give(&invalid_list_sem);
 	} else    {
 		BT_ERR("Memory Allocation timeout");
 		return false;
 	}
 	/* Start the lifetime timer */
 	k_timer_init (&(*entry_location)->lifetime, timer_cb, NULL);
 	k_timer_start(&(*entry_location)->lifetime, RREQ_INTERVAL_WAIT, 0);
 	return true;
 }


/* Search Entry Functions */

/**
*	@brief Search in the valid list by source and destination.
*
*	@param source_address
*	@param destination_address
*	@param network index
*	@param entry: Pointer to pointer to structure of type bt_mesh_route_entry
*
*	@return
*			- Explicit: True when found, False otherwise.
*			- Implicit: Pointer to the found entry (4th param).
*/
bool bt_mesh_search_valid_destination(u16_t source_address, u16_t destination_address,u16_t net_idx, struct bt_mesh_route_entry **entry)
{
	struct bt_mesh_route_entry *entry1 = NULL;

	k_sem_take(&valid_list_sem, K_FOREVER);
	SYS_SLIST_FOR_EACH_CONTAINER(&valid_list, entry1, node){
		/* Search for the destination and source addresses in their range of elements */
		if ((destination_address >= entry1->destination_address) &&
		    (destination_address < (entry1->destination_address + entry1->destination_number_of_elements)) &&
		    (source_address >= entry1->source_address) &&
		    (source_address < (entry1->source_address + entry1->source_number_of_elements)) &&
		    (net_idx == entry1->net_idx)) {
			k_sem_give(&valid_list_sem);
			*entry = entry1;
			return true;
		}
	}
	k_sem_give(&valid_list_sem);
	return false;
}

/**
*	@brief Search in the valid list by destination only.
*
*	@param destination_address
*	@param network index
*	@param entry: Pointer to pointer to structure of type bt_mesh_route_entry
*
*	@return
*			- Explicit: True when found, False otherwise.
*			- Implicit: Pointer to the found entry (3rd param).
*/
bool bt_mesh_search_valid_destination_without_source(u16_t destination_address, u16_t net_idx, struct bt_mesh_route_entry **entry)
{
	struct bt_mesh_route_entry *entry1 = NULL;
	k_sem_take(&valid_list_sem, K_FOREVER); /*take semaphore */
	SYS_SLIST_FOR_EACH_CONTAINER(&valid_list, entry1, node){
		/* Search for the destination in range of its elements */
		if ((destination_address >= entry1->destination_address) &&
		    (destination_address < entry1->destination_address + entry1->destination_number_of_elements)&&
		    (net_idx==entry1->net_idx)) {
			k_sem_give(&valid_list_sem);
			*entry = entry1;
			return true;
		}
	}
	k_sem_give(&valid_list_sem);
	return false;
}


/**
*	@brief Search in the invalid list by source and destination.
*
*	@param source_address
*	@param destination_address
*	@param network index
*	@param entry: Pointer to pointer to structure of type bt_mesh_route_entry
*
*	@return
*			- Explicit: True when found, False otherwise.
*			- Implicit: Pointer to the found entry (4th param).
*/
bool bt_mesh_search_invalid_destination(u16_t source_address, u16_t destination_address, u16_t net_idx, struct bt_mesh_route_entry **entry)
{
	struct bt_mesh_route_entry *entry1 = NULL;

	k_sem_take(&invalid_list_sem, K_FOREVER); /*take semaphore */
	SYS_SLIST_FOR_EACH_CONTAINER(&invalid_list, entry1, node){
		/* Search for the destination and source in range of their elements */
		if ((destination_address >= entry1->destination_address && destination_address < entry1->destination_address + entry1->destination_number_of_elements)
		    && (source_address >= entry1->source_address && source_address < entry1->source_address + entry1->source_number_of_elements) &&
		    (net_idx==entry1->net_idx)) {
			k_sem_give(&invalid_list_sem);
			*entry = entry1;
			return true;
		}

	}
	k_sem_give(&invalid_list_sem);
	return false;
}


/**
*	@brief Search in the invalid rerr list by source and destination.
*
*	@param source_address
*	@param destination_address
*	@param network index
*	@param entry: Pointer to pointer to structure of type bt_mesh_route_entry
*
*	@return
*			- Explicit: True when found, False otherwise.
*			- Implicit: Pointer to the found entry (4th param).
*/
bool bt_mesh_search_invalid_rerr_destination(u16_t source_address, u16_t destination_address, u16_t net_idx, struct bt_mesh_route_entry **entry)
{
	struct bt_mesh_route_entry *entry1 = NULL;

	k_sem_take(&invalid_rerr_list_sem, K_FOREVER); /*take semaphore */
	SYS_SLIST_FOR_EACH_CONTAINER(&invalid_rerr_list, entry1, node){
		/* Search for the destination and source in range of their elements */
		if ((destination_address >= entry1->destination_address &&
		 destination_address < entry1->destination_address + entry1->destination_number_of_elements)
		    && (source_address >= entry1->source_address && source_address < entry1->source_address + entry1->source_number_of_elements) &&
		    (net_idx==entry1->net_idx)) {
			k_sem_give(&invalid_rerr_list_sem);
			*entry = entry1;
			return true;
		}

	}
	k_sem_give(&invalid_rerr_list_sem);
	return false;
}


/**
*	@brief Search in the invalid list by source and destination with range of destination elements.
*
*	@param source_address
*	@param destination_address
*	@param destination_number_of_elements
*	@param network index
*	@param entry: Pointer to pointer to structure of type bt_mesh_route_entry
*
*	@return
*			- Explicit: True when found, False otherwise.
*			- Implicit: Pointer to the found entry (5th param).
*/
bool bt_mesh_search_invalid_destination_with_range(u16_t source_address, u16_t destination_address, u16_t destination_number_of_elements, u16_t net_idx, struct bt_mesh_route_entry **entry)
{
	struct bt_mesh_route_entry *entry1 = NULL;

	k_sem_take(&invalid_list_sem, K_FOREVER); /*take semaphore */
	SYS_SLIST_FOR_EACH_CONTAINER(&invalid_list, entry1, node){
		if ((entry1->destination_address >= destination_address) &&
		    (entry1->destination_address < (destination_address + destination_number_of_elements))&&
		    (source_address == entry1->source_address) &&
		    (net_idx==entry1->net_idx)) {
			k_sem_give(&invalid_list_sem);
			*entry = entry1;
			return true;
		}
	}
	k_sem_give(&invalid_list_sem);
	return false;
}


/**
*	@brief Search in the valid list by destination, next hop and network index.
*
*	@param destination_address
*	@param next_hop
*	@param net_idx
*	@param void (*search_callback)(struct bt_mesh_route_entry *): callback function that's called each time
*							an entry matches.
*	@return : N/A
*/
void bt_mesh_search_valid_destination_nexthop_net_idx_with_cb(u16_t destination_address, u16_t next_hop, u16_t net_idx,
	 void (*search_callback)(struct bt_mesh_route_entry *,struct bt_mesh_route_entry **))
{
	/*temp is a pointer for the loop to run safely */
	struct bt_mesh_route_entry *iterator_entry,*temp;
	k_sem_take(&valid_list_sem, K_FOREVER);
	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&valid_list, iterator_entry,temp, node)
	{
		if ((destination_address == iterator_entry->destination_address) && (next_hop==iterator_entry->next_hop) && (net_idx==iterator_entry->net_idx))
		{
			k_sem_give(&valid_list_sem);
			search_callback(iterator_entry,&temp);
			k_sem_take(&valid_list_sem, K_FOREVER);
		}
	}
	k_sem_give(&valid_list_sem);
}

/**
*	@brief Search in the valid list by source and destination within certain network subnet.
*
*	@param source_address
*	@param destination_address
*	@param net_idx : network index
*	@param entry: Pointer to pointer to structure of type bt_mesh_route_entry
*
*	@return
*			- Explicit: True when found, False otherwise.
*			- Implicit: Pointer to the found entry (4th param).
*/
bool bt_mesh_search_valid_destination_with_net_idx(u16_t source_address, u16_t destination_address,u16_t net_idx, struct bt_mesh_route_entry **entry)
{
	struct bt_mesh_route_entry *entry1 = NULL;
	k_sem_take(&valid_list_sem, K_FOREVER);
	SYS_SLIST_FOR_EACH_CONTAINER(&valid_list, entry1, node){
		/* Search for the destination and source addresses in their range of elements */
		if ((destination_address == entry1->destination_address) &&
		    (source_address == entry1->source_address) &&
		    (net_idx ==entry1->net_idx )) {
			k_sem_give(&valid_list_sem);
			*entry = entry1; //FIXME entry and entry1 might later point to a deleted entries by another thread
			return true;
		}
	}
	k_sem_give(&valid_list_sem);
	return false;
}

/**
*	@brief Search in the valid list by nect hop within certain network subnet.
*
*	@param next_hop_address
*	@param net_idx : network index
*	@param entry: Pointer to pointer to structure of type bt_mesh_route_entry
*
*	@return
*			- Explicit: True when found, False otherwise.
*			- Implicit: Pointer to the found entry (3rd param).
*/
bool bt_mesh_search_valid_next_hop_with_net_idx(u16_t next_hop_address, u16_t net_idx, struct bt_mesh_route_entry **entry)
{
	struct bt_mesh_route_entry *entry1 = NULL;
	k_sem_take(&valid_list_sem, K_FOREVER); /*take semaphore */
	SYS_SLIST_FOR_EACH_CONTAINER(&valid_list, entry1, node)
	{
		if ((entry1->next_hop == next_hop_address) && (entry1->net_idx == net_idx))
		{
			k_sem_give(&valid_list_sem);
			*entry = entry1;
			return true;
		}
	}
	k_sem_give(&valid_list_sem);
	return false;
}

/**
*	@brief Search in the valid list by nect hop within certain network subnet.
*
*	@param next_hop_address
*	@param net_idx : network index
*	@param void (*search_callback)(struct bt_mesh_route_entry *) : callback function each time an entry is found.
*
*	@return : N/A
*/

void bt_mesh_search_valid_nexthop_net_idx_with_cb(u16_t nexthop, u16_t net_idx, void (*search_callback)(struct bt_mesh_route_entry *,struct bt_mesh_route_entry **))
{
	/*temp is a pointer to run the loop safely*/
	struct bt_mesh_route_entry *iterator_entry,*temp;
	k_sem_take(&valid_list_sem, K_FOREVER);
	/*loop over the routing table with the given destination and */
	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&valid_list, iterator_entry, temp ,node)
	{	/* Search for the destination and source addresses in their range of elements */
		if ((nexthop == iterator_entry->next_hop) && (net_idx==iterator_entry->net_idx))
		{
			k_sem_give(&valid_list_sem);
			search_callback(iterator_entry,&temp);
			k_sem_take(&valid_list_sem, K_FOREVER);
		}
	}
	k_sem_give(&valid_list_sem);
}



/* Delete Entry Functions */

/**
 *	@brief Delete vaild entry when destination node is unreachable.
 *
 *	@param deleted_entry: Pointer to struct of type bt_mesh_route_entry holding the entry needs to be deleted.
 */
void bt_mesh_delete_entry_link_drop(struct bt_mesh_route_entry *deleted_entry)
{
	k_timer_stop(&deleted_entry->lifetime);
	k_sem_take(&valid_list_sem, K_FOREVER);   							/* take semaphore */
	sys_slist_find_and_remove(&valid_list, &deleted_entry->node);   /*delete node from linked list */
	k_sem_give(&valid_list_sem);                            /*return semaphore */
	k_mem_slab_free(&routing_table_slab, (void **)&deleted_entry);  /*free space in slab*/
	BT_DBG("valid Entry Deleted");
}

/**
 *	@brief Delete vaild entry when lifetime expires.
 *
 *	@param timer_id: Pointer to struct of type k_timer holding lifetime of an entry.
 */
void bt_mesh_delete_entry_valid(struct k_timer *timer_id)
{
	/* container of timer_id to be deleted*/
	struct bt_mesh_route_entry *entry = CONTAINER_OF(timer_id, struct bt_mesh_route_entry, lifetime);
	BT_DBG("SRC=%04x,DST=%04x",entry->source_address,entry->destination_address);
	k_sem_take(&valid_list_sem, K_FOREVER);   							/* take semaphore */
	sys_slist_find_and_remove(&valid_list, &entry->node);   /*delete node from linked list */
	k_sem_give(&valid_list_sem);                            /*return semaphore */
	k_mem_slab_free(&routing_table_slab, (void **)&entry);  /*free space in slab*/
	BT_DBG("valid Entry Deleted");
}

/**
 *	@brief Delete invaild entry when lifetime expires.
 *
 *	@param timer_id: Pointer to struct of type k_timer holding lifetime of an entry.
 */
void bt_mesh_delete_entry_invalid(struct k_timer *timer_id)
{
	 /* container of timer_id to be deleted*/
	struct bt_mesh_route_entry *entry = CONTAINER_OF(timer_id, struct bt_mesh_route_entry, lifetime);
	BT_DBG("SRC=%04x,DST=%04x",entry->source_address,entry->destination_address);
	k_sem_take(&invalid_list_sem, K_FOREVER);               /* take semaphore */
	sys_slist_find_and_remove(&invalid_list, &entry->node); /*delete node from linked list */
	k_sem_give(&invalid_list_sem);                          /*return semaphore */
	k_mem_slab_free(&routing_table_slab, (void **)&entry);  /*free space in slab*/
	BT_DBG("Invalid Entry Deleted");
}


/**
 *	@brief Delete invaild rerr entry when lifetime expires.
 *
 *	@param timer_id: Pointer to struct of type k_timer holding lifetime of an entry.
 */
void bt_mesh_delete_entry_invalid_rerr(struct k_timer *timer_id)
{
	 /* container of timer_id to be deleted*/
	struct bt_mesh_route_entry *entry = CONTAINER_OF(timer_id, struct bt_mesh_route_entry, lifetime);
	k_sem_take(&invalid_rerr_list_sem, K_FOREVER);               /* take semaphore */
	sys_slist_find_and_remove(&invalid_rerr_list, &entry->node); /*delete node from linked list */
	k_sem_give(&invalid_rerr_list_sem);                          /*return semaphore */
	k_mem_slab_free(&routing_table_slab, (void **)&entry);  /*free space in slab*/
	BT_DBG("Invalid rerr Entry Deleted");
}

/* Refresh Functions */

/**
*	@brief Refresh the lifetime timer in an entry in the valid list when data
*				 is sent on the route.
*
*	@param entry: Pointer to structure of type bt_mesh_route_entry that contains
*								the lifetime to be refreshed.
*/
void bt_mesh_refresh_lifetime_valid(struct bt_mesh_route_entry *entry)
{
	k_timer_start(&entry->lifetime, LIFETIME, 0);
	struct bt_mesh_route_entry* reverse_entry;
	if (bt_mesh_search_valid_destination(entry->destination_address,entry->source_address,entry->net_idx,&reverse_entry))
	{
		k_timer_start(&reverse_entry->lifetime, LIFETIME, 0);
		BT_DBG("Lifetime of valid entry refreshed bidrectional");
	}
	else {
	BT_DBG("one directional entry updated");
  }
}

/**
*	@brief Refresh the lifetime timer in an entry in the invalid list when data
*				 is sent on the route.
*
*	@param entry: Pointer to structure of type bt_mesh_route_entry that contains
*								the lifetime to be refreshed.
*/
void bt_mesh_refresh_lifetime_invalid(struct bt_mesh_route_entry *entry)
{
	k_timer_start(&entry->lifetime, LIFETIME, 0);
	BT_DBG("Lifetime of invalid entry refreshed");
}

/* Miscellaneous */

/**
*	@brief Validate route having passed source and destination addresses
*
*	@param source_address
* @param destination_address
*
*	@return True when entry is found and refreshed, false otherwise.
*/
bool bt_mesh_validate_route(struct bt_mesh_route_entry *entry)
{
	if (entry == NULL )
	{
		return false;
	}
	k_timer_stop(&entry->lifetime);
	k_sem_take(&invalid_list_sem, K_FOREVER); /*take semaphore */
	sys_slist_find_and_remove(&invalid_list, &entry->node);
	k_sem_give(&invalid_list_sem);

	k_sem_take(&valid_list_sem, K_FOREVER);         /*take semaphore */
	sys_slist_append(&valid_list, &entry->node);    /*insert node in linkedlist */
	k_sem_give(&valid_list_sem);

	memset(&entry->lifetime,0,sizeof(struct k_timer));
	k_timer_init(&entry->lifetime, bt_mesh_delete_entry_valid, NULL);
	k_timer_start(&entry->lifetime, LIFETIME, 0);
	return true;
}


/**
*	@brief Inalidate route having passed source and destination addresses
*
*	@param source_address
* 	@param destination_address
*
*	@return True when entry is found and refreshed, false otherwise.
*/
bool bt_mesh_invalidate_route(struct bt_mesh_route_entry *entry)
{
 	if (entry == NULL )
 	{
		return false;
 	}
	k_timer_stop(&entry->lifetime);
	k_sem_take(&valid_list_sem, K_FOREVER); /*take semaphore */
	sys_slist_find_and_remove(&valid_list, &entry->node);
	k_sem_give(&valid_list_sem);

	k_sem_take(&invalid_list_sem, K_FOREVER);       /*take semaphore */
	sys_slist_append(&invalid_list, &entry->node);  /*insert node in linkedlist */
	k_sem_give(&invalid_list_sem);

	memset(&entry->lifetime,0,sizeof(struct k_timer));
	k_timer_init(&entry->lifetime, bt_mesh_delete_entry_invalid, NULL);
	k_timer_start(&entry->lifetime, LIFETIME, 0);
	return true;
}

bool bt_mesh_invalidate_rerr_route(struct bt_mesh_route_entry *entry)
{
 	if (entry == NULL )
 	{
		return false;
 	}
	k_timer_stop(&entry->lifetime);
	k_sem_take(&valid_list_sem, K_FOREVER); /*take semaphore */
	sys_slist_find_and_remove(&valid_list, &entry->node);
	k_sem_give(&valid_list_sem);

	k_sem_take(&invalid_rerr_list_sem, K_FOREVER);       /*take semaphore */
	sys_slist_append(&invalid_rerr_list, &entry->node);  /*insert node in linkedlist */
	k_sem_give(&invalid_rerr_list_sem);

	memset(&entry->lifetime,0,sizeof(struct k_timer));
	k_timer_init(&entry->lifetime, bt_mesh_delete_entry_invalid_rerr, NULL);
	k_timer_start(&entry->lifetime, LIFETIME, 0);
	return true;
}


/* Test Functions */
void view_valid_list()
{
	if (sys_slist_is_empty(&valid_list)) {
		BT_DBG("Valid List is empty");
		return;
	}
	struct bt_mesh_route_entry *entry = NULL;
	k_sem_take(&valid_list_sem, K_FOREVER);
	SYS_SLIST_FOR_EACH_CONTAINER(&valid_list, entry, node){
		BT_DBG("\x1b[32mValid List:source address=%04x,destination address=%04x,nexthop address=%04x\x1b[0m", entry->source_address, entry->destination_address,entry->next_hop);
	}
	k_sem_give(&valid_list_sem);
}

void view_invalid_list()
{
	if (sys_slist_is_empty(&invalid_list)) {
		BT_DBG("Invalid List is empty");
		return;
	}
	struct bt_mesh_route_entry *entry = NULL;
	k_sem_take(&invalid_list_sem, K_FOREVER);
	SYS_SLIST_FOR_EACH_CONTAINER(&invalid_list, entry, node){
		BT_DBG("\x1b[31mInvalid List:source address=%04x,destination address=%04x\x1b[0m", entry->source_address, entry->destination_address);
	}
	k_sem_give(&invalid_list_sem);
}

void view_invalid_rerr_list()
{
	if (sys_slist_is_empty(&invalid_rerr_list)) {
		BT_DBG("Invalid rerr List is empty");
		return;
	}
	struct bt_mesh_route_entry *entry = NULL;
	k_sem_take(&invalid_rerr_list_sem, K_FOREVER);
	SYS_SLIST_FOR_EACH_CONTAINER(&invalid_rerr_list, entry, node){
		BT_DBG("\x1b[31mInvalid rerr List:source address=%04x,destination address=%04x\x1b[0m", entry->source_address, entry->destination_address);
	}
	k_sem_give(&invalid_rerr_list_sem);
}
