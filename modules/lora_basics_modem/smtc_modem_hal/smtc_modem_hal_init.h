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

struct smtc_modem_hal_cb {
	/**
	 * @brief Get battery level callback
	 *
	 * @param [out] value The battery level (in permille, where 1000‰ means full battery).
	 * The application should set this to the most recent battery level available.
	 *
	 * @return int 0 if the battery level was set, or a negative error code if
	 * no valid battery level is available
	 */
	int (*get_battery_level)(uint32_t *value);

	/**
	 * @brief Get temperature callback
	 *
	 * @param [out] value The temperature (in °C).
	 * The application should set this to the most recent temperature value available.
	 *
	 * @return int 0 if the temperature was set, or a negative error code if
	 * no valid temperature is available
	 */
	int (*get_temperature)(int32_t *value);

	/**
	 * @brief Get voltage level callback
	 *
	 * @param [out] value The voltage level (in mV).
	 * The application should set this to the most recent voltage level available.
	 *
	 * @return int 0 if the voltage level was set, or a negative error code if
	 * no valid voltage level is available
	 */
	int (*get_voltage)(uint32_t *value);

#ifdef CONFIG_LORA_BASICS_MODEM_FUOTA

	/**
	 * @brief Get hardware version callback
	 *
	 * @param [out] version The devices hardware version.
	 *
	 * @return int 0 if the hardware version was set, or a negative error code if no valid
	 * hardware was available.
	 */
	int (*get_hw_version_for_fuota)(uint32_t *version);

	/**
	 * @brief Get firmware version callback. Only use if fmp package is activated.
	 *
	 * @param [out] version The devices firmware version as described in TS006-1.0.0.
	 *
	 * @return int 0 if the firmware version was set, or a negative error code if no valid
	 * firmware was available.
	 */
	int (*get_fw_version_for_fuota)(uint32_t *version);

	/**
	 * @brief Get firmware status callback. Only use if fmp package is activated.
	 *
	 * @param [out] status The devices firmware status as described in TS006-1.0.0.
	 *
	 * @return int 0 if the firmware status was set, or a negative error code if no valid
	 * firmware status was available.
	 */
	int (*get_fw_status_available_for_fuota)(uint8_t *status);

	/**
	 * @brief Get next firmware version callback.
	 *
	 * @param [out] next_version The devices next firmware version.
	 *
	 * @return int 0 if the next firmware version was set, or a negative error code if no valid
	 * next firmware version was available.
	 */
	int (*get_next_fw_version_for_fuota)(uint32_t *next_version);

	/**
	 * @brief Get firmware delete status callback.
	 *
	 * @param [in] fw_to_delete_version The firmware version to delete.
	 * @param [out] status The devices firmware delete status.
	 *
	 * @return int 0 if the firmware was deleted successfully, or a negative error code
	 * otherwise.
	 */
	int (*get_fw_delete_status_for_fuota)(uint32_t fw_to_delete_version);
#endif

#ifdef CONFIG_LORA_BASICS_MODEM_USER_STORAGE_IMPL

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
#endif /* CONFIG_LORA_BASICS_MODEM_USER_STORAGE_IMPL */
};

/**
 * @brief Initialization of the hal implementation.
 *
 * This must be called before smtc_modem_init
 *
 * @param[in] transceiver The device pointer of the transceiver instance that will be used.
 */
void smtc_modem_hal_init(const struct device *transceiver);

/**
 * @brief Register the applicative callbacks for the LoRaWAN stack.
 *
 * Calling this too late (after Device Management messages are received) might
 * create undefined behaviour.
 *
 * @param[in] hal_cb The callbacks to use for the hal implementation. All must be set.
 *
 */
void smtc_modem_hal_register_callbacks(struct smtc_modem_hal_cb *hal_cb);

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
