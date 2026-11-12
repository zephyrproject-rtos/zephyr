/*
 * Copyright (c) 2026 TeraTron GmbH
 */

#include <errno.h>
#include <string.h>
#include <sample_usbd.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/usb/class/usbd_ccid.h>

LOG_MODULE_REGISTER(ccid_sample, LOG_LEVEL_INF);

struct sample_card {
	struct usbd_ccid_backend backend;
	atomic_t powered;
};

static struct sample_card card;
static const uint32_t clocks_khz[] = {4000};
static const uint32_t rates_bps[] = {10752};
static const struct usbd_ccid_t0_parameters t0_parameters = USBD_CCID_T0_PARAMETERS_DEFAULT;
static const struct usbd_ccid_reader_config reader_config = {
	.protocols = USBD_CCID_PROTOCOL_MASK(USBD_CCID_PROTOCOL_T0),
	.default_protocol = USBD_CCID_PROTOCOL_T0,
	.features = USBD_CCID_AUTO_PARAMETERS | USBD_CCID_AUTO_NEGOTIATION |
		    USBD_CCID_AUTO_VOLTAGE | USBD_CCID_SHORT_APDU,
	.voltage_support = USBD_CCID_VOLTAGE_MASK(USBD_CCID_VOLTAGE_3V),
	.clock_frequencies = clocks_khz,
	.clock_count = ARRAY_SIZE(clocks_khz),
	.data_rates = rates_bps,
	.data_rate_count = ARRAY_SIZE(rates_bps),
};

static enum usbd_ccid_card_state get_state(struct usbd_ccid_backend *backend)
{
	struct sample_card *card = CONTAINER_OF(backend, struct sample_card, backend);

	return atomic_get(&card->powered) ? USBD_CCID_CARD_ACTIVE : USBD_CCID_CARD_INACTIVE;
}

static int power_on(struct usbd_ccid_backend *backend, enum usbd_ccid_voltage voltage, uint8_t *atr,
		    size_t *length)
{
	struct sample_card *card = CONTAINER_OF(backend, struct sample_card, backend);
	const uint8_t answer[] = {0x3B, 0x00};

	ARG_UNUSED(voltage);
	if (*length < sizeof(answer)) {
		return -EMSGSIZE;
	}
	memcpy(atr, answer, sizeof(answer));
	*length = sizeof(answer);
	atomic_set(&card->powered, 1);
	return 0;
}

static int power_off(struct usbd_ccid_backend *backend)
{
	struct sample_card *card = CONTAINER_OF(backend, struct sample_card, backend);

	atomic_clear(&card->powered);
	return 0;
}

static int get_parameters(struct usbd_ccid_backend *backend,
			  struct usbd_ccid_parameters *parameters)
{
	ARG_UNUSED(backend);
	*parameters = (struct usbd_ccid_parameters){
		.protocol = USBD_CCID_PROTOCOL_T0,
		.data = &t0_parameters,
		.length = sizeof(t0_parameters),
	};
	return 0;
}

static int set_parameters(struct usbd_ccid_backend *backend,
			  const struct usbd_ccid_parameters *requested,
			  struct usbd_ccid_parameters *parameters)
{
	struct usbd_ccid_t0_parameters proposed;

	if (requested->protocol != USBD_CCID_PROTOCOL_T0) {
		return USBD_CCID_PARAMETER_ERROR_PROTOCOL;
	}
	if (requested->length != sizeof(proposed)) {
		return USBD_CCID_PARAMETER_ERROR_LENGTH;
	}
	memcpy(&proposed, requested->data, sizeof(proposed));
	/* The ATR determines convention, independently of the host request. */
	proposed.tccks &= ~USBD_CCID_CONVENTION_INVERSE;
	const uint8_t *expected = (const uint8_t *)&t0_parameters;
	const uint8_t *actual = (const uint8_t *)&proposed;

	for (size_t i = 0; i < sizeof(proposed); i++) {
		if (expected[i] != actual[i]) {
			return USBD_CCID_PARAMETER_ERROR_FINDEX_DINDEX + i;
		}
	}
	return get_parameters(backend, parameters);
}

static int get_rate(struct usbd_ccid_backend *backend,
		    struct usbd_ccid_data_rate_and_clock *settings)
{
	ARG_UNUSED(backend);
	settings->clock_khz = clocks_khz[0];
	settings->data_rate_bps = rates_bps[0];
	return 0;
}

static int transceive(struct usbd_ccid_backend *backend, const uint8_t *command, size_t length,
		      uint8_t *response, size_t *response_length)
{
	struct sample_card *card = CONTAINER_OF(backend, struct sample_card, backend);

	if (!atomic_get(&card->powered)) {
		return -EIO;
	}
	if (length < 4 || length > 261 || *response_length < 2) {
		return -EMSGSIZE;
	}
	response[0] = command[1] == 0xA4 ? 0x90 : 0x6D;
	response[1] = 0x00;
	*response_length = 2;
	return 0;
}

static const struct usbd_ccid_card_ops card_ops = {
	.get_state = get_state,
	.power_on = power_on,
	.power_off = power_off,
	.get_parameters = get_parameters,
	.set_parameters = set_parameters,
	.reset_parameters = get_parameters,
	.get_data_rate_and_clock = get_rate,
	.transceive = transceive,
};

int main(void)
{
	struct usbd_context *usb;
	int ret;

	card.backend.ops = &card_ops;
	card.backend.config = reader_config;
	ret = usbd_ccid_register_backend(DT_PROP(DT_NODELABEL(sample_card), instance_id),
					 &card.backend);
	if (ret != 0) {
		return ret;
	}
	usb = sample_usbd_setup_device(NULL);
	if (usb == NULL) {
		return -ENODEV;
	}
	ret = usbd_init(usb);
	if (ret == 0) {
		ret = usbd_enable(usb);
	}
	if (ret == 0) {
		LOG_INF("CCID sample ready");
	}
	return ret;
}
