/*
 * Copyright (c) 2022 Intellinium <giuliano.franchetto@intellinium.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_LORAWAN_NVM_H_
#define ZEPHYR_SUBSYS_LORAWAN_NVM_H_

#include <stddef.h>

/**
 * @brief Initialize storage for LoRaWAN backend records.
 *
 * Internal interface; call before reading or writing records.
 *
 * @return 0 on success, or a negative storage error otherwise.
 */
int lorawan_nvm_init(void);

/**
 * @brief Restore persisted state for the selected LoRaWAN backend.
 *
 * Implemented by the MAC backend selected at build time. Called internally
 * during lorawan_start(), after lorawan_nvm_init(). Calls must be serialized
 * with other MAC operations. The backend defines which records are restored
 * and how missing records are initialized.
 *
 * On failure, the caller must not start the MAC with the restored state.
 *
 * @retval 0 Backend state restored successfully.
 * @return Negative storage or backend validation error otherwise.
 */
int lorawan_nvm_restore(void);

/**
 * @brief Read a complete LoRaWAN backend record.
 *
 * Backends own record names, serialization and validation of their contents.
 * No backend state is applied by this function. Discard the output on failure.
 * Missing or erased records cannot be distinguished from first use.
 *
 * @param name Complete Settings key identifying the record.
 * @param data Destination buffer.
 * @param size Expected record size, which must match the stored size exactly.
 * @retval 0 Record read successfully.
 * @retval -ENOENT Record not found.
 * @retval -EINVAL Invalid argument or stored record size.
 * @retval -EIO Short read.
 * @return Other negative storage errors are propagated.
 */
int lorawan_nvm_read(const char *name, void *data, size_t size);

/**
 * @brief Write a complete LoRaWAN backend record.
 *
 * Backends decide when state must be stored and how to handle failures.
 * Write durability follows the selected Settings backend's guarantees.
 *
 * @param name Complete Settings key identifying the record.
 * @param data Serialized record to store.
 * @param size Record size in bytes, greater than zero.
 * @retval 0 Record written successfully.
 * @retval -EINVAL Invalid argument.
 * @return Other negative storage errors are propagated.
 */
int lorawan_nvm_write(const char *name, const void *data, size_t size);

#endif /* ZEPHYR_SUBSYS_LORAWAN_NVM_H_ */
