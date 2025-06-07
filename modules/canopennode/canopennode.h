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

#ifndef ZEPHYR_MODULES_CANOPENNODE_CANOPENNODE_H_
#define ZEPHYR_MODULES_CANOPENNODE_CANOPENNODE_H_

#include <CANopen.h>

#ifdef __cplusplus
extern "C" {
#endif

struct canopen_context {
	const struct device *dev;
	uint8_t pending_nodeid;
	uint16_t pending_bitrate;
	uint8_t active_nodeid;
	uint16_t active_bitrate;

	CO_t *co; /* back reference to canopen instance */
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
void canopen_storage_attach(struct canopen_context *ctx,
			    OD_entry_t *OD_1010_entry, /* store */
			    OD_entry_t *OD_1011_entry, /* restore */
			    CO_storage_entry_t *storage_entries,
			    OD_size_t storage_entries_count);

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
void canopen_program_download_attach(struct canopen_context *ctx);

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
 * @param ctx canopennode context for reference
 * @param green_cb callback for changing state on the green LED indicator.
 * @param green_arg argument to pass to the green LED indicator callback.
 * @param red_cb callback for changing state on the red LED indicator.
 * @param red_arg argument to pass to the red LED indicator callback.
 */
void canopen_leds_init(struct canopen_context *ctx,
		       canopen_led_callback_t green_cb, void *green_arg,
		       canopen_led_callback_t red_cb, void *red_arg);

/**
 * @brief Update CANopen LED indicators.
 *
 * Update the CANopen LED indicators and from the processing loop to
 * query the CO_LEDs object and update callbacks as needed.
 *
 * @param ctx canopennode context for reference
 */
void canopen_leds_update(struct canopen_context *ctx);

/**
 * @brief Callback for incoming CAN message
 *
 * This callback will be called from interrupt context and should therefore
 * return quickly.
 *
 * It can be used to e.g. wake the loop polling calling CO_process.
 */
typedef void (*canopen_rxmsg_callback_t)(void);

/**
 * @brief Set callback for incoming CAN message
 *
 * Set up callback to be called on incoming CAN message on any of
 * the configured filters for CANopenNode.
 *
 * This can be used to wake the loop calling CO_process when an incoming
 * message needs to be processed.
 *
 * Setting a new callback will overwrite any existing callback.
 *
 * @param callback the callback to set
 */
void canopen_set_rxmsg_callback(canopen_rxmsg_callback_t callback);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_MODULES_CANOPENNODE_CANOPENNODE_H_ */
