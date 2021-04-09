/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BT_OTS_DIR_LIST_INTERNAL_H_
#define BT_OTS_DIR_LIST_INTERNAL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>
#include <zephyr/types.h>
#include <bluetooth/gatt.h>
#include <bluetooth/services/ots.h>

struct bt_ots_dir_list {
	struct net_buf_simple net_buf;
	struct bt_gatt_ots_object *dir_list_obj;
	uint8_t _content[DIR_LIST_MAX_SIZE];
};

/** @brief Directory Listing record flag field. */
enum {
	/** Bit 0 Object Type UUID Size 0: 16bit 1: 128bit*/
	BT_OTS_DIR_LIST_FLAG_TYPE_128            = 0,
	/** Bit 1 Current Size Present*/
	BT_OTS_DIR_LIST_FLAG_CUR_SIZE            = 1,
	/** Bit 2 Allocated Size Present */
	BT_OTS_DIR_LIST_FLAG_ALLOC_SIZE          = 2,
	/** Bit 3 Object First-Created Present*/
	BT_OTS_DIR_LIST_FLAG_FIRST_CREATED       = 3,
	/** Bit 4 Object Last-Modified Present*/
	BT_OTS_DIR_LIST_FLAG_LAST_MODIFIED       = 4,
	/** Bit 5 Object Properties Present*/
	BT_OTS_DIR_LIST_FLAG_PROPERTIES          = 5,
	/** Bit 6 RFU*/
	BT_OTS_DIR_LIST_FLAG_RFU                 = 6,
	/** Bit 7 Extended Flags Present*/
	BT_OTS_DIR_LIST_FLAG_EXTENDED            = 7,
};

/** @brief Set @ref BT_OTS_DIR_LIST_SET_FLAG_TYPE_128 flag.
 *
 *  @param flags Directory Listing Object flags.
 */
#define BT_OTS_DIR_LIST_SET_FLAG_TYPE_128(flags) \
	WRITE_BIT((flags), BT_OTS_DIR_LIST_FLAG_TYPE_128, 1)

/** @brief Set @ref BT_OTS_DIR_LIST_FLAG_CUR_SIZE flag.
 *
 *  @param flags Directory Listing Object flags.
 */
#define BT_OTS_DIR_LIST_SET_FLAG_CUR_SIZE(flags) \
	WRITE_BIT((flags), BT_OTS_DIR_LIST_FLAG_CUR_SIZE, 1)

/** @brief Set @ref BT_OTS_DIR_LIST_FLAG_ALLOC_SIZE flag.
 *
 *  @param flags Directory Listing Object flags.
 */
#define BT_OTS_DIR_LIST_SET_FLAG_ALLOC_SIZE(flags) \
	WRITE_BIT((flags), BT_OTS_DIR_LIST_FLAG_ALLOC_SIZE, 1)

/** @brief Set @ref BT_OTS_DIR_LIST_FLAG_FIRST_CREATED flag.
 *
 *  @param flags Directory Listing Object flags.
 */
#define BT_OTS_DIR_LIST_SET_FLAG_FIRST_CREATED(flags) \
	WRITE_BIT((flags), BT_OTS_DIR_LIST_FLAG_FIRST_CREATED, 1)

/** @brief Set @ref BT_OTS_DIR_LIST_FLAG_LAST_MODIFIED flag.
 *
 *  @param flags Directory Listing Object flags.
 */
#define BT_OTS_DIR_LIST_SET_FLAG_LAST_MODIFIED(flags) \
	WRITE_BIT((flags), BT_OTS_DIR_LIST_FLAG_LAST_MODIFIED, 1)

/** @brief Set @ref BT_OTS_DIR_LIST_FLAG_PROPERTIES flag.
 *
 *  @param flags Directory Listing Object flags.
 */
#define BT_OTS_DIR_LIST_SET_FLAG_PROPERTIES(flags) \
	WRITE_BIT((flags), BT_OTS_DIR_LIST_FLAG_PROPERTIES, 1)

/** @brief Set @ref BT_OTS_DIR_LIST_FLAG_EXTENDED flag.
 *
 *  @param flags Directory Listing Object flags.
 */
#define BT_OTS_DIR_LIST_SET_FLAG_EXTENDED(flags) \
	WRITE_BIT((flags), BT_OTS_DIR_LIST_FLAG_EXTENDED, 1)

/** @brief Get @ref BT_OTS_DIR_LIST_GET_FLAG_TYPE_128 flag.
 *
 *  @param flags Directory Listing Object flags.
 */
#define BT_OTS_DIR_LIST_GET_FLAG_TYPE_128(flags) \
	((flags) & BIT(BT_OTS_DIR_LIST_FLAG_TYPE_128))

/** @brief Get @ref BT_OTS_DIR_LIST_GET_FLAG_CUR_SIZE flag.
 *
 *  @param flags Directory Listing Object flags.
 */
#define BT_OTS_DIR_LIST_GET_FLAG_CUR_SIZE(flags) \
	((flags) & BIT(BT_OTS_DIR_LIST_FLAG_CUR_SIZE))

/** @brief Get @ref BT_OTS_DIR_LIST_GET_FLAG_ALLOC_SIZE flag.
 *
 *  @param flags Directory Listing Object flags.
 */
#define BT_OTS_DIR_LIST_GET_FLAG_ALLOC_SIZE(flags) \
	((flags) & BIT(BT_OTS_DIR_LIST_FLAG_ALLOC_SIZE))

/** @brief Get @ref BT_OTS_DIR_LIST_GET_FLAG_FIRST_CREATED flag.
 *
 *  @param flags Directory Listing Object flags.
 */
#define BT_OTS_DIR_LIST_GET_FLAG_FIRST_CREATED(flags) \
	((flags) & BIT(BT_OTS_DIR_LIST_FLAG_FIRST_CREATED))

/** @brief Get @ref BT_OTS_DIR_LIST_GET_FLAG_LAST_MODIFIED flag.
 *
 *  @param flags Directory Listing Object flags.
 */
#define BT_OTS_DIR_LIST_GET_FLAG_LAST_MODIFIED(flags) \
	((flags) & BIT(BT_OTS_DIR_LIST_FLAG_LAST_MODIFIED))

/** @brief Get @ref BT_OTS_DIR_LIST_GET_FLAG_PROPERTIES flag.
 *
 *  @param flags Directory Listing Object flags.
 */
#define BT_OTS_DIR_LIST_GET_FLAG_PROPERTIES(flags) \
	((flags) & BIT(BT_OTS_DIR_LIST_FLAG_PROPERTIES))

/** @brief Get @ref BT_OTS_DIR_LIST_GET_FLAG_EXTENDED flag.
 *
 *  @param flags Directory Listing Object flags.
 */
#define BT_OTS_DIR_LIST_GET_FLAG_EXTENDED(flags) \
	((flags) & BIT(BT_OTS_DIR_LIST_FLAG_EXTENDED))

void bt_ots_dir_list_obj_add(struct bt_ots_dir_list *dir_list, void *obj_manager,
			     struct bt_gatt_ots_object *cur_obj, struct bt_gatt_ots_object *objj);
void bt_ots_dir_list_obj_remove(struct bt_ots_dir_list *dir_list, void *obj_manager,
				struct bt_gatt_ots_object *cur_obj, struct bt_gatt_ots_object *obj);
void bt_ots_dir_list_selected(struct bt_ots_dir_list *dir_list, void *obj_manager,
			      struct bt_gatt_ots_object *cur_obj);
void bt_ots_dir_list_init(struct bt_ots_dir_list **dir_list, void *obj_manager);
int bt_ots_dir_list_content_get(struct bt_ots_dir_list *dir_list, uint8_t **data,
				uint32_t len, uint32_t offset);

#ifdef __cplusplus
}
#endif

#endif /* BT_OTS_DIR_LIST_INTERNAL_H_ */
