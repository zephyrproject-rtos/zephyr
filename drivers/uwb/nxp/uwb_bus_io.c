/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <uwb_bus_interface.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(nxp_bus_io, CONFIG_UWB_NXP_LOG_LEVEL);

#define UWB_NUM_INTERRUPTS 1
#define UWB_NODE           DT_COMPAT_GET_ANY_STATUS_OKAY(nxp_uwb_device)

static struct gpio_dt_spec irq_gpio = GPIO_DT_SPEC_GET(UWB_NODE, uwb_irq_gpios);
static struct gpio_dt_spec rstn_gpio = GPIO_DT_SPEC_GET(UWB_NODE, uwb_rstn_gpios);

static struct gpio_callback irq_gpio_cb;
static uwbs_io_callback mCallbacks[UWB_NUM_INTERRUPTS];

static void uwb_bus_io_generic_cb()
{
	/* Handle more callbacks if needed */
	if (mCallbacks[0].fn) {
		mCallbacks[0].fn(mCallbacks[0].args);
	}
}

void UWB_HELIOS_IRQ_HANDLER(const struct device *unused1, struct gpio_callback *unused2,
			    uint32_t unused3)
{
	uwb_bus_io_generic_cb();
}

void uwb_bus_io_irq_cb(void *args)
{
	uwb_bus_board_ctx_t *pCtx = (uwb_bus_board_ctx_t *)args;
	// Signal TML read task
	k_sem_give(&pCtx->mIrqWaitSem);
}

uwb_bus_status_t uwb_bus_io_deinit(uwb_bus_board_ctx_t *pCtx)
{
	memset(mCallbacks, 0, sizeof(uwbs_io_callback) * UWB_NUM_INTERRUPTS);
	return kUWB_bus_Status_OK;
}

uwb_bus_status_t uwb_bus_io_init(uwb_bus_board_ctx_t *pCtx)
{
	uwb_bus_status_t status = kUWB_bus_Status_FAILED;
	int ret = 0;
	pCtx->irq_gpio = &irq_gpio;
	pCtx->rstn_gpio = &rstn_gpio;

	/* Configure RSTN pin as output */
	if (!gpio_is_ready_dt(pCtx->rstn_gpio)) {
		LOG_ERR("RSTN GPIO is not ready");
		return kUWB_bus_Status_FAILED;
	}

	ret = gpio_pin_configure_dt(pCtx->rstn_gpio, GPIO_OUTPUT_INACTIVE);
	if (ret != 0) {
		LOG_ERR("RSTN GPIO Pin configuration failed");
		return kUWB_bus_Status_FAILED;
	}

	status = uwb_bus_io_uwbs_irq_enable(pCtx);

	return status;
}

uwb_bus_status_t uwb_bus_io_val_set(uwb_bus_board_ctx_t *pCtx, uwbs_io_t gpioPin,
				    uwbs_io_state_t gpioValue)
{
	int ret = 0;

	if (pCtx == NULL) {
		LOG_ERR("uwbs bus context is NULL");
		return kUWB_bus_Status_FAILED;
	}
	if (kUWBS_IO_State_NA == gpioValue) {
		LOG_ERR("Invalid GPIO state");
		return kUWB_bus_Status_FAILED;
	}

	switch (gpioPin) {
	case kUWBS_IO_O_RSTN: {
		ret = gpio_pin_set_dt(pCtx->rstn_gpio, gpioValue);
		if (ret != 0) {
			LOG_ERR("RSTN GPIO Pin set failed");
			return kUWB_bus_Status_FAILED;
		}
		return kUWB_bus_Status_OK;
	} break;

	default:
		LOG_ERR("UWBD IO GPIO Pin not supported");
		return kUWB_bus_Status_FAILED;
	}
}

uwb_bus_status_t uwb_bus_io_val_get(uwb_bus_board_ctx_t *pCtx, uwbs_io_t gpioPin,
				    uwbs_io_state_t *pGpioValue)
{
	if (pCtx == NULL || pGpioValue == NULL) {
		LOG_ERR("uwbs bus context is NULL");
		return kUWB_bus_Status_FAILED;
	}
	switch (gpioPin) {
	case kUWBS_IO_I_UWBS_IRQ: {
		*pGpioValue = (uwbs_io_state_t)gpio_pin_get_dt(pCtx->irq_gpio);
	} break;

	default:
		LOG_ERR("UWBD IO GPIO Pin not supported");
		return kUWB_bus_Status_FAILED;
	}
	return kUWB_bus_Status_OK;
}

uwb_bus_status_t uwb_bus_io_irq_dis(uwb_bus_board_ctx_t *pCtx, uwbs_io_t irqPin)
{
	uwb_bus_status_t status = kUWB_bus_Status_FAILED;

	if (pCtx == NULL) {
		LOG_ERR("uwbs bus context is NULL");
		return status;
	}

	switch (irqPin) {
	case kUWBS_IO_I_UWBS_IRQ: {
	} break;
	default:
		LOG_ERR("UWBD IO GPIO Pin Interrupt not supported");
		goto end;
	}

	mCallbacks[0].fn = NULL;
	mCallbacks[0].args = NULL;
	status = kUWB_bus_Status_OK;
end:
	return status;
}

uwb_bus_status_t uwb_bus_io_irq_en(uwb_bus_board_ctx_t *pCtx, uwbs_io_t irqPin,
				   uwbs_io_callback *pCallback)
{
	uwb_bus_status_t status = kUWB_bus_Status_FAILED;
	int ret = 0;

	if (pCtx == NULL || pCallback == NULL) {
		LOG_ERR("uwbs bus context is NULL");
		return status;
	}

	switch (irqPin) {
	case kUWBS_IO_I_UWBS_IRQ: {
		mCallbacks[0].fn = pCallback->fn;
		mCallbacks[0].args = pCallback->args;
	} break;
	default:
		LOG_ERR("UWBD IO GPIO Pin Interrupt not supported");
		goto end;
	}

	if (!gpio_is_ready_dt(pCtx->irq_gpio)) {
		LOG_ERR("UWBD IO GPIO is not ready");
		goto end;
	}

	/* Configure IRQ pin and register the callback */
	ret = gpio_pin_configure_dt(pCtx->irq_gpio, GPIO_INPUT);
	if (ret != 0) {
		LOG_ERR("UWBD IO GPIO Pin configuration failed");
		goto end;
	}
	/* Configure the interrupt edge for IRQ pin */
	ret = gpio_pin_interrupt_configure_dt(pCtx->irq_gpio, GPIO_INT_EDGE_RISING);
	if (ret != 0) {
		LOG_ERR("UWBD IO GPIO Pin interrupt configuration failed");
		goto end;
	}

	gpio_init_callback(&irq_gpio_cb, UWB_HELIOS_IRQ_HANDLER, BIT(pCtx->irq_gpio->pin));
	gpio_add_callback(pCtx->irq_gpio->port, &irq_gpio_cb);

	status = kUWB_bus_Status_OK;
end:
	return status;
}

uwb_bus_status_t uwb_bus_io_irq_wait(uwb_bus_board_ctx_t *pCtx, uint32_t timeout_ms)
{
	uwb_bus_status_t status = kUWB_bus_Status_OK;

	if (pCtx == NULL) {
		LOG_ERR("uwbs bus context is NULL");
		return kUWB_bus_Status_FAILED;
	}

	k_timeout_t timeout = {.ticks = timeout_ms};
	if (k_sem_take(&pCtx->mIrqWaitSem, timeout) != 0) {
		LOG_DBG("k_sem_take failed");
		status = kUWB_bus_Status_FAILED;
	}

	return status;
}

uwb_bus_status_t uwb_bus_io_uwbs_irq_enable(uwb_bus_board_ctx_t *pCtx)
{
	uwbs_io_callback callback;
	if (pCtx == NULL) {
		LOG_ERR("uwbs bus context is NULL");
		return kUWB_bus_Status_FAILED;
	}
	callback.fn = uwb_bus_io_irq_cb;
	callback.args = pCtx;
	return uwb_bus_io_irq_en(pCtx, kUWBS_IO_I_UWBS_IRQ, &callback);
}
