/*
 * Copyright (c) 2024 Semtech Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SMTC_MODEM_HAL_INIT_H
#define SMTC_MODEM_HAL_INIT_H

#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/sys_clock.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Defines the battery level callback handler function signature.
 *
 * @retval 0      if the node is connected to an external power source
 * @retval 1..254 battery level, where 1 is the minimum and 254 is the maximum value
 * @retval 255    if the node was not able to measure the battery level
 */
typedef uint8_t (*lorawan_battery_level_cb_t)(void);

/**
 * @brief Defines the battery voltage callback handler function signature.
 *
 * @retval The battery voltage, in mV
 */
typedef uint16_t (*lorawan_battery_voltage_cb_t)(void);

/**
 * @brief Defines the system temperature callback handler function signature.
 *
 * @retval The temperature, in °C
 */
typedef int8_t (*lorawan_temperature_cb_t)(void);

#ifdef CONFIG_LORA_BASICS_MODEM_FUOTA

struct lorawan_fuota_cb {
	/**
	 * @brief Callback to return the hardware version.
	 *
	 * See HW version in FMP Specification TS006-1.0.0.
	 *
	 * @return The device's hardware version.
	 */
	uint32_t (*get_hw_version)(void);

	/**
	 * @brief Callback to return the firmware version.
	 *
	 * See FW version in FMP Specification TS006-1.0.0.
	 *
	 * @return The device's firmware version.
	 */
	uint32_t (*get_fw_version)(void);

	/**
	 * @brief Callback to return the firmware status.
	 *
	 * See UpImageStatus in FMP Specification TS006-1.0.0.
	 *
	 * @return The device's firmware status.
	 */
	uint8_t (*get_fw_status_available)();

	/**
	 * @brief Callback to return the firmware upgrade next version.
	 *
	 * See nextFirmwareVersion in FMP Specification TS006-1.0.0.
	 *
	 * @return The next firmware upgrade image version.
	 */
	uint32_t (*get_next_fw_version)(void);

	/**
	 * @brief Callback to delete a chosen firmware upgrade image.
	 *
	 * See DevDeleteImageReq in FMP Specification TS006-1.0.0.
	 *
	 * @param [in] fw_version The firmware version to delete.
	 * @return 0 if the firmware was deleted, or a DevDeleteImageAns.
	 */
	uint8_t (*get_fw_delete_status)(uint32_t fw_version);
};

#endif /* CONFIG_LORA_BASICS_MODEM_FUOTA */

#ifdef CONFIG_LORA_BASICS_MODEM_USER_STORAGE_IMPL

struct lorawan_user_storage_cb {

	/**
	 * @brief Persistently store context from the lora basics modem
	 *
	 * The application should use some persistent storage to store the context.
	 *
	 * @param[in] ctx_id The ID of the context to store. Each ID must be stored
	 * separately.
	 * @param[in] offset Memory offset after ctx address
	 * @param[in] buffer The buffer to store.
	 * @param[in] size The size of the buffer to store, in bytes.
	 */
	void (*context_store)(const uint8_t ctx_id, const uint32_t offset, const uint8_t *buffer,
			      const uint32_t size);

	/**
	 * @brief Restore context to the lora basics modem
	 *
	 * The application should load the context from the persistent storage used in
	 * context_store.
	 *
	 * @param[in] ctx_id The ID of the context to restore.
	 * @param[in] offset Memory offset after ctx address
	 * @param[in] buffer The buffer to read into.
	 * @param[in] size The size of the buffer, in bytes.
	 */
	void (*context_restore)(const uint8_t ctx_id, const uint32_t offset, uint8_t *buffer,
				const uint32_t size);
};

#endif /* CONFIG_LORA_BASICS_MODEM_USER_STORAGE_IMPL */

/**
 * @brief Initialization of the hal implementation.
 *
 * This must be called before smtc_modem_init
 *
 * @param[in] transceiver The device pointer of the transceiver instance that will be used.
 */
void lorawan_smtc_modem_hal_init(const struct device *transceiver);

/**
 * @brief Register a battery level callback function.
 *
 * Provide the LoRaWAN stack with a function to be called whenever a battery
 * level needs to be read.
 *
 * Should no callback be provided the lorawan backend will report 255.
 *
 * @param cb Pointer to the battery level function
 */
void lorawan_register_battery_level_callback(lorawan_battery_level_cb_t cb);

/**
 * @brief Register a battery voltage callback function.
 *
 * Provide the LoRaWAN stack with a function to be called whenever a battery
 * voltage needs to be read.
 *
 *
 * @param cb Pointer to the battery voltage function
 */
void lorawan_register_battery_voltage_callback(lorawan_battery_voltage_cb_t cb);

/**
 * @brief Register a temperature callback function.
 *
 * Provide the LoRaWAN stack with a function to be called whenever the system
 * temperature needs to be read.
 *
 * @param cb Pointer to the temperature function
 */
void lorawan_register_temperature_callback(lorawan_temperature_cb_t cb);

#ifdef CONFIG_LORA_BASICS_MODEM_FUOTA
/**
 * @brief Register callbacks for the LoRaWAN Firmware Upgrade OTA feature.
 *
 * Calling this too late (after Device Management messages are received) might
 * create undefined behaviour. See documentation at:
 * https://resources.lora-alliance.org/technical-specifications/ts006-1-0-0-firmware-management-protocol
 *
 * @param[in] cb The callbacks to use for the hal implementation. All must be set.
 *
 */
void lorawan_register_fuota_callbacks(struct lorawan_fuota_cb *cb);

#endif /* CONFIG_LORA_BASICS_MODEM_FUOTA */

#ifdef CONFIG_LORA_BASICS_MODEM_USER_STORAGE_IMPL
/**
 * @brief Register the user storage implementation callbacks for the LoRaWAN stack..
 *
 * Calling this too late (after smtc_modem_init) might create undefined behaviour.
 *
 * @param[in] cb The callbacks to use for the hal implementation. All must be set.
 *
 */
void lorawan_register_user_storage_callbacks(struct lorawan_user_storage_cb *cb);

#endif /* CONFIG_LORA_BASICS_MODEM_USER_STORAGE_IMPL */

/**
 * @brief Re-initialization of the hal radio irq radio callbacks.
 *
 * This function must be called if after calling smtc_modem_suspend_before_user_radio_access()
 * function to directly access radio, user registers alternative radio irq functions with
 * <transceiver>_board_attach_interrupt(). To resume usage of defout irq callbacks:
 * prv_transceiver_event_cb(), this function must be called, before resuming usage of
 * smtc modem by calling smtc_modem_resume_after_user_radio_access(), otherwise
 * event handling will not work.
 */
void smtc_modem_hal_irq_reset_radio_irq(void);

/**
 * @brief Interruptible sleep that will exit when radio events happen.
 *
 */
void smtc_modem_hal_interruptible_msleep(k_timeout_t timeout);

/**
 * @brief If smtc_modem_hal_interruptible_msleep is running, interrupt it.
 *
 * This function is used to trigger the LoRa Basics Modem stack when an event occurs,
 * whether it is an interrupt from the transceiver, a timer or a user request.
 *
 */
void smtc_modem_hal_wake_up(void);

#ifdef __cplusplus
}
#endif

#endif /* SMTC_MODEM_HAL_INIT_H */
