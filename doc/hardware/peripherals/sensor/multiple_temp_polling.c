/*
 * Copyright (c) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/sensor.h>

#define TEMP_CHANNELS				\
	{ SENSOR_CHAN_AMBIENT_TEMP, 0 },	\
	{ SENSOR_CHAN_AMBIENT_TEMP, 1 }
#define TEMP_ALIAS(id) DT_ALIAS(CONCAT(temp, id))
#define TEMP_IODEV_SYM(id) CONCAT(temp_iodev, id)
#define TEMP_IODEV_PTR(id) &TEMP_IODEV_SYM(id)
#define TEMP_DEFINE_IODEV(id)			\
	SENSOR_DT_READ_IODEV(			\
		TEMP_IODEV_SYM(id),		\
		TEMP_ALIAS(id),			\
		TEMP_CHANNELS			\
		);

#define NUM_SENSORS 2

LISTIFY(NUM_SENSORS, TEMP_DEFINE_IODEV, (;));

struct sensor_iodev *iodevs[NUM_SENSORS] = { LISTIFY(NUM_SENSORS, TEMP_IODEV_PTR, (,)) };

RTIO_DEFINE_WITH_MEMPOOL(temp_ctx, NUM_SENSORS, NUM_SENSORS, NUM_SENSORS, 8, sizeof(void *));

int main(void)
{
	int rc;
	uint32_t temp_frame_iter = 0;
	struct sensor_q31_data temp_data[2] = {0};
	struct sensor_decoder_api *decoder;
	struct rtio_cqe *cqe;
	uint8_t *buf;
	uint32_t buf_len;

	while (1) {
		/* Non-Blocking read for each sensor */
		for (int i = 0; i < NUM_SENSORS; i++) {
			rc = sensor_read_async_mempool(iodevs[i], &temp_ctx, iodevs[i]);

			if (rc != 0) {
				printk("sensor_read() failed %d\n", rc);
				return;
			}
		}

		/* Wait for read completions */
		for (int i = 0; i < NUM_SENSORS; i++) {
			cqe = rtio_cqe_consume_block(&temp_ctx);

			if (cqe->result != 0) {
				printk("async read failed %d\n", cqe->result);
				return;
			}

			/* Get the associated mempool buffer with the completion */
			rc = rtio_cqe_get_mempool_buffer(&temp_ctx, cqe, &buf, &buf_len);

			if (rc != 0) {
				printk("get mempool buffer failed %d\n", rc);
				return;
			}

			struct device *sensor = ((struct sensor_read_config *)
				((struct rtio_iodev *)cqe->userdata)->data)->sensor;

			/* Done with the completion event, release it */
			rtio_cqe_release(&temp_ctx, cqe);

			rc = sensor_get_decoder(sensor, &decoder);
			if (rc != 0) {
				printk("sensor_get_decoder failed %d\n", rc);
				return;
			}

			/* Frame iterators, one per channel we are decoding */
			uint32_t temp_fits[2] = { 0, 0 };

			decoder->decode(buf, {SENSOR_CHAN_AMBIENT_TEMP, 0},
					&temp_fits[0], 1, &temp_data[0]);
			decoder->decode(buf, {SENSOR_CHAN_AMBIENT_TEMP, 1},
					&temp_fits[1], 1, &temp_data[1]);

			/* Done with the buffer, release it */
			rtio_release_buffer(&temp_ctx, buf, buf_len);

			printk("Temperature for %s channel 0 " PRIsensor_q31_data ", channel 1 "
			    PRIsensor_q31_data "\n",
			    dev->name,
			    PRIsensor_q31_data_arg(temp_data[0], 0),
			    PRIsensor_q31_data_arg(temp_data[1], 0));
		}
	}

	k_msleep(1);
}
