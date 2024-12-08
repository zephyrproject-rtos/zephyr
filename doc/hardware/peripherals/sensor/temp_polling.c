/*
 * Copyright (c) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/sensor.h>

#define TEMP_CHANNEL {SENSOR_CHAN_AMBIENT_TEMP, 0}

const struct device *const temp0 = DEVICE_DT_GET(DT_ALIAS(temp0));

SENSOR_DT_READ_IODEV(temp_iodev, DT_ALIAS(temp0), {TEMP_CHANNEL});
RTIO_DEFINE(temp_ctx, 1, 1);

int main(void)
{
	int rc;
	uint8_t buf[8];
	uint32_t temp_frame_iter = 0;
	struct sensor_q31_data temp_data = {0};
	struct sensor_decode_context temp_decoder = SENSOR_DECODE_CONTEXT_INIT(
		SENSOR_DECODER_DT_GET(DT_ALIAS(temp0)), buf, SENSOR_CHAN_AMBIENT_TEMP, 0);

	while (1) {
		/* Blocking read */
		rc = sensor_read(temp_iodev, &temp_ctx, buf, sizeof(buf));

		if (rc != 0) {
			printk("sensor_read() failed %d\n", rc);
		}

		/* Decode the data into a single q31 */
		sensor_decode(&temp_decoder, &temp_data, 1);

		printk("Temperature " PRIsensor_q31_data "\n",
		       PRIsensor_q31_data_arg(temp_data, 0));

		k_msleep(1);
	}
}
