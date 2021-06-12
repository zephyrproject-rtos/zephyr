/*
 * Copyright (c) 2019 Vestas Wind Systems A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */


/**
 * @defgroup CAN CAN BUS
 * @{
 * @}
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

struct canopen_context {
	const struct device *dev;
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

/**
 * @brief Attach CANopen object dictionary program download handlers.
 *
 * Attach CANopen program download functions to object dictionary
 * indexes 0x1F50, 0x1F51, 0x1F56, and 0x1F57. This function must be
 * called after calling CANopenNode `CO_init()`.
 *
 * @param nmt CANopenNode NMT object
 * @param sdo CANopenNode SDO server object
 * @param em  CANopenNode Emergency object
 */
void canopen_program_download_attach(CO_NMT_t *nmt, CO_SDO_t *sdo, CO_EM_t *em);

/**
 * @typedef canopen_led_callback_t
 * @brief CANopen LED indicator callback function signature.
 *
 * @param value true if the LED indicator shall be turned on, false otherwise.
 * @param arg argument that was passed when LEDs were initialized.
 */
typedef void (*canopen_led_callback_t)(bool value, void *arg);

/**
 * @brief Initialize CANopen LED indicators.
 *
 * Initialize CANopen LED indicators and attach callbacks for setting
 * their state. Two LED indicators, a red and a green, are supported
 * according to CiA 303-3.
 *
 * @param nmt CANopenNode NMT object.
 * @param green_cb callback for changing state on the green LED indicator.
 * @param green_arg argument to pass to the green LED indicator callback.
 * @param red_cb callback for changing state on the red LED indicator.
 * @param red_arg argument to pass to the red LED indicator callback.
 */
void canopen_leds_init(CO_NMT_t *nmt,
		       canopen_led_callback_t green_cb, void *green_arg,
		       canopen_led_callback_t red_cb, void *red_arg);

/**
 * @brief Indicate CANopen program download in progress
 *
 * Indicate that a CANopen program download is in progress.
 *
 * @param in_progress true if program download is in progress, false otherwise
 */
void canopen_leds_program_download(bool in_progress);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_CANOPEN_H_ */
