/*
 * Copyright (c) 2024 Titouan Christophe
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file
 * @brief Sample application for USB MIDI 2.0 device class
 */

#include <sample_usbd.h>

#include <zephyr/device.h>
#include <zephyr/input/input.h>
#include <zephyr/usb/class/usb_midi.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(sample_usb_midi, LOG_LEVEL_INF);

static const struct device *const midi = DEVICE_DT_GET(DT_NODELABEL(usb_midi));

static void key_press(struct input_event *evt, void *user_data)
{
	/* Only handle key presses in the 7bit MIDI range */
	if (evt->type != INPUT_EV_KEY || evt->code > 0x7f) {
		return;
	}
	uint8_t command = evt->value ? MIDI_NOTE_ON : MIDI_NOTE_OFF;
	uint8_t channel = 0;
	uint8_t note = evt->code;
	uint8_t velocity = 100;

	usb_midi_send(midi, UMP_MIDI1(0, command, channel, note, velocity));
}
INPUT_CALLBACK_DEFINE(NULL, key_press, NULL);

static void on_midi_packet(const struct device *dev, const uint32_t *ump)
{
	LOG_INF("Received MIDI packet (MT=%X)", UMP_MT(ump));

	/* Only send MIDI1 channel voice messages back to the host */
	if (UMP_MT(ump) == MT_MIDI1_CHANNEL_VOICE) {
		LOG_INF("Send back MIDI1 message %X%X %02X %02X", UMP_MIDI1_CHANNEL(ump),
			UMP_MIDI1_COMMAND(ump), UMP_MIDI1_P1(ump), UMP_MIDI1_P2(ump));
		usb_midi_send(dev, ump);
	}
}

int main(void)
{
	struct usbd_context *sample_usbd;

	if (!device_is_ready(midi)) {
		LOG_ERR("MIDI device not ready");
		return -1;
	}

	usb_midi_set_callback(midi, on_midi_packet);

	sample_usbd = sample_usbd_init_device(NULL);
	if (sample_usbd == NULL) {
		LOG_ERR("Failed to initialize USB device");
		return -1;
	}

	if (!usbd_can_detect_vbus(sample_usbd) && usbd_enable(sample_usbd)) {
		LOG_ERR("Failed to enable device support");
		return -1;
	}

	LOG_INF("USB device support enabled");
	return 0;
}
