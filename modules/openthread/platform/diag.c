/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

#include <openthread/platform/diag.h>

#include "platform-zephyr.h"

/**
 * Diagnostics mode variables.
 *
 */
static bool sDiagMode;
static void *sDiagCallbackContext;
static otPlatDiagOutputCallback sDiagOutputCallback;

static void diag_output(const char *aFormat, ...)
{
	va_list args;

	va_start(args, aFormat);

	if (sDiagOutputCallback != NULL) {
		sDiagOutputCallback(aFormat, args, sDiagCallbackContext);
	}

	va_end(args);
}

void otPlatDiagSetOutputCallback(otInstance *aInstance,
				 otPlatDiagOutputCallback aCallback,
				 void *aContext)
{
	OT_UNUSED_VARIABLE(aInstance);

	sDiagOutputCallback  = aCallback;
	sDiagCallbackContext = aContext;
}

otError otPlatDiagProcess(otInstance *aInstance, uint8_t aArgsLength, char *aArgs[])
{
	ARG_UNUSED(aInstance);
	ARG_UNUSED(aArgsLength);

	/* Add more platform specific diagnostics features here. */
	diag_output("diag feature '%s' is not supported\r\n", aArgs[0]);

	return OT_ERROR_NOT_IMPLEMENTED;
}

void otPlatDiagModeSet(bool aMode)
{
	sDiagMode = aMode;

	if (!sDiagMode) {
		otPlatRadioSleep(NULL);
	}
}

bool otPlatDiagModeGet(void)
{
	return sDiagMode;
}

void otPlatDiagChannelSet(uint8_t aChannel)
{
	ARG_UNUSED(aChannel);
}

void otPlatDiagTxPowerSet(int8_t aTxPower)
{
	ARG_UNUSED(aTxPower);
}

void otPlatDiagRadioReceived(otInstance *aInstance,
			     otRadioFrame *aFrame,
			     otError aError)
{
	ARG_UNUSED(aInstance);
	ARG_UNUSED(aFrame);
	ARG_UNUSED(aError);
}

otError otPlatDiagRadioTransmitCarrier(otInstance *aInstance, bool aEnable)
{
	if (!otPlatDiagModeGet()) {
		return OT_ERROR_INVALID_STATE;
	}

	return platformRadioTransmitCarrier(aInstance, aEnable);
}

void otPlatDiagAlarmCallback(otInstance *aInstance)
{
	ARG_UNUSED(aInstance);
}

/*
 * To enable gpio diag commands, in Devicetree create `openthread` node in `/options/` path
 * with `compatible = "openthread,config"` property and `diag-gpios` property,
 * which should contain array of GPIO pin's configuration properties containing controller phandles,
 * pin numbers and pin flags. e.g:
 *
 * options {
 *	openthread {
 *		compatible = "openthread,config";
 *		diag-gpios = <&gpio0 0 GPIO_ACTIVE_HIGH>,
 *			     <&gpio1 0 GPIO_ACTIVE_LOW>;
 *	};
 * };
 *
 * To enable reading current gpio pin mode, define
 * `CONFIG_GPIO_GET_DIRECTION` in prj.conf.
 *
 * Note: `<gpio>` in `diag gpio` commands is an index of diag-gpios array. For example shown above,
 * `ot diag gpio mode 0` will return current mode of pin nmb 0 controlled by `gpio0` controller.
 */
#if DT_HAS_COMPAT_STATUS_OKAY(openthread_config) && \
	DT_NODE_HAS_PROP(DT_COMPAT_GET_ANY_STATUS_OKAY(openthread_config), diag_gpios)

static const struct gpio_dt_spec gpio_spec[] = {
	DT_FOREACH_PROP_ELEM_SEP(DT_COMPAT_GET_ANY_STATUS_OKAY(openthread_config),
				 diag_gpios, GPIO_DT_SPEC_GET_BY_IDX, (,))};

static otError gpio_get_spec(uint32_t gpio_idx, const struct gpio_dt_spec **spec)
{
	if (gpio_idx >= ARRAY_SIZE(gpio_spec)) {
		return OT_ERROR_INVALID_ARGS;
	}

	*spec = &gpio_spec[gpio_idx];

	if (!otPlatDiagModeGet()) {
		return OT_ERROR_INVALID_STATE;
	}

	if (!gpio_is_ready_dt(*spec)) {
		return OT_ERROR_INVALID_ARGS;
	}

	const struct gpio_driver_config *const cfg =
		(const struct gpio_driver_config *)((*spec)->port->config);

	if ((cfg->port_pin_mask & (gpio_port_pins_t)BIT((*spec)->pin)) == 0U) {
		return OT_ERROR_INVALID_ARGS;
	}

	return OT_ERROR_NONE;
}

otError otPlatDiagGpioSet(uint32_t aGpio, bool aValue)
{
	const struct gpio_dt_spec *spec;
	otError error;

	error = gpio_get_spec(aGpio, &spec);

	if (error != OT_ERROR_NONE) {
		return error;
	}

#if defined(CONFIG_GPIO_GET_DIRECTION)
	if (gpio_pin_is_output_dt(spec) != 1) {
		return OT_ERROR_INVALID_STATE;
	}
#endif

	if (gpio_pin_set_dt(spec, (int)aValue) != 0) {
		return OT_ERROR_FAILED;
	}

	return OT_ERROR_NONE;
}

otError otPlatDiagGpioGet(uint32_t aGpio, bool *aValue)
{
	const struct gpio_dt_spec *spec;
	otError error;
	int rv;

	error = gpio_get_spec(aGpio, &spec);

	if (error != OT_ERROR_NONE) {
		return error;
	}

	if (aValue == NULL) {
		return OT_ERROR_INVALID_ARGS;
	}

#if defined(CONFIG_GPIO_GET_DIRECTION)
	if (gpio_pin_is_input_dt(spec) != 1) {
		return OT_ERROR_INVALID_STATE;
	}
#endif

	rv = gpio_pin_get_dt(spec);
	if (rv < 0) {
		return OT_ERROR_FAILED;
	}
	*aValue = (bool)rv;

	return OT_ERROR_NONE;
}

otError otPlatDiagGpioSetMode(uint32_t aGpio, otGpioMode aMode)
{
	const struct gpio_dt_spec *spec;
	otError error;
	int rv = 0;

	error = gpio_get_spec(aGpio, &spec);

	if (error != OT_ERROR_NONE) {
		return error;
	}

	switch (aMode) {
	case OT_GPIO_MODE_INPUT:
		rv = gpio_pin_configure_dt(spec, GPIO_INPUT);
		break;

	case OT_GPIO_MODE_OUTPUT:
		rv = gpio_pin_configure_dt(spec, GPIO_OUTPUT);
		break;

	default:
		return OT_ERROR_INVALID_ARGS;
	}

	if (rv != 0) {
		return OT_ERROR_FAILED;
	}

	return OT_ERROR_NONE;
}

#if defined(CONFIG_GPIO_GET_DIRECTION)
otError otPlatDiagGpioGetMode(uint32_t aGpio, otGpioMode *aMode)
{
	const struct gpio_dt_spec *spec;
	otError error;
	gpio_port_pins_t pins_in, pins_out;

	error = gpio_get_spec(aGpio, &spec);

	if (error != OT_ERROR_NONE) {
		return error;
	}
	if (aMode == NULL) {
		return OT_ERROR_INVALID_ARGS;
	}

	if (gpio_port_get_direction(spec->port, BIT(spec->pin), &pins_in, &pins_out) < 0) {
		return OT_ERROR_FAILED;
	}

	if (((gpio_port_pins_t)BIT(spec->pin) & pins_in) != 0U) {
		*aMode = OT_GPIO_MODE_INPUT;
	} else if (((gpio_port_pins_t)BIT(spec->pin) & pins_out) != 0U) {
		*aMode = OT_GPIO_MODE_OUTPUT;
	} else {
		return OT_ERROR_FAILED;
	}

	return OT_ERROR_NONE;
}
#endif /* CONFIG_GPIO_GET_DIRECTION */
#endif /* DT_HAS_COMPAT_STATUS_OKAY(openthread_config) && \
	* DT_NODE_HAS_PROP(DT_COMPAT_GET_ANY_STATUS_OKAY(openthread_config), diag_gpios)
	*/
