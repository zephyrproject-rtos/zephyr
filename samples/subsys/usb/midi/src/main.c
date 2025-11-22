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
	/* Only handle key presses in the 7bit MIDI range */
	if (evt->type != INPUT_EV_KEY || evt->code > 0x7f) {
		return;
	}
	uint8_t channel = 0;
	uint8_t note = evt->code;
	uint8_t velocity = evt->value ? 100 : 0;
	uint8_t status = (evt->value ? 0x90 : 0x80) | channel;
	int ret;

#if IS_ENABLED(CONFIG_USBD_MIDI2_ALTSETTING_MIDI1)
	uint8_t midi_bytes[3] = {status, note, velocity};

	ret = usbd_midi_send_midi1(midi, 0, midi_bytes, sizeof(midi_bytes));
#else
	struct midi_ump ump = UMP_MIDI1_CHANNEL_VOICE(0, status >> 4, channel, note, velocity);

	ret = usbd_midi_send(midi, ump);
#endif

	if (ret != 0) {
		LOG_WRN("Failed to send note event (%d)", ret);
	}
}
INPUT_CALLBACK_DEFINE(NULL, key_press, NULL);

static const struct ump_endpoint_dt_spec ump_ep_dt = UMP_ENDPOINT_DT_SPEC_GET(USB_MIDI_DT_NODE);

const struct ump_stream_responder_cfg responder_cfg =
	UMP_STREAM_RESPONDER(midi, usbd_midi_send, &ump_ep_dt);

#if IS_ENABLED(CONFIG_USBD_MIDI2_ALTSETTING_MIDI1)
/**
 * @brief Echo raw MIDI 1.0 messages back to the host.
 *
 * @param dev USB-MIDI device instance.
 * @param cable Virtual cable/group index.
 * @param bytes Pointer to the MIDI status byte followed by data bytes.
 * @param len Number of valid bytes stored in @p bytes.
 *
 * @return N/A.
 */
static void on_midi1_message(const struct device *dev, uint8_t cable, const uint8_t *bytes,
			     uint8_t len)
{
	uint8_t payload[3] = {0};
	uint8_t p2 = 0;
	int ret;

	if (len > (uint8_t)sizeof(payload)) {
		len = (uint8_t)sizeof(payload);
	}

	memcpy(payload, bytes, len);
	if (len == 3U) {
		p2 = payload[2];
	}

	LOG_INF("Send back MIDI1 message on cable %u: %02X %02X %02X", cable, payload[0],
		payload[1], p2);

	ret = usbd_midi_send_midi1(dev, cable, payload, len);
	if (ret != 0) {
		LOG_WRN("MIDI1 echo failed (%d)", ret);
	}
}
#endif

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
	/* Light up the LED (if any) when USB-MIDI2.0 is enabled */
	if (led.port) {
		gpio_pin_set_dt(&led, ready);
	}
}

static const struct usbd_midi_ops ops = {
	.rx_packet_cb = on_midi_packet,
#if IS_ENABLED(CONFIG_USBD_MIDI2_ALTSETTING_MIDI1)
	.rx_midi1_cb = on_midi1_message,
#endif
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
