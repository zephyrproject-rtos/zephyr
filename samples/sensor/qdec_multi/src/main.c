/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Multi-unit quadrature decoder sample using the Read-and-Decode API.
 *
 * This sample targets any quadrature decoder whose driver exposes
 * multiple counting units as child nodes (``unit@N``) of a single
 * devicetree node, each keyed by its ``reg`` value. The Read-and-Decode
 * API threads the unit index through ``sensor_chan_spec.chan_idx`` so
 * each unit can be read from the same sensor device.
 *
 * The devicetree is expected to provide a ``qdec0`` alias pointing to
 * that parent node.
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/rtio/rtio.h>

#define QDEC_NODE DT_ALIAS(qdec0)

BUILD_ASSERT(DT_NODE_EXISTS(QDEC_NODE), "Board overlay must provide a 'qdec0' alias pointing to a "
					"multi-unit quadrature decoder device.");

#define QDEC_UNIT_CHAN(node_id)                                                                    \
	{.chan_type = SENSOR_CHAN_ROTATION, .chan_idx = DT_REG_ADDR(node_id)},

SENSOR_DT_READ_IODEV(qdec_iodev, QDEC_NODE, DT_FOREACH_CHILD(QDEC_NODE, QDEC_UNIT_CHAN));

RTIO_DEFINE_WITH_MEMPOOL(qdec_rtio, 4, 4, 4, 256, sizeof(void *));

/* clang-format off */
static const uint8_t unit_indices[] = {
	DT_FOREACH_CHILD_SEP(QDEC_NODE, DT_REG_ADDR, (,))
};
/* clang-format on */

int main(void)
{
	const struct device *dev = DEVICE_DT_GET(QDEC_NODE);
	const struct sensor_decoder_api *decoder;
	uint8_t buf[256];
	int rc;

	if (!device_is_ready(dev)) {
		printf("qdec device not ready\n");
		return 0;
	}
	if (sensor_get_decoder(dev, &decoder) != 0) {
		printf("decoder unavailable (driver lacks .get_decoder?)\n");
		return 0;
	}

	printf("qdec multi-unit sample: %zu units\n", ARRAY_SIZE(unit_indices));

	while (1) {
		k_msleep(1000);

		rc = sensor_read(&qdec_iodev, &qdec_rtio, buf, sizeof(buf));
		if (rc != 0) {
			printf("sensor_read: %d\n", rc);
			continue;
		}

		for (size_t i = 0; i < ARRAY_SIZE(unit_indices); i++) {
			struct sensor_chan_spec spec = {
				.chan_type = SENSOR_CHAN_ROTATION,
				.chan_idx = unit_indices[i],
			};
			struct sensor_q31_data data = {0};
			uint32_t fit = 0;

			rc = decoder->decode(buf, spec, &fit, 1, &data);
			if (rc < 1) {
				continue;
			}
			printf("unit %u: q31=%d shift=%d\n", unit_indices[i],
			       (int)data.readings[0].value, data.shift);
		}
	}
	return 0;
}
