/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

#include <openthread/error.h>
#include <openthread/platform/alarm-milli.h>
#include <openthread/platform/diag.h>
#include <openthread/platform/radio.h>

#include "platform-zephyr.h"
#include "zephyr/sys/util.h"

enum {
	DIAG_TRANSMIT_MODE_IDLE,
	DIAG_TRANSMIT_MODE_PACKETS,
	DIAG_TRANSMIT_MODE_CARRIER,
	DIAG_TRANSMIT_MODE_MODCARRIER

} diag_trasmit_mode;

/**
 * Diagnostics mode variables.
 *
 */

static bool sDiagMode;
static void *sDiagCallbackContext;
static otPlatDiagOutputCallback sDiagOutputCallback;
static uint8_t sTransmitMode = DIAG_TRANSMIT_MODE_IDLE;
static uint8_t sChannel = 20;
static uint32_t sTxPeriod = 1;
static int32_t sTxCount;
static int32_t sTxRequestedCount = 1;

static otError startModCarrier(otInstance *aInstance, uint8_t aArgsLength, char *aArgs[]);
static otError processTransmit(otInstance *aInstance, uint8_t aArgsLength, char *aArgs[]);

static otError parse_long(char *aArgs, long *aValue)
{
	char *endptr;
	*aValue = strtol(aArgs, &endptr, 0);
	return (*endptr == '\0') ? OT_ERROR_NONE : OT_ERROR_PARSE;
}

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
#if defined(CONFIG_IEEE802154_CARRIER_FUNCTIONS)
	if (strcmp(aArgs[0], "modcarrier") == 0) {
		return startModCarrier(aInstance, aArgsLength - 1, aArgs + 1);
	}
#endif

	if (strcmp(aArgs[0], "transmit") == 0) {
		return processTransmit(aInstance, aArgsLength - 1, aArgs + 1);
	}

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
	sChannel = aChannel;
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

#if defined(CONFIG_IEEE802154_CARRIER_FUNCTIONS)
otError otPlatDiagRadioTransmitCarrier(otInstance *aInstance, bool aEnable)
{
	if (!otPlatDiagModeGet() || (sTransmitMode != DIAG_TRANSMIT_MODE_IDLE &&
				     sTransmitMode != DIAG_TRANSMIT_MODE_CARRIER)) {
		return OT_ERROR_INVALID_STATE;
	}

	if (aEnable) {
		sTransmitMode = DIAG_TRANSMIT_MODE_CARRIER;
	} else {
		sTransmitMode = DIAG_TRANSMIT_MODE_IDLE;
	}

	return platformRadioTransmitCarrier(aInstance, aEnable);
}
#endif /* CONFIG_IEEE802154_CARRIER_FUNCTIONS */

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

#if defined(CONFIG_IEEE802154_CARRIER_FUNCTIONS)

static otError startModCarrier(otInstance *aInstance, uint8_t aArgsLength, char *aArgs[])
{
	bool enable = true;
	uint8_t data[OT_RADIO_FRAME_MAX_SIZE + 1];

	if (aArgsLength <= 0) {
		return OT_ERROR_INVALID_ARGS;
	}

	if (!otPlatDiagModeGet() || (sTransmitMode != DIAG_TRANSMIT_MODE_IDLE &&
				     sTransmitMode != DIAG_TRANSMIT_MODE_MODCARRIER)) {
		return OT_ERROR_INVALID_STATE;
	}

	if (strcmp(aArgs[0], "stop") == 0) {
		enable = false;
		sTransmitMode = DIAG_TRANSMIT_MODE_IDLE;
	} else {
		if (hex2bin(aArgs[0], strlen(aArgs[0]), data, ARRAY_SIZE(data)) == 0) {
			return OT_ERROR_INVALID_ARGS;
		}
		sTransmitMode = DIAG_TRANSMIT_MODE_MODCARRIER;
	}

	return platformRadioTransmitModulatedCarrier(aInstance, enable, data);
}

#endif

void otPlatDiagAlarmCallback(otInstance *aInstance)
{
	uint32_t now;
	otRadioFrame *txPacket;
	const uint16_t diag_packet_len = 30;

	if (sTransmitMode == DIAG_TRANSMIT_MODE_PACKETS) {
		if ((sTxCount > 0) || (sTxCount == -1)) {
			txPacket = otPlatRadioGetTransmitBuffer(aInstance);

			txPacket->mInfo.mTxInfo.mTxDelayBaseTime = 0;
			txPacket->mInfo.mTxInfo.mTxDelay = 0;
			txPacket->mInfo.mTxInfo.mMaxCsmaBackoffs = 0;
			txPacket->mInfo.mTxInfo.mMaxFrameRetries = 0;
			txPacket->mInfo.mTxInfo.mRxChannelAfterTxDone = sChannel;
			txPacket->mInfo.mTxInfo.mTxPower = OT_RADIO_POWER_INVALID;
			txPacket->mInfo.mTxInfo.mIsHeaderUpdated = false;
			txPacket->mInfo.mTxInfo.mIsARetx = false;
			txPacket->mInfo.mTxInfo.mCsmaCaEnabled = false;
			txPacket->mInfo.mTxInfo.mCslPresent = false;
			txPacket->mInfo.mTxInfo.mIsSecurityProcessed = false;

			txPacket->mLength = diag_packet_len;

			for (uint8_t i = 0; i < diag_packet_len; i++) {
				txPacket->mPsdu[i] = i;
			}

			otPlatRadioTransmit(aInstance, txPacket);

			if (sTxCount != -1) {
				sTxCount--;
			}

			now = otPlatAlarmMilliGetNow();
			otPlatAlarmMilliStartAt(aInstance, now, sTxPeriod);
		} else {
			sTransmitMode = DIAG_TRANSMIT_MODE_IDLE;
			otPlatAlarmMilliStop(aInstance);
			otPlatLog(OT_LOG_LEVEL_DEBG, OT_LOG_REGION_PLATFORM, "Transmit done");
		}
	}
}

static otError processTransmit(otInstance *aInstance, uint8_t aArgsLength, char *aArgs[])
{
	otError error = OT_ERROR_NONE;
	long value;
	uint32_t now;

	if (!otPlatDiagModeGet()) {
		return OT_ERROR_INVALID_STATE;
	}

	if (aArgsLength == 0) {
		diag_output("transmit will send %" PRId32 " diagnostic messages with %" PRIu32
			    " ms interval\r\n",
			    sTxRequestedCount, sTxPeriod);

	} else if (strcmp(aArgs[0], "stop") == 0) {
		if (sTransmitMode == DIAG_TRANSMIT_MODE_IDLE) {
			return OT_ERROR_INVALID_STATE;
		}

		otPlatAlarmMilliStop(aInstance);
		diag_output("diagnostic message transmission is stopped\r\n");
		sTransmitMode = DIAG_TRANSMIT_MODE_IDLE;
		otPlatRadioReceive(aInstance, sChannel);

	} else if (strcmp(aArgs[0], "start") == 0) {
		if (sTransmitMode != DIAG_TRANSMIT_MODE_IDLE) {
			return OT_ERROR_INVALID_STATE;
		}

		otPlatAlarmMilliStop(aInstance);
		sTransmitMode = DIAG_TRANSMIT_MODE_PACKETS;
		sTxCount = sTxRequestedCount;
		now = otPlatAlarmMilliGetNow();
		otPlatAlarmMilliStartAt(aInstance, now, sTxPeriod);
		diag_output("sending %" PRId32 " diagnostic messages with %" PRIu32
			    " ms interval\r\n",
			    sTxRequestedCount, sTxPeriod);
	} else if (strcmp(aArgs[0], "interval") == 0) {

		if (aArgsLength != 2) {
			return OT_ERROR_INVALID_ARGS;
		}

		error = parse_long(aArgs[1], &value);
		if (error != OT_ERROR_NONE) {
			return error;
		}

		if (value <= 0) {
			return OT_ERROR_INVALID_ARGS;
		}
		sTxPeriod = (uint32_t)(value);
		diag_output("set diagnostic messages interval to %" PRIu32
			    " ms\r\n", sTxPeriod);

	} else if (strcmp(aArgs[0], "count") == 0) {

		if (aArgsLength != 2) {
			return OT_ERROR_INVALID_ARGS;
		}

		error = parse_long(aArgs[1], &value);
		if (error != OT_ERROR_NONE) {
			return error;
		}

		if ((value <= 0) && (value != -1)) {
			return OT_ERROR_INVALID_ARGS;
		}

		sTxRequestedCount = (uint32_t)(value);
		diag_output("set diagnostic messages count to %" PRId32 "\r\n",
			    sTxRequestedCount);
	} else {
		return OT_ERROR_INVALID_ARGS;
	}

	return error;
}
