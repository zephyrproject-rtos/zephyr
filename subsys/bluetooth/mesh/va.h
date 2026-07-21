/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stdbool.h>

/** Virtual address entry. */
struct bt_mesh_va {
	uint16_t ref:15,
		 changed:1;
	uint16_t addr;
	uint8_t  uuid[16];
};

/** @brief Store Label UUID.
 *
 * @param uuid   Label UUID to be stored.
 * @param entry  Pointer to a memory where a new or updated entry will be stored, or NULL.
 *
 * @return STATUS_SUCCESS if entry is stored or updated, error code otherwise.
 */
uint8_t bt_mesh_va_add(const uint8_t uuid[16], const struct bt_mesh_va **entry);

/** @brief Delete Label UUID.
 *
 * @c uuid must be a pointer to @ref bt_mesh_va.uuid. Use @ref bt_mesh_va_uuid_get to get valid
 * pointer.
 *
 * @param uuid Label UUID to delete.
 *
 * @return STATUS_SUCCESS if entry is deleted, error code otherwise.
 */
uint8_t bt_mesh_va_del(const uint8_t *uuid);

/** @brief Find virtual address entry by Label UUID.
 *
 * @c uuid can be a user data.
 *
 * @param uuid pointer to memory with Label UUID to be found.
 *
 * @return Pointer to a valid entry, NULL otherwise.
 */
const struct bt_mesh_va *bt_mesh_va_find(const uint8_t *uuid);

/** @brief Check if there are more than one Label UUID which hash has the specified virtual
 * address.
 *
 * @param addr Virtual address to check
 *
 * @return True if there is collision, false otherwise.
 */
bool bt_mesh_va_collision_check(uint16_t addr);

/* Needed for storing va as indexes in persistent storage. */
/** @brief Get Label UUID by index.
 *
 * @param idx Index of virtual address entry.
 *
 * @return Pointer to a valid Label UUID, or NULL if entry is not found.
 */
const uint8_t *bt_mesh_va_get_uuid_by_idx(uint16_t idx);

/** @brief Get virtual address entry index by Label UUID.
 *
 * @c uuid must be a pointer to @ref bt_mesh_va.uuid. Use @ref bt_mesh_va_uuid_get to get valid
 * pointer.
 *
 * @param uuid  Label UUID which index to find
 * @param uuidx Pointer to a memory where to store index.
 *
 * @return 0 if entry is found, error code otherwise.
 */
int bt_mesh_va_get_idx_by_uuid(const uint8_t *uuid, uint16_t *uuidx);

/** @brief Store pending virtual address entries in the persistent storage.*/
void bt_mesh_va_pending_store(void);

/** @brief Remove all stored virtual addresses and remove them from the persistent storage. */
void bt_mesh_va_clear(void);
