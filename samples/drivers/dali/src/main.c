#include <stdio.h>

#include <zephyr/kernel.h>
#include <zephyr/drivers/led.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main);

#include <app_version.h>
#include <dali.h>

#define DALI_NODE DT_NODELABEL(dali0)

int main(void)
{
	LOG_INF("DALI Blinky %s", APP_VERSION_STRING);
	LOG_INF("Target board: %s", CONFIG_BOARD_TARGET);
	LOG_DBG("Build version %s", STRINGIFY(APP_BUILD_VERSION));

	/* get the DALI device */
	const struct device *dali_dev = DEVICE_DT_GET(DALI_NODE);
	if (!dali_dev) {
		LOG_ERR("failed to get DALI device.");
		return 0;
	}

	/* prepare on / off frame */
	const struct dali_tx_frame frame_recall_max = (struct dali_tx_frame){
		.frame =
			(struct dali_frame){
				.event_type = DALI_FRAME_GEAR,
				.data = 0xff05,
			},
		.priority = 2,
	};
	const struct dali_tx_frame frame_off = (struct dali_tx_frame){
		.frame =
			(struct dali_frame){
				.event_type = DALI_FRAME_GEAR,
				.data = 0xff00,
			},
		.priority = 2,
	};

	/* send on / off DALI frames */
	for (;;) {
		dali_send(dali_dev, &frame_recall_max);
		k_sleep(K_MSEC(2000));
		dali_send(dali_dev, &frame_off);
		k_sleep(K_MSEC(2000));
	}
	return 0;
}
