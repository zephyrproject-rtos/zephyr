/** @file
 *  @brief Bluetooth subsystem persistent storage APIs.
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __BT_STORAGE_H
#define __BT_STORAGE_H

/**
 * @brief Persistent Storage
 * @defgroup bt_storage Persistent Storage
 * @ingroup bluetooth
 * @{
 */

#include <sys/types.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Well known storage keys */
enum {
	/** Identity Address.
	  * Type: bt_addr_le_t (7 bytes)
	  */
	BT_STORAGE_ID_ADDR,

	/** Local Identity Resolving Key.
	  * Type: uint8_t key[16]
	  */
	BT_STORAGE_LOCAL_IRK,

	/** List of addresses of remote devices.
	  * Type: bt_addr_le_t addrs[n] (length is variable).
	  *
	  * This is only used for reading. Modification of the list happens
	  * implicitly by writing entries for each remote device. This value
	  * is only used with the local storage, i.e. NULL as the target
	  * bt_addr_le_t passed to the read callback.
	  */
	BT_STORAGE_ADDRESSES,

	/** Slave Long Term Key for legacy pairing.
	  * Type: struct bt_storage_ltk
	  */
	BT_STORAGE_SLAVE_LTK,

	/** Long Term Key for legacy pairing.
	  * Type: struct bt_storage_ltk
	  */
	BT_STORAGE_LTK,

	/** Identity Resolving Key
	  * Type: uint8_t key[16]
	  */
	BT_STORAGE_IRK,
};

/** LTK key flags */
enum {
	/* Key has been generated with MITM protection */
	BT_STORAGE_LTK_AUTHENTICATED   = BIT(0),

	/* Key has been generated using the LE Secure Connection pairing */
	BT_STORAGE_LTK_SC              = BIT(1),
};

struct bt_storage_ltk {
	uint8_t                 flags;
	/* Encryption key size used to generate key */
	uint8_t                 size;
	uint16_t                ediv;
	uint8_t                 rand[8];
	uint8_t                 val[16];
};

struct bt_storage {
	/** Read the value of a key from storage.
	 *
	 *  @param addr   Remote address or NULL for local storage
	 *  @param key    BT_STORAGE_* key to read
	 *  @param data   Memory location to place the data
	 *  @param length Maximum number of bytes to read
	 *
	 *  @return Number of bytes read or negative error value on
	 *          failure.
	 */
	ssize_t (*read)(const bt_addr_le_t *addr, uint16_t key,
			void *data, size_t length);

	/** Write the value of a key to storage.
	 *
	 *  @param addr   Remote address or NULL for local storage
	 *  @param key    BT_STORAGE_* key to write
	 *  @param data   Memory location of the data
	 *  @param length Number of bytes to write
	 *
	 *  @return Number of bytes written or negative error value on
	 *          failure.
	 */
	ssize_t (*write)(const bt_addr_le_t *addr, uint16_t key,
			 const void *data, size_t length);

	/** Clear all keys for a specific address
	 *
	 *  @param addr   Remote address, BT_ADDR_LE_ANY for all
	 *                remote devices, or NULL for local storage.
	 *
	 *  @return 0 on success or negative error value on failure.
	 */
	int (*clear)(const bt_addr_le_t *addr);

};

/** Register callbacks for storage handling.
  *
  * @param storage Callback struct.
  */
void bt_storage_register(const struct bt_storage *storage);

/** Clear all storage keys for a specific address
  *
  * @param addr  Remote address, NULL for local storage or
  *              BT_ADDR_LE_ANY to clear all remote devices.
  *
  * @return 0 on success or negative error value on failure.
  */
int bt_storage_clear(const bt_addr_le_t *addr);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* __BT_STORAGE_H */
