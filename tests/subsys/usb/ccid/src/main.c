// SPDX-License-Identifier: Apache-2.0
/* SPDX-FileCopyrightText: Copyright (c) 2026 TeraTron GmbH */
#include <errno.h>
#include <string.h>
#include <zephyr/ztest.h>
#include <zephyr/usb/usbd.h>
#include <zephyr/usb/usbh.h>
#include <zephyr/usb/class/usbd_ccid.h>
#include <zephyr/sys/byteorder.h>
#include "usbh_ch9.h"
#include "usbh_device.h"

#define HEADER_SIZE       10
#define READER_COUNT      3
#define RESPONSE_CAPACITY (CONFIG_USBD_CCID_MAX_MESSAGE_SIZE + 512)

USBD_DEVICE_DEFINE(ccid_device, DEVICE_DT_GET(DT_NODELABEL(zephyr_udc0)), 0x2FE3, 0xFFFF);
USBD_CONFIGURATION_DEFINE(fs_config, USB_SCD_SELF_POWERED, 100, NULL);
USBD_CONFIGURATION_DEFINE(hs_config, USB_SCD_SELF_POWERED, 100, NULL);
USBD_DESC_LANG_DEFINE(language);
USBH_CONTROLLER_DEFINE(host, DEVICE_DT_GET(DT_NODELABEL(zephyr_uhc0)));

static const uint32_t clocks_a[] = {1000, 2000};
static const uint32_t clocks_b[] = {4000};
static const uint32_t rates_a[] = {9600, 19200};
static const uint32_t rates_b[] = {10752};
static const struct usbd_ccid_reader_config configs[] = {
	{
		.protocols = USBD_CCID_PROTOCOL_MASK(USBD_CCID_PROTOCOL_T0),
		.default_protocol = USBD_CCID_PROTOCOL_T0,
		.features = USBD_CCID_AUTO_PARAMETERS | USBD_CCID_AUTO_NEGOTIATION |
			    USBD_CCID_AUTO_VOLTAGE | USBD_CCID_SHORT_APDU,
		.voltage_support = USBD_CCID_VOLTAGE_MASK(USBD_CCID_VOLTAGE_3V),
		.clock_frequencies = clocks_a,
		.clock_count = ARRAY_SIZE(clocks_a),
		.data_rates = rates_a,
		.data_rate_count = ARRAY_SIZE(rates_a),
	},
	{
		.protocols = USBD_CCID_PROTOCOL_MASK(USBD_CCID_PROTOCOL_T1),
		.default_protocol = USBD_CCID_PROTOCOL_T1,
		.features = USBD_CCID_AUTO_PARAMETERS | USBD_CCID_AUTO_NEGOTIATION |
			    USBD_CCID_AUTO_VOLTAGE | USBD_CCID_EXTENDED_APDU,
		.voltage_support = USBD_CCID_VOLTAGE_MASK(USBD_CCID_VOLTAGE_1V8),
		.max_ifsd = 254,
		.clock_frequencies = clocks_b,
		.clock_count = ARRAY_SIZE(clocks_b),
		.data_rates = rates_b,
		.data_rate_count = ARRAY_SIZE(rates_b),
		.time_extension_interval_ms = 100,
		.time_extension_multiplier = 2,
	},
	{
		.protocols = USBD_CCID_PROTOCOL_MASK(USBD_CCID_PROTOCOL_T0) |
			     USBD_CCID_PROTOCOL_MASK(USBD_CCID_PROTOCOL_T1),
		.default_protocol = USBD_CCID_PROTOCOL_T0,
		.features = USBD_CCID_AUTO_PARAMETERS | USBD_CCID_AUTO_NEGOTIATION |
			    USBD_CCID_AUTO_VOLTAGE | USBD_CCID_EXTENDED_APDU,
		.voltage_support = USBD_CCID_VOLTAGE_MASK(USBD_CCID_VOLTAGE_5V),
		.max_ifsd = 128,
		.clock_frequencies = clocks_a,
		.clock_count = ARRAY_SIZE(clocks_a),
		.data_rates = rates_a,
		.data_rate_count = ARRAY_SIZE(rates_a),
	},
};

struct card {
	const struct usbd_ccid_reader_config *config;
	struct usbd_ccid_backend backend;
	atomic_t state;
	uint8_t protocol;
	uint8_t parameters[7];
	unsigned int exchange_calls;
	size_t input_length;
	struct k_sem entered;
	struct k_sem release;
};

struct reader {
	struct card *card;
	uint8_t iface;
	uint8_t out_ep;
	uint8_t in_ep;
};

static struct card cards[READER_COUNT];
static struct reader readers[READER_COUNT];
static struct usb_device *device;

static void defaults(struct card *card)
{
	const uint8_t initial[] = {0x11, 0x00, 0x00, 0x0A, 0x00, 0x20, 0x00};

	card->protocol = card->config->default_protocol;
	memcpy(card->parameters, initial, sizeof(initial));
	if (card->protocol == USBD_CCID_PROTOCOL_T1) {
		card->parameters[1] = 0x10;
		card->parameters[3] = 0x4D;
	}
}

static enum usbd_ccid_card_state state(struct usbd_ccid_backend *backend)
{
	return atomic_get(&CONTAINER_OF(backend, struct card, backend)->state);
}

static int power_on(struct usbd_ccid_backend *backend, enum usbd_ccid_voltage voltage, uint8_t *atr,
		    size_t *length)
{
	struct card *card = CONTAINER_OF(backend, struct card, backend);
	const uint8_t t0[] = {0x3B, 0x00};
	const uint8_t t1[] = {0x3B, 0x80, 0x01, 0x81};

	ARG_UNUSED(voltage);
	defaults(card);
	size_t size = card->protocol == USBD_CCID_PROTOCOL_T0 ? sizeof(t0) : sizeof(t1);

	if (*length < size) {
		return -EMSGSIZE;
	}
	memcpy(atr, card->protocol == USBD_CCID_PROTOCOL_T0 ? t0 : t1, size);
	*length = size;
	atomic_set(&card->state, USBD_CCID_CARD_ACTIVE);
	return 0;
}

static int power_off(struct usbd_ccid_backend *backend)
{
	atomic_set(&CONTAINER_OF(backend, struct card, backend)->state, USBD_CCID_CARD_INACTIVE);
	return 0;
}

static int get_parameters(struct usbd_ccid_backend *backend,
			  struct usbd_ccid_parameters *parameters)
{
	struct card *card = CONTAINER_OF(backend, struct card, backend);

	*parameters = (struct usbd_ccid_parameters){
		.protocol = card->protocol,
		.data = card->parameters,
		.length = card->protocol == USBD_CCID_PROTOCOL_T0 ? 5 : 7,
	};
	return 0;
}

static int set_parameters(struct usbd_ccid_backend *backend,
			  const struct usbd_ccid_parameters *requested,
			  struct usbd_ccid_parameters *parameters)
{
	struct card *card = CONTAINER_OF(backend, struct card, backend);
	size_t expected = requested->protocol == USBD_CCID_PROTOCOL_T0 ? 5 : 7;

	if (requested->length != expected) {
		return USBD_CCID_PARAMETER_ERROR_LENGTH;
	}
	const uint8_t *data = requested->data;

	if (data[2] == 0xEE) {
		return USBD_CCID_PARAMETER_ERROR_GUARD_TIME;
	}
	memcpy(card->parameters, data, expected);
	card->protocol = requested->protocol;
	return get_parameters(backend, parameters);
}

static int reset_parameters(struct usbd_ccid_backend *backend,
			    struct usbd_ccid_parameters *parameters)
{
	defaults(CONTAINER_OF(backend, struct card, backend));
	return get_parameters(backend, parameters);
}

static int get_rate(struct usbd_ccid_backend *backend,
		    struct usbd_ccid_data_rate_and_clock *settings)
{
	struct card *card = CONTAINER_OF(backend, struct card, backend);

	settings->clock_khz = card->config->clock_frequencies[0];
	settings->data_rate_bps = card->config->data_rates[0];
	return 0;
}

static int transceive(struct usbd_ccid_backend *backend, const uint8_t *command, size_t length,
		      uint8_t *response, size_t *response_length)
{
	struct card *card = CONTAINER_OF(backend, struct card, backend);

	++card->exchange_calls;
	card->input_length = length;
	if (length != 0 && command[0] == 0x5A) {
		k_sem_give(&card->entered);
		if (k_sem_take(&card->release, K_SECONDS(5)) != 0) {
			return -ETIMEDOUT;
		}
		/* Blocking test commands contain only the marker and zero bytes.
		 * USB traffic while blocked must not change the backend's input.
		 */
		if (command[0] != 0x5A) {
			return -EPROTO;
		}
		for (size_t i = 1; i < length; i++) {
			if (command[i] != 0) {
				return -EPROTO;
			}
		}
	}
	if (length != 0 && (command[0] == 0xA5 || command[0] == 0xA6)) {
		for (size_t i = 1; i < length; i++) {
			if (command[i] != (uint8_t)i) {
				return -EPROTO;
			}
		}
	}
	if (length != 0 && command[0] == 0xA5) {
		size_t result = (CONFIG_USBD_CCID_MAX_MESSAGE_SIZE - HEADER_SIZE) * 2 + 17;

		if (*response_length < result) {
			return -EMSGSIZE;
		}
		for (size_t i = 0; i < result; i++) {
			response[i] = i;
		}
		response[result - 2] = 0x90;
		response[result - 1] = 0;
		*response_length = result;
		return 0;
	}
	if (atomic_get(&card->state) != USBD_CCID_CARD_ACTIVE || *response_length < 3) {
		return -EIO;
	}
	response[0] = card->protocol;
	response[1] = 0x90;
	response[2] = 0x00;
	*response_length = 3;
	return 0;
}

static const struct usbd_ccid_card_ops ops = {
	.get_state = state,
	.power_on = power_on,
	.power_off = power_off,
	.get_parameters = get_parameters,
	.set_parameters = set_parameters,
	.reset_parameters = reset_parameters,
	.get_data_rate_and_clock = get_rate,
	.transceive = transceive,
};

static int transfer_done(struct usb_device *udev, struct uhc_transfer *xfer)
{
	ARG_UNUSED(udev);
	k_sem_give(xfer->priv);
	return 0;
}

static size_t bulk(uint8_t ep, uint8_t *data, size_t length, bool input)
{
	struct k_sem done;
	struct uhc_transfer *xfer;
	struct net_buf *buf;
	size_t result;

	zassert_ok(k_sem_init(&done, 0, 1));
	xfer = usbh_xfer_alloc(device, ep, transfer_done, &done);
	zassert_not_null(xfer);
	buf = usbh_xfer_buf_alloc(device, MAX(length, 1));
	zassert_not_null(buf);
	if (!input && length != 0) {
		(void)net_buf_add_mem(buf, data, length);
	}
	zassert_ok(usbh_xfer_buf_add(device, xfer, buf));
	zassert_ok(usbh_xfer_enqueue(device, xfer));
	zassert_ok(k_sem_take(&done, K_SECONDS(5)), "USB transfer timeout ep %x", ep);
	zassert_ok(xfer->err, "USB transfer failed ep %x", ep);
	result = buf->len;
	if (input) {
		memcpy(data, buf->data, result);
	}
	(void)uhc_xfer_unref(xfer);
	usbh_xfer_buf_free(device, buf);
	return result;
}

static size_t command_level(struct reader *reader, uint8_t type, uint8_t specific, uint16_t level,
			    const uint8_t *parameters, size_t length, uint8_t *response)
{
	uint8_t request[CONFIG_USBD_CCID_MAX_MESSAGE_SIZE] = {0};
	static uint8_t sequence;

	zassert_true(length <= sizeof(request) - HEADER_SIZE);
	request[0] = type;
	sys_put_le32(length, request + 1);
	request[6] = ++sequence;
	request[7] = specific;
	sys_put_le16(level, request + 8);
	if (length != 0) {
		memcpy(request + HEADER_SIZE, parameters, length);
	}
	bulk(reader->out_ep, request, HEADER_SIZE + length, false);
	size_t received = bulk(reader->in_ep, response, RESPONSE_CAPACITY, true);

	zassert_true(received >= HEADER_SIZE);
	zassert_equal(response[6], sequence);
	zassert_equal(sys_get_le32(response + 1), received - HEADER_SIZE);
	return received;
}

static size_t command(struct reader *reader, uint8_t type, uint8_t specific,
		      const uint8_t *parameters, size_t length, uint8_t *response)
{
	return command_level(reader, type, specific, 0, parameters, length, response);
}

static struct reader *extended_reader(void)
{
	for (size_t i = 0; i < READER_COUNT; i++) {
		if ((readers[i].card->config->features & USBD_CCID_EXTENDED_APDU) != 0) {
			return &readers[i];
		}
	}
	zassert_unreachable("No extended APDU reader");
	return NULL;
}

static void discover_readers(void)
{
	const struct usb_cfg_descriptor *cfg = device->cfg_desc;
	const uint8_t *end = (const uint8_t *)cfg + cfg->wTotalLength;
	const uint8_t *next = (const uint8_t *)cfg + cfg->bLength;
	struct reader *reader = NULL;
	size_t count = 0;

	while (next + 2 <= end && next[0] >= 2 && next + next[0] <= end) {
		if (next[1] == USB_DESC_INTERFACE && next[5] == 0x0B) {
			zassert_true(count < READER_COUNT);
			reader = &readers[count++];
			reader->iface = next[2];
		} else if (reader && next[1] == 0x21) {
			uint32_t protocols = sys_get_le32(next + 6);

			for (size_t i = 0; i < ARRAY_SIZE(cards); i++) {
				if (cards[i].config->protocols == protocols) {
					reader->card = &cards[i];
				}
			}
			zassert_not_null(reader->card);
			zassert_equal(next[5], reader->card->config->voltage_support);
			zassert_equal(sys_get_le32(next + 40), reader->card->config->features);
		} else if (reader && next[1] == USB_DESC_ENDPOINT && (next[3] & 3) == 2) {
			if ((next[2] & USB_EP_DIR_IN) != 0) {
				reader->in_ep = next[2];
			} else {
				reader->out_ep = next[2];
			}
		}
		next += next[0];
	}
	zassert_equal(count, READER_COUNT);
	for (size_t i = 0; i < READER_COUNT; i++) {
		zassert_not_equal(readers[i].in_ep, 0);
		zassert_not_equal(readers[i].out_ep, 0);
	}
}

ZTEST(ccid, test_parameters_and_apdu)
{
	uint8_t response[RESPONSE_CAPACITY];

	for (size_t i = 0; i < ARRAY_SIZE(readers); i++) {
		struct reader *reader = &readers[i];
		uint8_t protocol = reader->card->config->default_protocol;
		size_t length = protocol == USBD_CCID_PROTOCOL_T0 ? 5 : 7;
		uint8_t requested[7];

		zassert_true(command(reader, 0x62, 0, NULL, 0, response) > HEADER_SIZE);
		zassert_equal(response[7], 0);
		zassert_equal(command(reader, 0x6C, 0, NULL, 0, response), HEADER_SIZE + length);
		zassert_equal(response[9], protocol);
		memcpy(requested, response + HEADER_SIZE, length);
		requested[2] = 5;
		command(reader, 0x61, protocol, requested, length, response);
		zassert_equal(response[7], 0);
		zassert_equal(response[HEADER_SIZE + 2], 5);
		requested[2] = 0xEE;
		command(reader, 0x61, protocol, requested, length, response);
		zassert_equal(response[7], 0x40);
		zassert_equal(response[8], USBD_CCID_PARAMETER_ERROR_GUARD_TIME);
		zassert_equal(response[HEADER_SIZE + 2], 5);
		command(reader, 0x6D, 0, NULL, 0, response);
		zassert_equal(response[HEADER_SIZE + 2], 0);
		const uint8_t apdu[] = {0x00, 0xA4, 0x04, 0x00};

		zassert_equal(command(reader, 0x6F, 0, apdu, sizeof(apdu), response),
			      HEADER_SIZE + 3);
		zassert_equal(response[HEADER_SIZE], protocol);
		zassert_equal(response[HEADER_SIZE + 1], 0x90);
	}
}

ZTEST(ccid, test_apdu_chaining)
{
	uint8_t response[RESPONSE_CAPACITY];
	uint8_t payload[623];
	const size_t chunk = CONFIG_USBD_CCID_MAX_MESSAGE_SIZE - HEADER_SIZE;
	const size_t total = chunk * 2 + 17;

	payload[0] = 0xA5;
	for (size_t i = 1; i < sizeof(payload); i++) {
		payload[i] = i;
	}
	for (size_t i = 0; i < ARRAY_SIZE(readers); i++) {
		struct reader *reader = &readers[i];
		unsigned int calls;
		size_t received;

		command(reader, 0x62, 0, NULL, 0, response);
		calls = reader->card->exchange_calls;
		received = command_level(reader, 0x6F, 0, 1, payload, 300, response);
		if ((reader->card->config->features & USBD_CCID_EXTENDED_APDU) == 0) {
			zassert_equal(received, HEADER_SIZE);
			zassert_equal(response[8], 8);
			zassert_equal(reader->card->exchange_calls, calls);
			continue;
		}
		zassert_equal(received, HEADER_SIZE);
		zassert_equal(response[9], 0x10);
		zassert_equal(command_level(reader, 0x6F, 0, 3, payload + 300, 250, response),
			      HEADER_SIZE);
		zassert_equal(response[9], 0x10);
		zassert_equal(reader->card->exchange_calls, calls);
		zassert_equal(command_level(reader, 0x6F, 0, 2, payload + 550, 73, response),
			      chunk + HEADER_SIZE);
		zassert_equal(response[9], 1);
		zassert_equal(reader->card->exchange_calls, calls + 1);
		zassert_equal(reader->card->input_length, sizeof(payload));
		for (size_t n = 0; n < chunk; n++) {
			zassert_equal(response[HEADER_SIZE + n], (uint8_t)n);
		}
		zassert_equal(command_level(reader, 0x6F, 0, 0x10, NULL, 0, response),
			      chunk + HEADER_SIZE);
		zassert_equal(response[9], 3);
		for (size_t n = 0; n < chunk; n++) {
			zassert_equal(response[HEADER_SIZE + n], (uint8_t)(n + chunk));
		}
		zassert_equal(command_level(reader, 0x6F, 0, 0x10, NULL, 0, response),
			      HEADER_SIZE + 17);
		zassert_equal(response[9], 2);
		for (size_t n = 0; n < 15; n++) {
			zassert_equal(response[HEADER_SIZE + n], (uint8_t)(total - 17 + n));
		}
		zassert_equal(response[HEADER_SIZE + 15], 0x90);
		zassert_equal(response[HEADER_SIZE + 16], 0);
		command_level(reader, 0x6F, 0, 0x10, NULL, 0, response);
		zassert_equal(response[8], 8);
	}
}

ZTEST(ccid, test_clock_tables)
{
	for (size_t i = 0; i < ARRAY_SIZE(readers); i++) {
		struct reader *reader = &readers[i];
		struct net_buf *buf = usbh_xfer_buf_alloc(device, 64);

		zassert_not_null(buf);
		zassert_ok(usbh_req_setup(device, 0xA1, 2, 0, reader->iface, 64, buf));
		zassert_equal(buf->len, reader->card->config->clock_count * sizeof(uint32_t));
		for (size_t n = 0; n < reader->card->config->clock_count; n++) {
			zassert_equal(sys_get_le32(buf->data + n * 4),
				      reader->card->config->clock_frequencies[n]);
		}
		usbh_xfer_buf_free(device, buf);
	}
}

static void send_raw(struct reader *reader, uint8_t type, uint8_t seq, bool block)
{
	uint8_t request[14] = {type, 0, 0, 0, 0, 0, seq, 0, 0, 0, 0x5A};

	if (block) {
		request[1] = 4;
	}
	bulk(reader->out_ep, request, block ? sizeof(request) : HEADER_SIZE, false);
}

static size_t receive_sequence(struct reader *reader, uint8_t seq, uint8_t *response)
{
	for (size_t i = 0; i < 8; i++) {
		size_t length = bulk(reader->in_ep, response, RESPONSE_CAPACITY, true);

		zassert_true(length >= HEADER_SIZE);
		if (response[6] == seq) {
			return length;
		}
		/* The host discards replies from an aborted command (CCID 5.3.1). */
		zassert_equal(response[7] & 0xC0, 0x80, "Unexpected old final response");
	}
	zassert_unreachable("No matching response");
	return 0;
}

ZTEST(ccid, test_abort_and_time_extension)
{
	uint8_t response[RESPONSE_CAPACITY];
	struct reader *reader = NULL;

	for (size_t i = 0; i < READER_COUNT; i++) {
		if (readers[i].card->config->time_extension_interval_ms != 0) {
			reader = &readers[i];
		}
	}
	zassert_not_null(reader);
	struct card *card = reader->card;

	command(reader, 0x62, 0, NULL, 0, response);
	send_raw(reader, 0x6F, 200, true);
	zassert_ok(k_sem_take(&card->entered, K_SECONDS(1)));
	zassert_equal(bulk(reader->in_ep, response, RESPONSE_CAPACITY, true), HEADER_SIZE);
	zassert_equal(response[6], 200);
	zassert_equal(response[7] & 0xC0, 0x80);
	zassert_equal(response[8], 2);
	/* A different interface remains usable while this backend is blocked. */
	struct reader *other = reader == &readers[0] ? &readers[1] : &readers[0];

	command(other, 0x65, 0, NULL, 0, response);
	zassert_equal(response[7] & 0xC0, 0);
	zassert_ok(usbh_req_setup(device, 0x21, 1, 77 << 8, reader->iface, 0, NULL));
	/* A large command during ABORT is drained without a second payload store. */
	static uint8_t rejected[CONFIG_USBD_CCID_MAX_MESSAGE_SIZE] = {0x6F};

	sys_put_le32(sizeof(rejected) - HEADER_SIZE, rejected + 1);
	rejected[6] = 75;
	memset(rejected + HEADER_SIZE, 0xCC, sizeof(rejected) - HEADER_SIZE);
	bulk(reader->out_ep, rejected, sizeof(rejected), false);
	receive_sequence(reader, 75, response);
	zassert_equal(response[7] & 0xC0, 0x40);
	zassert_equal(response[8], 0xFF);
	send_raw(reader, 0x65, 76, false);
	receive_sequence(reader, 76, response);
	zassert_equal(response[7] & 0xC0, 0x40);
	zassert_equal(response[8], 0xFF);
	send_raw(reader, 0x72, 77, false);
	k_sem_give(&card->release);
	zassert_equal(receive_sequence(reader, 77, response), HEADER_SIZE);
	zassert_equal(response[0], 0x81);
	zassert_equal(response[7] & 0xC0, 0);
	command(reader, 0x65, 0, NULL, 0, response);
	zassert_equal(response[7] & 0xC0, 0);

	/* Reconfigure while a callback is still holding its command buffer. */
	send_raw(reader, 0x6F, 201, true);
	zassert_ok(k_sem_take(&card->entered, K_SECONDS(1)));
	zassert_ok(usbh_req_setup(device, 0, USB_SREQ_SET_CONFIGURATION, 0, 0, 0, NULL));
	zassert_ok(usbh_req_setup(device, 0, USB_SREQ_SET_CONFIGURATION, 1, 0, 0, NULL));
	k_sem_give(&card->release);
	command(reader, 0x65, 0, NULL, 0, response);
	zassert_equal(response[7], 1, "Reconfiguration must require power-on");
	command(reader, 0x62, 0, NULL, 0, response);
	zassert_equal(response[7], 0);
}

ZTEST(ccid, test_busy_frame_workspace)
{
	uint8_t response[RESPONSE_CAPACITY];
	static uint8_t request[CONFIG_USBD_CCID_MAX_MESSAGE_SIZE];
	struct reader *reader = extended_reader();
	size_t mps = usbd_bus_speed(&ccid_device) == USBD_SPEED_HS ? 512 : 64;
	unsigned int calls = reader->card->exchange_calls;

	command(reader, 0x62, 0, NULL, 0, response);
	memset(request, 0, sizeof(request));
	request[0] = 0x6F;
	sys_put_le32(mps - HEADER_SIZE, request + 1);
	request[6] = 225;
	request[HEADER_SIZE] = 0x5A;
	/* A full final packet leaves OUT armed for an optional ZLP. */
	bulk(reader->out_ep, request, mps, false);
	zassert_ok(k_sem_take(&reader->card->entered, K_SECONDS(1)));

	sys_put_le32(2 * mps - HEADER_SIZE, request + 1);
	request[6] = 226;
	memset(request + HEADER_SIZE, 0xCC, sizeof(request) - HEADER_SIZE);
	bulk(reader->out_ep, request, mps, false);
	k_msleep(20);
	/* Finish the old command and consume its reply between the new frame's
	 * packets. The initially discarded frame must remain rejected even once
	 * the workspace is free; it must not execute with missing payload bytes.
	 */
	k_sem_give(&reader->card->release);
	do {
		receive_sequence(reader, 225, response);
	} while ((response[7] & 0xC0) == 0x80);
	zassert_equal(response[7], 0);
	zassert_equal(response[HEADER_SIZE], reader->card->protocol);
	bulk(reader->out_ep, request + mps, mps, false);
	receive_sequence(reader, 226, response);
	zassert_equal(response[7] & 0xC0, 0x40);
	zassert_equal(response[8], 0xE0);
	zassert_equal(reader->card->exchange_calls, calls + 1);

	const uint8_t payload[] = {0xA6, 1, 2, 3};

	command(reader, 0x6F, 0, payload, sizeof(payload), response);
	zassert_equal(response[7], 0);
	zassert_equal(response[HEADER_SIZE], reader->card->protocol);
	zassert_equal(reader->card->exchange_calls, calls + 2);
}

ZTEST(ccid, test_bulk_out_packet_boundaries)
{
	uint8_t response[RESPONSE_CAPACITY];
	static uint8_t payload[CONFIG_USBD_CCID_MAX_MESSAGE_SIZE - HEADER_SIZE];
	static uint8_t request[CONFIG_USBD_CCID_MAX_MESSAGE_SIZE + 512] = {0x6F};
	struct reader *reader = extended_reader();
	size_t mps = usbd_bus_speed(&ccid_device) == USBD_SPEED_HS ? 512 : 64;

	for (size_t i = 0; i < sizeof(payload); i++) {
		payload[i] = i;
	}
	payload[0] = 0xA6;
	command(reader, 0x62, 0, NULL, 0, response);
	const size_t sizes[] = {mps - 1,     mps,     mps + 1,
				2 * mps - 1, 2 * mps, CONFIG_USBD_CCID_MAX_MESSAGE_SIZE};

	for (size_t i = 0; i < ARRAY_SIZE(sizes); i++) {
		if (sizes[i] > CONFIG_USBD_CCID_MAX_MESSAGE_SIZE) {
			continue;
		}
		unsigned int calls = reader->card->exchange_calls;

		/* bulk() does not append an OUT ZLP, including at exact MPS multiples. */
		zassert_equal(command(reader, 0x6F, 0, payload, sizes[i] - HEADER_SIZE, response),
			      HEADER_SIZE + 3, "CCID message length %zu", sizes[i]);
		zassert_equal(response[7], 0);
		zassert_equal(response[HEADER_SIZE + 1], 0x90);
		zassert_equal(reader->card->exchange_calls, calls + 1);
		zassert_equal(reader->card->input_length, sizes[i] - HEADER_SIZE);
	}
	/* A host may still append a ZLP; it must not become a CCID command. */
	bulk(reader->out_ep, NULL, 0, false);
	command(reader, 0x65, 0, NULL, 0, response);
	zassert_equal(response[7], 0);

	size_t total = 2 * mps;
	unsigned int calls = reader->card->exchange_calls;

	sys_put_le32(total - HEADER_SIZE, request + 1);
	request[6] = 210;
	memcpy(request + HEADER_SIZE, payload, total - HEADER_SIZE);
	/* Deliver a complete CCID message over two host transfers. Each transfer
	 * ends at MPS, so neither produces a short packet or an OUT ZLP.
	 */
	bulk(reader->out_ep, request, mps, false);
	k_msleep(20);
	zassert_equal(reader->card->exchange_calls, calls, "Incomplete command was dispatched");
	bulk(reader->out_ep, request + mps, mps, false);
	zassert_equal(bulk(reader->in_ep, response, RESPONSE_CAPACITY, true), HEADER_SIZE + 3);
	zassert_equal(response[6], 210);
	zassert_equal(response[7], 0);
	zassert_equal(reader->card->exchange_calls, calls + 1);
	/* Append ZLP before reading IN, as a host transfer with a trailing ZLP
	 * would do. The reader must not wait for IN before accepting that ZLP.
	 */
	request[6] = 215;
	bulk(reader->out_ep, request, total, false);
	bulk(reader->out_ep, NULL, 0, false);
	zassert_equal(bulk(reader->in_ep, response, RESPONSE_CAPACITY, true), HEADER_SIZE + 3);
	zassert_equal(response[6], 215);
	zassert_equal(response[7], 0);

	/* A short transfer before dwLength bytes is a length error, and the next
	 * complete command must still work. No partial command reaches the backend.
	 */
	calls = reader->card->exchange_calls;
	request[6] = 211;
	bulk(reader->out_ep, request, mps, false);
	bulk(reader->out_ep, NULL, 0, false);
	bulk(reader->in_ep, response, RESPONSE_CAPACITY, true);
	zassert_equal(response[6], 211);
	zassert_equal(response[8], 1);
	zassert_equal(response[7] & 0xC0, 0x40);
	zassert_equal(reader->card->exchange_calls, calls);
	command(reader, 0x65, 0, NULL, 0, response);
	zassert_equal(response[7], 0);

	/* Drain a message larger than the advertised maximum with bounded storage. */
	total = CONFIG_USBD_CCID_MAX_MESSAGE_SIZE + mps;
	sys_put_le32(total - HEADER_SIZE, request + 1);
	request[6] = 212;
	bulk(reader->out_ep, request, total, false);
	bulk(reader->in_ep, response, RESPONSE_CAPACITY, true);
	zassert_equal(response[6], 212);
	zassert_equal(response[8], 1);
	zassert_equal(response[7] & 0xC0, 0x40);
	zassert_equal(reader->card->exchange_calls, calls);
	command(reader, 0x65, 0, NULL, 0, response);
	zassert_equal(response[7], 0);

	/* Control ABORT discards a partially received command before accepting
	 * its independent Bulk-OUT request, rather than appending it as payload.
	 */

	sys_put_le32(2 * mps - HEADER_SIZE, request + 1);
	request[6] = 213;
	bulk(reader->out_ep, request, mps, false);
	zassert_ok(usbh_req_setup(device, 0x21, 1, 214 << 8, reader->iface, 0, NULL));
	uint8_t abort_request[HEADER_SIZE] = {0x72, 0, 0, 0, 0, 0, 214};

	bulk(reader->out_ep, abort_request, sizeof(abort_request), false);
	bulk(reader->in_ep, response, RESPONSE_CAPACITY, true);
	zassert_equal(response[6], 214);
	zassert_equal(response[0], 0x81);
	zassert_equal(response[7] & 0xC0, 0);
	zassert_equal(reader->card->exchange_calls, calls);
	command(reader, 0x65, 0, NULL, 0, response);
	zassert_equal(response[7], 0);

	bulk(reader->out_ep, request, mps, false);
	zassert_ok(usbh_req_setup(device, 0, USB_SREQ_SET_CONFIGURATION, 0, 0, 0, NULL));
	zassert_ok(usbh_req_setup(device, 0, USB_SREQ_SET_CONFIGURATION, 1, 0, 0, NULL));
	command(reader, 0x65, 0, NULL, 0, response);
	zassert_equal(response[7], 1);
	zassert_equal(reader->card->exchange_calls, calls);
}

ZTEST(ccid, test_abort_pending_response)
{
	uint8_t response[RESPONSE_CAPACITY];
	static uint8_t request[CONFIG_USBD_CCID_MAX_MESSAGE_SIZE + 512] = {0x6F};
	struct reader *reader = extended_reader();
	size_t mps = usbd_bus_speed(&ccid_device) == USBD_SPEED_HS ? 512 : 64;
	size_t total = CONFIG_USBD_CCID_MAX_MESSAGE_SIZE + mps;
	unsigned int calls = reader->card->exchange_calls;

	command(reader, 0x62, 0, NULL, 0, response);
	sys_put_le32(total - HEADER_SIZE, request + 1);
	request[6] = 216;
	bulk(reader->out_ep, request, total, false);
	k_msleep(20);
	/* Both abort halves must be receivable before the host discards an old
	 * error reply waiting on IN. No old response may block the Bulk half.
	 */
	zassert_ok(usbh_req_setup(device, 0x21, 1, 217 << 8, reader->iface, 0, NULL));
	send_raw(reader, 0x72, 217, false);
	bulk(reader->in_ep, response, RESPONSE_CAPACITY, true);
	if (response[6] == 216) {
		zassert_equal(response[8], 1);
		bulk(reader->in_ep, response, RESPONSE_CAPACITY, true);
	}
	zassert_equal(response[6], 217);
	zassert_equal(response[0], 0x81);
	zassert_equal(response[7] & 0xC0, 0);

	request[6] = 220;
	bulk(reader->out_ep, request, total, false);
	k_msleep(20);
	zassert_ok(usbh_req_setup(device, 0x21, 1, 221 << 8, reader->iface, 0, NULL));
	send_raw(reader, 0x65, 222, false);
	/* Completing an old packet must not consume the newly pending failure
	 * reply for a command received during the replacement handshake.
	 */
	bulk(reader->in_ep, response, RESPONSE_CAPACITY, true);
	if (response[6] == 220) {
		zassert_equal(response[8], 1);
		bulk(reader->in_ep, response, RESPONSE_CAPACITY, true);
	}
	zassert_equal(response[6], 222);
	zassert_equal(response[7] & 0xC0, 0x40);
	zassert_equal(response[8], 0xFF);
	send_raw(reader, 0x72, 221, false);
	bulk(reader->in_ep, response, RESPONSE_CAPACITY, true);
	zassert_equal(response[6], 221);
	zassert_equal(response[7] & 0xC0, 0);
	zassert_equal(reader->card->exchange_calls, calls);
	command(reader, 0x65, 0, NULL, 0, response);
	zassert_equal(response[7], 0);
}

ZTEST(ccid, test_framing_recovery)
{
	uint8_t response[RESPONSE_CAPACITY];
	struct reader *reader = &readers[0];
	uint8_t invalid[] = {0x65, 1, 0, 0, 0, 0, 203, 0, 0, 0};

	/* Short headers have no usable slot/sequence; reception must recover. */
	bulk(reader->out_ep, invalid, 7, false);
	command(reader, 0x65, 0, NULL, 0, response);
	zassert_equal(response[7] & 0xC0, 0);
	bulk(reader->out_ep, invalid, sizeof(invalid), false);
	zassert_equal(bulk(reader->in_ep, response, RESPONSE_CAPACITY, true), HEADER_SIZE);
	zassert_equal(response[6], 203);
	zassert_equal(response[7] & 0xC0, 0x40);
	zassert_equal(response[8], 1);
	invalid[1] = 0;
	invalid[5] = 1;
	bulk(reader->out_ep, invalid, sizeof(invalid), false);
	bulk(reader->in_ep, response, RESPONSE_CAPACITY, true);
	zassert_equal(response[7], 0x42);
	zassert_equal(response[8], 5);
	command(reader, 0x65, 0, NULL, 0, response);
	zassert_equal(response[7] & 0xC0, 0);
}

ZTEST(ccid, test_suspend_resume)
{
	uint8_t response[RESPONSE_CAPACITY];
	struct reader *reader = &readers[0];
	struct card *card = reader->card;

	command(reader, 0x62, 0, NULL, 0, response);
	send_raw(reader, 0x6F, 202, true);
	zassert_ok(k_sem_take(&card->entered, K_SECONDS(1)));
	zassert_ok(uhc_bus_suspend(host.dev));
	for (size_t i = 0; i < 100 && !usbd_is_suspended(&ccid_device); i++) {
		k_msleep(1);
	}
	zassert_true(usbd_is_suspended(&ccid_device));
	k_sem_give(&card->release);
	for (size_t i = 0; i < 100 && atomic_get(&card->state) == USBD_CCID_CARD_ACTIVE; i++) {
		k_msleep(1);
	}
	zassert_equal(atomic_get(&card->state), USBD_CCID_CARD_INACTIVE);
	zassert_ok(uhc_bus_resume(host.dev));
	for (size_t i = 0; i < 100 && usbd_is_suspended(&ccid_device); i++) {
		k_msleep(1);
	}
	zassert_false(usbd_is_suspended(&ccid_device));
	/* Idle interfaces still had OUT transfers queued at suspend. Their
	 * cancellation and rearming must also work, without losing a command.
	 */
	for (size_t i = 0; i < READER_COUNT; i++) {
		command(&readers[i], 0x65, 0, NULL, 0, response);
		zassert_equal(response[7], 1);
	}
	command(reader, 0x62, 0, NULL, 0, response);
	zassert_equal(response[7], 0);
	const uint8_t select[] = {0, 0xA4, 0, 0};

	zassert_equal(command(reader, 0x6F, 0, select, sizeof(select), response), HEADER_SIZE + 3);
	zassert_equal(response[7], 0);
}

static void *setup(void)
{
	const unsigned int ids[] = {2, 7, 19};
	static const char *const names[] = {"ccid_2", "ccid_7", "ccid_19"};

	zassert_ok(usbh_init(&host));
	zassert_ok(usbh_enable(&host));
	zassert_ok(uhc_bus_reset(host.dev));
	zassert_ok(uhc_bus_resume(host.dev));
	zassert_ok(uhc_sof_enable(host.dev));
	zassert_ok(usbd_add_descriptor(&ccid_device, &language));
	zassert_ok(usbd_add_configuration(&ccid_device, USBD_SPEED_FS, &fs_config));
	if (USBD_SUPPORTS_HIGH_SPEED && usbd_caps_speed(&ccid_device) == USBD_SPEED_HS) {
		zassert_ok(usbd_add_configuration(&ccid_device, USBD_SPEED_HS, &hs_config));
	}
	for (size_t i = 0; i < READER_COUNT; i++) {
		cards[i].config = &configs[i];
		zassert_ok(k_sem_init(&cards[i].entered, 0, 1));
		zassert_ok(k_sem_init(&cards[i].release, 0, 1));
		(void)atomic_set(&cards[i].state, USBD_CCID_CARD_INACTIVE);
		defaults(&cards[i]);
		cards[i].backend.ops = &ops;
		cards[i].backend.config = configs[i];
		zassert_ok(usbd_ccid_register_backend(ids[i], &cards[i].backend));
		zassert_ok(usbd_register_class(&ccid_device, names[i], USBD_SPEED_FS, 1));
		if (USBD_SUPPORTS_HIGH_SPEED && usbd_caps_speed(&ccid_device) == USBD_SPEED_HS) {
			zassert_ok(usbd_register_class(&ccid_device, names[i], USBD_SPEED_HS, 1));
		}
	}
	zassert_ok(usbd_init(&ccid_device));
	zassert_ok(usbd_enable(&ccid_device));
	for (size_t i = 0; i < 200; i++) {
		device = usbh_device_get_any(&host);
		if (device != NULL && device->state == USB_STATE_CONFIGURED) {
			break;
		}
		k_msleep(10);
	}
	zassert_not_null(device);
	zassert_equal(device->state, USB_STATE_CONFIGURED);
	discover_readers();
	return NULL;
}

static void teardown(void *fixture)
{
	ARG_UNUSED(fixture);
	zassert_ok(usbd_disable(&ccid_device));
	zassert_ok(usbd_shutdown(&ccid_device));
	zassert_ok(usbh_disable(&host));
}

ZTEST_SUITE(ccid, NULL, setup, NULL, NULL, teardown);
