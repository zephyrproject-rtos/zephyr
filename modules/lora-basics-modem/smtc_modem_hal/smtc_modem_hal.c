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
