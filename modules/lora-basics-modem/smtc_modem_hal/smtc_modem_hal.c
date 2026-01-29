/*
 * Copyright (c) 2025 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdarg.h>

#include <zephyr/device.h>
#include <zephyr/drivers/lora.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/random/random.h>

#include <lbm_common.h>
#include <smtc_modem_hal.h>
#include <smtc_modem_hal_ext.h>

LOG_MODULE_REGISTER(smtc_modem_hal, CONFIG_LORA_LOG_LEVEL);

#define HAL_WORKQ_STACK_SIZE 1024
#define HAL_WORKQ_PRIORITY (-1)

typedef void (*callback_t)(void *context);

struct cb_data_t {
	struct gpio_callback cb;
	struct k_work work;
	callback_t dio_cb;
	void *context;
};

struct timer_data_t {
	callback_t timer_cb;
	void *context;
};

static struct cb_data_t prv_cb_data;
static struct timer_data_t prv_timer_data;

static const struct device *prv_transceiver_dev;

static bool prv_modem_irq_enabled = true;
static bool prv_pending_timer_cb;
static bool prv_pending_dio_cb;

static K_THREAD_STACK_DEFINE(hal_workq_stack, HAL_WORKQ_STACK_SIZE);
static struct k_work_q hal_workq;

static void hal_timer_callback(struct k_timer *timer);
static K_TIMER_DEFINE(prv_timer, hal_timer_callback, NULL);

static void hal_irq_work_handler(struct k_work *work)
{
	struct cb_data_t *data = CONTAINER_OF(work, struct cb_data_t, work);

	if (!prv_modem_irq_enabled) {
		prv_pending_dio_cb = true;
		return;
	}

	if (data->dio_cb != NULL) {
		data->dio_cb(data->context);
	}
}

static void hal_irq_callback(const struct device *port, struct gpio_callback *cb, uint32_t pins)
{
	struct cb_data_t *data = CONTAINER_OF(cb, struct cb_data_t, cb);

	k_work_submit_to_queue(&hal_workq, &data->work);
}

static void hal_timer_callback(struct k_timer *timer)
{
	ARG_UNUSED(timer);

	if (!prv_modem_irq_enabled) {
		prv_pending_timer_cb = true;
		return;
	}

	if (prv_timer_data.timer_cb != NULL) {
		prv_timer_data.timer_cb(prv_timer_data.context);
	}
}

void smtc_modem_hal_init(const struct device *transceiver)
{
	__ASSERT(transceiver, "transceiver must be provided");
	__ASSERT(DEVICE_API_IS(lora, transceiver), "transceiver must be a LoRa device");

	prv_transceiver_dev = transceiver;

	k_work_queue_start(&hal_workq, hal_workq_stack,
			   K_THREAD_STACK_SIZEOF(hal_workq_stack),
			   HAL_WORKQ_PRIORITY, NULL);
	k_thread_name_set(&hal_workq.thread, "lbm_hal_workq");

	k_work_init(&prv_cb_data.work, hal_irq_work_handler);
}

/* -------------------------------------------------------------------------- */
/* --- TIME MANAGEMENT ------------------------------------------------------ */
/* -------------------------------------------------------------------------- */

/**
 * @brief Provide the time since startup in seconds.
 *
 * @remark Used for scheduling autonomous retransmissions (i.e: NbTrans),
 *         transmitting MAC answers, basically any delay without accurate time
 *         constraints. It is also used to measure the time spent inside the
 *         LoRaWAN process for the integrated failsafe.
 *
 * @return Current system uptime in seconds
 */
uint32_t smtc_modem_hal_get_time_in_s(void)
{
	return k_uptime_seconds();
}

/**
 * @brief Provide the time since startup in milliseconds.
 *
 * The returned value must monotonically increase all the way to 0xFFFFFFFF and
 * then overflow to 0x00000000.
 *
 * @return Current system uptime in milliseconds
 */
uint32_t smtc_modem_hal_get_time_in_ms(void)
{
	return k_uptime_get_32();
}

/* -------------------------------------------------------------------------- */
/* --- TIMER MANAGEMENT ----------------------------------------------------- */
/* -------------------------------------------------------------------------- */

/**
 * @brief Start a timer that will expire at the requested time.
 *
 * Upon expiration, the provided callback is called with context as its sole
 * argument. The callback is executed in an interrupt context, with interrupts
 * disabled.
 *
 * @param [in] milliseconds Timer duration in milliseconds
 * @param [in] callback     Callback function to be called when the timer expires
 * @param [in] context      Context to be passed to the callback function
 */
void smtc_modem_hal_start_timer(const uint32_t milliseconds, callback_t callback, void *context)
{
	prv_timer_data.timer_cb = callback;
	prv_timer_data.context = context;

	k_timer_start(&prv_timer, K_MSEC(milliseconds), K_NO_WAIT);
}

/**
 * @brief Stop the timer that may have been started with smtc_modem_hal_start_timer.
 */
void smtc_modem_hal_stop_timer(void)
{
	k_timer_stop(&prv_timer);
}

/* -------------------------------------------------------------------------- */
/* --- IRQ MANAGEMENT ------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

/**
 * @brief Disable the two interrupt sources that execute the LoRa Basics Modem
 *        code: the timer, and the transceiver DIO interrupt source.
 */
void smtc_modem_hal_disable_modem_irq(void)
{
	prv_modem_irq_enabled = false;
}

/**
 * @brief Enable the two interrupt sources that execute the LoRa Basics Modem
 *        code: the timer, and the transceiver DIO interrupt source.
 */
void smtc_modem_hal_enable_modem_irq(void)
{
	prv_modem_irq_enabled = true;

	if (prv_pending_timer_cb) {
		prv_pending_timer_cb = false;
		if (prv_timer_data.timer_cb != NULL) {
			prv_timer_data.timer_cb(prv_timer_data.context);
		}
	}

	if (prv_pending_dio_cb) {
		prv_pending_dio_cb = false;
		if (prv_cb_data.dio_cb != NULL) {
			prv_cb_data.dio_cb(prv_cb_data.context);
		}
	}
}

/* -------------------------------------------------------------------------- */
/* --- RANDOM NUMBER -------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

/**
 * @brief Return a uniformly-distributed unsigned random integer from the closed
 *        interval [val_1, val_2] or [val_2, val_1].
 *
 * @param [in] val_1 First boundary value
 * @param [in] val_2 Second boundary value
 *
 * @return Random value in the range [min(val_1,val_2), max(val_1,val_2)]
 */
uint32_t smtc_modem_hal_get_random_nb_in_range(const uint32_t val_1, const uint32_t val_2)
{
	uint32_t min = MIN(val_1, val_2);
	uint32_t range = MAX(val_1, val_2) - min;
	uint32_t random = sys_rand32_get();

	if (range == UINT32_MAX) {
		return random;
	}

	return min + (random % (range + 1));
}

/* -------------------------------------------------------------------------- */
/* --- RADIO ENVIRONMENT ---------------------------------------------------- */
/* -------------------------------------------------------------------------- */

/**
 * @brief Store the callback and context argument that must be executed when a
 *        radio event occurs.
 *
 * @param [in] callback Callback that will be called when a radio event occurs
 * @param [in] context  Context that will be passed to the callback
 */
void smtc_modem_hal_irq_config_radio_irq(callback_t dio_cb, void *context)
{
	int ret;

	__ASSERT(prv_transceiver_dev, "smtc_modem_hal_init must be called first");
	__ASSERT(dio_cb, "DIO1 callback must be provided");

	if (prv_cb_data.dio_cb != NULL) {
		ret = lbm_driver_remove_dio1_gpio_callback(prv_transceiver_dev, &prv_cb_data.cb);
		if (ret < 0) {
			LOG_ERR("Failed to remove DIO1 GPIO callback: %d", ret);
		}
	}

	prv_cb_data.dio_cb = dio_cb;
	prv_cb_data.context = context;

	ret = lbm_driver_add_dio1_gpio_callback(prv_transceiver_dev, &prv_cb_data.cb,
						hal_irq_callback);
	if (ret < 0) {
		LOG_ERR("Failed to add DIO1 GPIO callback: %d", ret);
	}
}

/**
 * @brief Power up the TCXO.
 *
 * If the TCXO is not controlled by the transceiver, power up the TCXO and then
 * busy wait until the TCXO is running with the proper accuracy. If the TCXO is
 * controlled by the transceiver or if no TCXO is present, implement an empty
 * function.
 */
void smtc_modem_hal_start_radio_tcxo(void)
{
	/*
	 * We only support TCXO's that are wired to the transceiver. In such cases,
	 * this function must be empty. See 5.25 of the porting guide.
	 */
}

/**
 * @brief Power down the TCXO.
 *
 * If the TCXO is not controlled by the transceiver, stop the TCXO. If the TCXO
 * is controlled by the transceiver or if no TCXO is present, implement an empty
 * function.
 */
void smtc_modem_hal_stop_radio_tcxo(void)
{
	/*
	 * We only support TCXO's that are wired to the transceiver. In such cases,
	 * this function must be empty. See 5.26 of the porting guide.
	 */
}

/**
 * @brief Return the time in milliseconds that the TCXO needs to start up with
 *        the required accuracy.
 *
 * This does not implement a delay but is used to perform certain calculations
 * so the modem accounts for startup latency when scheduling reception windows.
 * Return zero if no TCXO is deployed.
 *
 * @return TCXO startup delay in milliseconds
 */
uint32_t smtc_modem_hal_get_radio_tcxo_startup_delay_ms(void)
{
	return 0;
}

/**
 * @brief Set antenna switch for Tx operation or not.
 *
 * If no antenna switch is used then implement an empty command.
 *
 * @param [in] is_tx_on Set to true for Tx operation, false otherwise
 */
void smtc_modem_hal_set_ant_switch(bool is_tx_on)
{
	/* No antenna switch is used. */
}

/**
 * @brief Check if the radio is free.
 *
 * @remark Except a very specific application, this function should always
 *         return false. This function is used to check if the radio is used
 *         by an external stack.
 *
 * @remark Not implemented yet.
 *
 * @return false Radio is free (default)
 */
bool smtc_modem_external_stack_currently_use_radio(void)
{
	/* Not implemented yet */
	return false;
}

/* -------------------------------------------------------------------------- */
/* --- RESET MANAGEMENT ----------------------------------------------------- */
/* -------------------------------------------------------------------------- */

/**
 * @brief Resets the MCU.
 *
 * @remark Not implemented yet.
 */
void smtc_modem_hal_reset_mcu(void)
{
	/* Not implemented yet */
}

/* -------------------------------------------------------------------------- */
/* --- WATCHDOG MANAGEMENT -------------------------------------------------- */
/* -------------------------------------------------------------------------- */

/**
 * @brief Reloads watchdog counter.
 *
 * @remark Application has to call this function periodically. The call period
 *         must be less than WATCHDOG_RELOAD_PERIOD.
 *
 * @remark Not implemented yet.
 */
void smtc_modem_hal_reload_wdog(void)
{
	/* Not implemented yet */
}

/**
 * @brief Set an offset into the RTC counter.
 *
 * @remark Used for debug purpose such as wrapping issue.
 *
 * @remark Not implemented yet.
 *
 * @param [in] offset_to_test_wrapping Offset value to add
 */
void smtc_modem_hal_set_offset_to_test_wrapping(const uint32_t offset_to_test_wrapping)
{
	/* Not implemented yet */
}

/* -------------------------------------------------------------------------- */
/* --- CONTEXT SAVING MANAGEMENT -------------------------------------------- */
/* -------------------------------------------------------------------------- */

/**
 * @brief Restores the data context.
 *
 * @remark This function is used to restore Modem data from a non volatile memory.
 *
 * @remark Not implemented yet.
 *
 * @param [in]  ctx_type Type of modem context that need to be restored
 * @param [in]  offset   Memory offset after ctx_type address
 * @param [out] buffer   Buffer pointer to write to
 * @param [in]  size     Buffer size to read in bytes
 */
void smtc_modem_hal_context_restore(const modem_context_type_t ctx_type, uint32_t offset,
				    uint8_t *buffer, const uint32_t size)
{
	/* Not implemented yet */
}

/**
 * @brief Stores the data context.
 *
 * @remark This function is used to store Modem data in a non volatile memory.
 *
 * @remark Not implemented yet.
 *
 * @param [in] ctx_type Type of modem context that need to be saved
 * @param [in] offset   Memory offset after ctx_type address
 * @param [in] buffer   Buffer pointer to write from
 * @param [in] size     Buffer size to write in bytes
 */
void smtc_modem_hal_context_store(const modem_context_type_t ctx_type, uint32_t offset,
				  const uint8_t *buffer, const uint32_t size)
{
	/* Not implemented yet */
}

/**
 * @brief Erase a chosen number of flash pages of a context.
 *
 * @remark This function is only used with CONTEXT_STORE_AND_FORWARD.
 *
 * @remark Not implemented yet.
 *
 * @param [in] ctx_type Type of modem context that need to be erased
 * @param [in] offset   Memory offset after ctx_type address
 * @param [in] nb_page  Number of pages to erase
 */
void smtc_modem_hal_context_flash_pages_erase(const modem_context_type_t ctx_type,
					      uint32_t offset, uint8_t nb_page)
{
	/* Not implemented yet */
}

/* -------------------------------------------------------------------------- */
/* --- PANIC MANAGEMENT ----------------------------------------------------- */
/* -------------------------------------------------------------------------- */

/**
 * @brief Action to be taken in case of modem panic.
 *
 * @remark In case Device Management is used, it is recommended to perform the
 *         crashlog storage and status update in this function.
 *
 * @param [in] func The name of the function where the panic occurs
 * @param [in] line The line number where the panic occurs
 * @param [in] fmt  String format
 * @param [in] ...  String arguments
 */
void smtc_modem_hal_on_panic(uint8_t *func, uint32_t line, const char *fmt, ...)
{
	LOG_ERR("LBM panic: %s:%u", func, line);
	k_panic();
}

/* -------------------------------------------------------------------------- */
/* --- ENVIRONMENT MANAGEMENT ----------------------------------------------- */
/* -------------------------------------------------------------------------- */

/**
 * @brief Return the battery level.
 *
 * @remark According to LoRaWAN 1.0.4 spec:
 *         0: The end-device is connected to an external power source.
 *         1..254: Battery level, where 1 is the minimum and 254 is the maximum.
 *         255: The end-device was not able to measure the battery level.
 *
 * @remark Not implemented yet.
 *
 * @return Battery level for LoRaWAN stack (255 = not able to measure)
 */
uint8_t smtc_modem_hal_get_battery_level(void)
{
	/* Not implemented yet */
	return 255;
}

/**
 * @brief Return board wake up delay in milliseconds.
 *
 * @remark Not implemented yet.
 *
 * @return Board wake up delay in ms (0 = no delay)
 */
int8_t smtc_modem_hal_get_board_delay_ms(void)
{
	/* Not implemented yet */
	return 0;
}

/* -------------------------------------------------------------------------- */
/* --- TRACE MANAGEMENT ----------------------------------------------------- */
/* -------------------------------------------------------------------------- */

/**
 * @brief Prints debug trace.
 *
 * @remark Not implemented yet.
 *
 * @param [in] fmt  String format
 * @param [in] ...  String arguments
 */
void smtc_modem_hal_print_trace(const char *fmt, ...)
{
	/* Not implemented yet */
}

/* -------------------------------------------------------------------------- */
/* --- FUOTA MANAGEMENT ----------------------------------------------------- */
/* -------------------------------------------------------------------------- */

/**
 * @brief Get hardware version for FUOTA.
 *
 * @remark Only used if FMP package is activated.
 *
 * @remark Not implemented yet.
 *
 * @return Hardware version as defined in FMP Alliance package TS006-1.0.0
 */
uint32_t smtc_modem_hal_get_hw_version_for_fuota(void)
{
	/* Not implemented yet */
	return 0;
}

/**
 * @brief Get firmware version for FUOTA.
 *
 * @remark Only used if FMP package is activated.
 *
 * @remark Not implemented yet.
 *
 * @return Firmware version as defined in FMP Alliance package TS006-1.0.0
 */
uint32_t smtc_modem_hal_get_fw_version_for_fuota(void)
{
	/* Not implemented yet */
	return 0;
}

/**
 * @brief Get firmware status available for FUOTA.
 *
 * @remark Only used if FMP package is activated.
 *
 * @remark Not implemented yet.
 *
 * @return Firmware status field as defined in FMP Alliance package TS006-1.0.0
 */
uint8_t smtc_modem_hal_get_fw_status_available_for_fuota(void)
{
	/* Not implemented yet */
	return 0;
}

/**
 * @brief Get firmware delete status for FUOTA.
 *
 * @remark Only used if FMP package is activated.
 *
 * @remark Not implemented yet.
 *
 * @param [in] fw_to_delete_version Firmware version to delete
 *
 * @return Firmware status field as defined in FMP Alliance package TS006-1.0.0
 */
uint8_t smtc_modem_hal_get_fw_delete_status_for_fuota(uint32_t fw_to_delete_version)
{
	/* Not implemented yet */
	return 0;
}

/**
 * @brief Get next firmware version for FUOTA.
 *
 * @remark Only used if FMP package is activated.
 *
 * @remark Not implemented yet.
 *
 * @return Firmware version that will be running once upgrade is installed
 */
uint32_t smtc_modem_hal_get_next_fw_version_for_fuota(void)
{
	/* Not implemented yet */
	return 0;
}

/* -------------------------------------------------------------------------- */
/* --- DEVICE MANAGEMENT ---------------------------------------------------- */
/* -------------------------------------------------------------------------- */

/**
 * @brief Return temperature in Celsius.
 *
 * @remark Not implemented yet.
 *
 * @return Temperature in Celsius (0 = dummy value)
 */
int8_t smtc_modem_hal_get_temperature(void)
{
	/* Not implemented yet */
	return 0;
}

/**
 * @brief Return MCU voltage in millivolts.
 *
 * @remark Not implemented yet.
 *
 * @return MCU voltage in mV (0 = dummy value)
 */
uint16_t smtc_modem_hal_get_voltage_mv(void)
{
	/* Not implemented yet */
	return 0;
}

/**
 * @brief Stores the crashlog.
 *
 * @remark This function is used to store the Modem crashlog in a non volatile
 *         memory.
 *
 * @remark Not implemented yet.
 *
 * @param [in] crash_string        Crashlog string to be stored
 * @param [in] crash_string_length Crashlog string length
 */
void smtc_modem_hal_crashlog_store(const uint8_t *crash_string, uint8_t crash_string_length)
{
	/* Not implemented yet */
}

/**
 * @brief Restores the crashlog.
 *
 * @remark This function is used to restore the Modem crashlog from a non
 *         volatile memory.
 *
 * @remark Not implemented yet.
 *
 * @param [out] crash_string        Crashlog string to be restored
 * @param [out] crash_string_length Crashlog string length
 */
void smtc_modem_hal_crashlog_restore(uint8_t *crash_string, uint8_t *crash_string_length)
{
	/* Not implemented yet */
}

/**
 * @brief Stores the crashlog status.
 *
 * @remark This function is used to store the Modem crashlog status in a non
 *         volatile memory. This status will allow the Modem to handle crashlog
 *         send task if needed after a crash.
 *
 * @remark Not implemented yet.
 *
 * @param [in] available True if a crashlog is available, false otherwise
 */
void smtc_modem_hal_crashlog_set_status(bool available)
{
	/* Not implemented yet */
}

/**
 * @brief Get the previously stored crashlog status.
 *
 * @remark This function is used to get the Modem crashlog status from a non
 *         volatile memory. This status will allow the Modem to handle crashlog
 *         send task if needed after a crash.
 *
 * @remark Not implemented yet.
 *
 * @return false No crashlog available (default)
 */
bool smtc_modem_hal_crashlog_get_status(void)
{
	/* Not implemented yet */
	return false;
}

/* -------------------------------------------------------------------------- */
/* --- STORE AND FORWARD ---------------------------------------------------- */
/* -------------------------------------------------------------------------- */

/**
 * @brief The number of reserved pages in flash for the Store and Forward service.
 *
 * @remark The number must be at least 3 pages.
 *
 * @remark Not implemented yet.
 *
 * @return Number of flash pages (0 = dummy value)
 */
uint16_t smtc_modem_hal_store_and_forward_get_number_of_pages(void)
{
	/* Not implemented yet */
	return 0;
}

/**
 * @brief Gives the size of a flash page in bytes.
 *
 * @remark Not implemented yet.
 *
 * @return Flash page size in bytes (0 = dummy value)
 */
uint16_t smtc_modem_hal_flash_get_page_size(void)
{
	/* Not implemented yet */
	return 0;
}

/* -------------------------------------------------------------------------- */
/* --- RTOS COMPATIBILITY --------------------------------------------------- */
/* -------------------------------------------------------------------------- */

/**
 * @brief This function is called by the LBM stack on each LBM interruption
 *        (radio interrupt or low-power timer interrupt).
 *
 * @remark It could be convenient in the case of an RTOS implementation to
 *         notify the thread that manages the LBM stack.
 *
 * @remark Not implemented yet.
 */
void smtc_modem_hal_user_lbm_irq(void)
{
	/* Not implemented yet */
}
