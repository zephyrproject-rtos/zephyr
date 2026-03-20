/*
 * Copyright (c) 2024 Titouan Christophe
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file
 * @brief Sample application for USB MIDI 2.0 device class
 */

#include <sample_usbd.h>

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/input/input.h>
#include <zephyr/sys/util.h>
#include <zephyr/usb/class/usbd_midi2.h>

#include <ump_stream_responder.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(sample_usb_midi, LOG_LEVEL_INF);

#define USB_MIDI_DT_NODE DT_NODELABEL(usb_midi)

static const struct device *const midi = DEVICE_DT_GET(USB_MIDI_DT_NODE);

static struct gpio_dt_spec led = GPIO_DT_SPEC_GET_OR(DT_ALIAS(led0), gpios, {0});

static void key_press(struct input_event *evt, void *user_data)
{
	struct usbd_midi1_packet packet;

	ARG_UNUSED(user_data);

	/* Only handle key presses in the 7bit MIDI range */
	if (evt->type != INPUT_EV_KEY || evt->code > 0x7f) {
		return;
	}
	uint8_t channel = 0;
	uint8_t note = evt->code;
	uint8_t velocity = evt->value ? 100 : 0;
	int ret;

	packet.cable_number = 0;
	packet.len = 3U;
	packet.bytes[0] = (evt->value ? MIDI_STATUS_NOTE_ON : MIDI_STATUS_NOTE_OFF) | channel;
	packet.bytes[1] = note;
	packet.bytes[2] = velocity;

	ret = usbd_midi_send_midi1(midi, packet);

	if (ret != 0) {
		LOG_WRN("Failed to send note event (%d)", ret);
	}
}
INPUT_CALLBACK_DEFINE(NULL, key_press, NULL);

static const struct ump_endpoint_dt_spec ump_ep_dt =
	UMP_ENDPOINT_DT_SPEC_GET(USB_MIDI_DT_NODE);

const struct ump_stream_responder_cfg responder_cfg =
	UMP_STREAM_RESPONDER(midi, usbd_midi_send, &ump_ep_dt);

/**
 * @brief Echo raw MIDI 1.0 messages back to the host.
 *
 * @param dev USB-MIDI device instance.
 * @param packet Raw USB-MIDI 1.0 payload.
 *
 * @return N/A.
 */
static void on_midi1_message(const struct device *dev, const struct usbd_midi1_packet packet)
{
	int ret;

	LOG_INF("Send back MIDI1 message on cable %u: %02X %02X %02X", packet.cable_number,
		packet.bytes[0], packet.bytes[1], (packet.len == 3U) ? packet.bytes[2] : 0U);

	ret = usbd_midi_send_midi1(dev, packet);
	if (ret != 0) {
		LOG_WRN("MIDI1 echo failed (%d)", ret);
	}
}

static void on_midi_packet(const struct device *dev, const struct midi_ump ump)
{
	LOG_INF("Received MIDI packet (MT=%X)", UMP_MT(ump));

	switch (UMP_MT(ump)) {
	case UMP_MT_MIDI1_CHANNEL_VOICE:
		/* Only send MIDI1 channel voice messages back to the host */
		LOG_INF("Send back MIDI1 message %02X %02X %02X", UMP_MIDI_STATUS(ump),
			UMP_MIDI1_P1(ump), UMP_MIDI1_P2(ump));
		usbd_midi_send(dev, ump);
		break;
	case UMP_MT_UMP_STREAM:
		ump_stream_respond(&responder_cfg, ump);
		break;
	}
}

static void on_device_ready(const struct device *dev, const bool ready)
{
	ARG_UNUSED(dev);

	/* Light up the LED (if any) while the USB-MIDI interface is available */
	if (led.port) {
		gpio_pin_set_dt(&led, ready);
	}
}

static const struct usbd_midi_ops ops = {
	.rx_packet_cb = on_midi_packet,
	.rx_midi1_cb = on_midi1_message,
	.ready_cb = on_device_ready,
};

int main(void)
{
	struct usbd_context *sample_usbd;

	if (!device_is_ready(midi)) {
		LOG_ERR("MIDI device not ready");
		return -1;
	}

	if (led.port) {
		if (gpio_pin_configure_dt(&led, GPIO_OUTPUT)) {
			LOG_ERR("Unable to setup LED, not using it");
			memset(&led, 0, sizeof(led));
		}
	}

	usbd_midi_set_ops(midi, &ops);

	sample_usbd = sample_usbd_init_device(NULL);
	if (sample_usbd == NULL) {
		LOG_ERR("Failed to initialize USB device");
		return -1;
	}

	if (usbd_enable(sample_usbd)) {
		LOG_ERR("Failed to enable device support");
		return -1;
	}

	LOG_INF("USB device support enabled");
	return 0;
}
