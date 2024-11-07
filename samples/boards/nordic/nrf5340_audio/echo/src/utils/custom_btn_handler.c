#include "custom_btn_handler.h"
#include <zephyr/kernel.h>
#include <macros_common.h>
#include <zephyr/zbus/zbus.h>
#include "nrf5340_audio_common.h"
#include "button_assignments.h"
#include <hw_codec.h>
#include <i2s_handler.h>
#include "led.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(custom_btn_handler, CONFIG_CUSTOM_BTN_HANDLER_LOG_LEVEL);

ZBUS_CHAN_DECLARE(button_chan);
ZBUS_SUBSCRIBER_DEFINE(button_evt_sub, CONFIG_BUTTON_MSG_SUB_QUEUE_SIZE);

static struct k_thread button_msg_sub_thread_data;
static k_tid_t button_msg_sub_thread_id;
K_THREAD_STACK_DEFINE(button_msg_sub_thread_stack, CONFIG_BUTTON_MSG_SUB_STACK_SIZE);

bool audio_state;

/**
 * @brief Switch for all button operation
 *
 */
static void button_msg_sub_thread(void)
{
	int ret;
	const struct zbus_channel *chan;

	while (1) {
		ret = zbus_sub_wait(&button_evt_sub, &chan, K_FOREVER);
		ERR_CHK(ret);

		struct button_msg msg;

		ret = zbus_chan_read(chan, &msg, ZBUS_READ_TIMEOUT_MS);
		ERR_CHK(ret);

		LOG_DBG("Got btn evt from queue - id = %d, action = %d", msg.button_pin,
			msg.button_action);

		if (msg.button_action != BUTTON_PRESS) {
			LOG_WRN("Unhandled button action");
			return;
		}

		switch (msg.button_pin) {
		case BUTTON_PLAY_PAUSE:
			/*  leds and state check */
			if (!audio_state) {
				audio_start();
				ret = led_on(LED_APP_RGB, LED_COLOR_BLUE);
				ERR_CHK(ret);
			} else {
				audio_stop();
				ret = led_on(LED_APP_RGB, LED_COLOR_YELLOW);
				ERR_CHK(ret);
			}
			audio_state = !audio_state;

			break;

		case BUTTON_VOLUME_UP:
		/* call hardware up */
			hw_codec_volume_increase();
			break;

		case BUTTON_VOLUME_DOWN:
			/* call hardware down */
			hw_codec_volume_decrease();
			break;

		case BUTTON_4:
			/* TODO: use this button to switch between line in and mic */
			break;

		case BUTTON_5:
			break;

		default:
			LOG_WRN("Unexpected/unhandled button id: %d", msg.button_pin);
		}

		STACK_USAGE_PRINT("button_msg_thread", &button_msg_sub_thread_data);
	}
}


/**
 * @brief	Create zbus subscriber threads.
 *
 * @return	0 for success, error otherwise.
 */
static int zbus_subscribers_create(void)
{
	int ret;

	/* do not initialise globals to false */
	audio_state = false;

	button_msg_sub_thread_id = k_thread_create(
		&button_msg_sub_thread_data, button_msg_sub_thread_stack,
		CONFIG_BUTTON_MSG_SUB_STACK_SIZE, (k_thread_entry_t)button_msg_sub_thread, NULL,
		NULL, NULL, K_PRIO_PREEMPT(CONFIG_BUTTON_MSG_SUB_THREAD_PRIO), 0, K_NO_WAIT);
	ret = k_thread_name_set(button_msg_sub_thread_id, "BUTTON_MSG_SUB");
	if (ret) {
		LOG_ERR("Failed to create button_msg thread");
		return ret;
	}
	return 0;
}

/**
 * @brief	Link zbus producers and observers.
 *
 * @return	0 for success, error otherwise.
 */
static int zbus_link_producers_observers(void)
{
	int ret;

	if (!IS_ENABLED(CONFIG_ZBUS)) {
		return -ENOTSUP;
	}

	ret = zbus_chan_add_obs(&button_chan, &button_evt_sub, ZBUS_ADD_OBS_TIMEOUT_MS);
	if (ret) {
		LOG_ERR("Failed to add button sub");
		return ret;
	}

	return 0;
}


int enable_buttons(void)
{
	int ret;

	ret = zbus_subscribers_create();
	ERR_CHK_MSG(ret, "Failed to create zbus subscriber threads");

	ret = zbus_link_producers_observers();
	ERR_CHK_MSG(ret, "Failed to link zbus producers and observers");

	return 0;
}
