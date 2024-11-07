#include <stdio.h>

#include <zephyr/kernel.h>

#include "nrf5340_audio_common.h"
#include "nrf5340_audio_dk.h"
#include <macros_common.h>
#include <i2s_handler.h>
#include <custom_btn_handler.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main, CONFIG_MAIN_LOG_LEVEL);


int main(void)
{
	int ret;

	LOG_INF("nRF5340 APP core started");

	/* Standard inits, these were edited from the audio application */
	ret = nrf5340_audio_dk_init();
	ERR_CHK(ret);

	/* Some more standard inits such as the buttons and leds */
	ret = nrf5340_audio_common_init();
	ERR_CHK(ret);

	/* Set callback that is called whenever an interrupt happens by I2S */
	/* Custom i2s_handler file */
	ret = audio_init();
	ERR_CHK(ret);

	/* Start the i2s echoing process */
	/* more user friendly with buttons */
	ret = enable_buttons();
	ERR_CHK(ret);

	LOG_INF("Main thread finished");

	return 0;
}
