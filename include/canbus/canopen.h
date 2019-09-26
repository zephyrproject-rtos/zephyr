/*
 * Copyright (c) 2019 Vestas Wind Systems A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief CANopen Network Stack
 * @defgroup canopen CANopen Network Stack
 * @ingroup CAN
 * @{
 */

#ifndef ZEPHYR_INCLUDE_CANOPEN_H_
#define ZEPHYR_INCLUDE_CANOPEN_H_

#include <CANopen.h>
#include <CO_Emergency.h>
#include <CO_SDO.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief CANopen object dictionary storage types.
 */
enum canopen_storage {
	CANOPEN_STORAGE_RAM,
	CANOPEN_STORAGE_ROM,
	CANOPEN_STORAGE_EEPROM,
};

/**
 * @brief Attach CANopen object dictionary storage handlers.
 *
 * Attach CANopen storage handler functions to object dictionary
 * indexes 0x1010 (Store parameters) and 0x1011 (Restore default
 * parameters). This function must be called after calling CANopenNode
 * `CO_init()`.
 *
 * The handlers will save object dictionary entries of type @ref
 * CANOPEN_STORAGE_ROM to non-volatile storage when a CANopen SDO
 * client writes 0x65766173 ('s', 'a', 'v', 'e' from LSB to MSB) to
 * object dictionary index 0x1010 sub-index 1.
 *
 * Object dictionary entries of types @ref CANOPEN_STORAGE_ROM (and
 * optionally @ref CANOPEN_STORAGE_EEPROM) will be deleted from
 * non-volatile storage when a CANopen SDO client writes 0x64616F6C
 * ('l', 'o', 'a', 'd' from LSB to MSB) to object dictionary index
 * 0x1011 sub-index 1.
 *
 * Object dictionary entries of type @ref CANOPEN_STORAGE_EEPROM may be
 * saved by the application by periodically calling @ref
 * canopen_storage_save().
 *
 * Object dictionary entries of type @ref CANOPEN_STORAGE_RAM are
 * never saved to non-volatile storage.
 *
 * @param sdo CANopenNode SDO server object
 * @param em  CANopenNode Emergency object
 */
void canopen_storage_attach(CO_SDO_t *sdo, CO_EM_t *em);

/**
 * @brief Save CANopen object dictionary entries to non-volatile storage.
 *
 * Save object dictionary entries of a given type to non-volatile
 * storage.
 *
 * @param storage CANopen object dictionary entry type
 *
 * @return 0 if successful, negative errno code if failure
 */
int canopen_storage_save(enum canopen_storage storage);

/**
 * @brief Erase CANopen object dictionary entries from non-volatile storage.
 *
 * Erase object dictionary entries of a given type from non-volatile
 * storage.
 *
 * @param storage CANopen object dictionary entry type
 *
 * @return 0 if successful, negative errno code if failure
 */
int canopen_storage_erase(enum canopen_storage storage);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_CANOPEN_H_ */
