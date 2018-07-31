/** @file routing_table.h
 *  @brief Reverse and forward tables constructed by AODV.
 *
 *  @bug No known bugs.
 */

 /* -- Includes -- */


/** @brief Entry data of routing table */
struct bt_mesh_route_entry {
	u16_t source_address;                   /* Destination address (2B) */
	u16_t destination_address;              /* Source address (2B)*/
	u32_t destination_sequence_number;      /* Destination Sequence number (4B)*/
	u16_t next_hop;                         /* Next hop address (2B)*/
	u16_t source_number_of_elements;		/* Source number of elements (2B) */
	u16_t destination_number_of_elements;	/* Destination number of elements (2B) */
	u8_t hop_count;                 		/* Number of hops (1B) */
	s8_t rssi;								/* Average RSSI (1B) */
	bool repairable;						/* Repairable Flag (1B) */
	u16_t net_idx;							/* Network Index (2B) */
	struct k_timer lifetime;        		/* Lifetime timer (52B) */
	sys_snode_t node;               		/* Linkedlist node (4B) */
};

/* FUNCTIONS PROTOTYPES */

void bt_mesh_routing_table_init();

/* Create Entry Functions */
bool bt_mesh_create_entry_valid(struct bt_mesh_route_entry **entry_data);
bool bt_mesh_create_entry_invalid(struct bt_mesh_route_entry **entry_data);
bool bt_mesh_create_entry_invalid_rerr(struct bt_mesh_route_entry **entry_location);
bool bt_mesh_create_entry_invalid_with_cb(struct bt_mesh_route_entry **entry_data, void (*timer_cb)(struct k_timer *timer_id));

/* Search Entry Functions */
bool bt_mesh_search_valid_destination(u16_t source_address, u16_t destination_address, u16_t net_idx, struct bt_mesh_route_entry **entry);
bool bt_mesh_search_valid_destination_without_source(u16_t destination_address, u16_t net_idx, struct bt_mesh_route_entry **entry);
bool bt_mesh_search_invalid_destination(u16_t source_address, u16_t destination_address, u16_t net_idx, struct bt_mesh_route_entry **entry);
bool bt_mesh_search_invalid_rerr_destination(u16_t source_address, u16_t destination_address, u16_t net_idx, struct bt_mesh_route_entry **entry);
bool bt_mesh_search_invalid_destination_with_range(u16_t source_address, u16_t destination_address, u16_t destination_number_of_elements, u16_t net_idx, struct bt_mesh_route_entry **entry);
bool bt_mesh_search_valid_next_hop_with_net_idx(u16_t next_hop_address, u16_t net_idx, struct bt_mesh_route_entry **entry);
void bt_mesh_search_valid_destination_nexthop_net_idx_with_cb(u16_t destination_address, u16_t next_hop, u16_t net_idx,
	 								 void (*search_callback)(struct bt_mesh_route_entry *,struct bt_mesh_route_entry **));
bool bt_mesh_search_valid_destination_with_net_idx(u16_t source_address, u16_t destination_address,u16_t net_idx, struct bt_mesh_route_entry **entry);
void bt_mesh_search_valid_nexthop_net_idx_with_cb(u16_t nexthop, u16_t net_idx, void (*cb)(struct bt_mesh_route_entry *,struct bt_mesh_route_entry **));

/* Delete Entry Functions */
void bt_mesh_delete_entry_link_drop(struct bt_mesh_route_entry *deleted_entry);
void bt_mesh_delete_entry_valid(struct k_timer *timer_id);
void bt_mesh_delete_entry_invalid(struct k_timer *timer_id);
void bt_mesh_delete_entry_invalid_rerr(struct k_timer *timer_id);

/* Refresh Functions */
void bt_mesh_refresh_lifetime_valid(struct bt_mesh_route_entry *entry);
void bt_mesh_refresh_lifetime_invalid(struct bt_mesh_route_entry *entry);

/* Miscellaneous */
bool bt_mesh_validate_route(struct bt_mesh_route_entry *entry);
bool bt_mesh_invalidate_route(struct bt_mesh_route_entry *entry);
bool bt_mesh_invalidate_rerr_route(struct bt_mesh_route_entry *entry);

/* Test Functions */
void view_valid_list();
void view_invalid_list();
void view_invalid_rerr_list();
