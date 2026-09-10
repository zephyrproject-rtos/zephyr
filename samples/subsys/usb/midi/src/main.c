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

static enum usbd_midi_status midi_status = USBD_MIDI_STATUS_DISABLED;

static void key_press(struct input_event *evt, void *user_data)
{
	struct midi_ump ump;
	uint8_t channel = 0;
	uint8_t note;
	uint8_t velocity;
	uint8_t command;
	int ret;

	ARG_UNUSED(user_data);

	/* Only handle key presses in the 7bit MIDI range */
	if (evt->type != INPUT_EV_KEY || evt->code > 0x7f) {
		return;
	}

	note = evt->code;
	velocity = evt->value ? 100 : 0;
	command = evt->value ? UMP_MIDI_NOTE_ON : UMP_MIDI_NOTE_OFF;

	ump = UMP_MIDI1_CHANNEL_VOICE(0, command, channel, note, velocity);
	ret = usbd_midi_send(midi, ump);
	if (ret != 0) {
		LOG_WRN("Failed to send note event (%d)", ret);
	}
}
INPUT_CALLBACK_DEFINE(NULL, key_press, NULL);

static const struct ump_endpoint_dt_spec ump_ep_dt =
	UMP_ENDPOINT_DT_SPEC_GET(USB_MIDI_DT_NODE);

const struct ump_stream_responder_cfg responder_cfg =
	UMP_STREAM_RESPONDER(midi, usbd_midi_send, &ump_ep_dt);

static void on_midi_packet(const struct device *dev, const struct midi_ump ump)
{
	LOG_INF("Received MIDI packet (MT=%X)", UMP_MT(ump));

	switch (UMP_MT(ump)) {
	case UMP_MT_MIDI1_CHANNEL_VOICE:
		LOG_INF("Send back MIDI1 message %02X %02X %02X", UMP_MIDI_STATUS(ump),
			UMP_MIDI1_P1(ump), UMP_MIDI1_P2(ump));
		usbd_midi_send(dev, ump);
		break;
	case UMP_MT_MIDI2_CHANNEL_VOICE:
		LOG_INF("Send back MIDI2 channel voice message");
		usbd_midi_send(dev, ump);
		break;
	case UMP_MT_UMP_STREAM:
		ump_stream_respond(&responder_cfg, ump);
		break;
	}
}

static void on_status_changed(const struct device *dev, const enum usbd_midi_status status)
{
	ARG_UNUSED(dev);

	midi_status = status;

	/* Light up the LED (if any) while the USB-MIDI interface is available */
	if (led.port) {
		gpio_pin_set_dt(&led, status != USBD_MIDI_STATUS_DISABLED);
	}

	switch (status) {
	case USBD_MIDI_STATUS_DISABLED:
		LOG_INF("USB-MIDI interface disabled");
		break;
	case USBD_MIDI_STATUS_MIDI1:
		LOG_INF("Host selected USB-MIDI 1.0 alternate setting");
		break;
	case USBD_MIDI_STATUS_MIDI2:
		LOG_INF("Host selected USB-MIDI 2.0 alternate setting");
		break;
	default:
		CODE_UNREACHABLE;
	}
}

static const struct usbd_midi_ops ops = {
	.rx_packet_cb = on_midi_packet,
	.status_changed_cb = on_status_changed,
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
