/*
 * Copyright (c) 2026 Mahendra Sondagar <mahendra@dotcom.co.in>
 * Copyright (c) 2026 Parin Baudhanwala <parin.baudhanwala@dnkmail.in>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Generic presence sensor sample: reads whatever channels the driver's decoder
 * supports with the Sensor Read & Decode API, on data-ready or by polling.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/sys/printk.h>

#if DT_HAS_COMPAT_STATUS_OKAY(st_sths34pf80)
#include <zephyr/drivers/sensor/sths34pf80.h>
#endif

#define SENSOR_NODE DT_ALIAS(presence_sensor)

#if !DT_NODE_HAS_STATUS_OKAY(SENSOR_NODE)
#error "Define the presence-sensor alias in a board overlay"
#endif

#define USE_TRIGGER DT_NODE_HAS_PROP(SENSOR_NODE, int_gpios)

/* Large enough for the raw frame of any supported driver.
 * For the STHS34PF80 the actual frame is sizeof(struct sths34pf80_rtio_data).
 */
#define RAW_BUF_SIZE 128

struct chan_info {
	enum sensor_channel chan;
	const char *name;
};

static const struct chan_info channels[] = {
	{SENSOR_CHAN_AMBIENT_TEMP, "ambient temp [C]"},
#if DT_HAS_COMPAT_STATUS_OKAY(st_sths34pf80)
	{(enum sensor_channel)SENSOR_CHAN_STHS34PF80_OBJECT_TEMP, "object temp [C]"},
	{(enum sensor_channel)SENSOR_CHAN_STHS34PF80_PRESENCE, "presence"},
	{(enum sensor_channel)SENSOR_CHAN_STHS34PF80_MOTION, "motion"},
	{(enum sensor_channel)SENSOR_CHAN_STHS34PF80_PRESENCE_FLAG, "presence flag"},
	{(enum sensor_channel)SENSOR_CHAN_STHS34PF80_MOTION_FLAG, "motion flag"},
	{(enum sensor_channel)SENSOR_CHAN_STHS34PF80_TEMP_SHOCK_FLAG, "temp shock flag"},
#endif
};

SENSOR_DT_READ_IODEV(sensor_iodev, SENSOR_NODE, {SENSOR_CHAN_ALL, 0});
RTIO_DEFINE(sensor_rtio, 1, 1);

#if USE_TRIGGER
static K_SEM_DEFINE(data_sem, 0, 1);
#endif

static void print_all(const struct device *dev)
{
	const struct sensor_decoder_api *decoder;
	uint8_t buf[RAW_BUF_SIZE] __aligned(8);
	/* Space for the header + exactly one reading. */
	uint8_t out_buf[sizeof(struct sensor_q31_data) + sizeof(struct sensor_q31_sample_data)]
		__aligned(8);
	struct sensor_q31_data *out = (struct sensor_q31_data *)out_buf;
	int rc;

	rc = sensor_get_decoder(dev, &decoder);
	if (rc != 0) {
		printk("No decoder (%d)\n", rc);
		return;
	}

	rc = sensor_read(&sensor_iodev, &sensor_rtio, buf, sizeof(buf));
	if (rc != 0) {
		printk("Read failed (%d)\n", rc);
		return;
	}

	for (size_t i = 0; i < ARRAY_SIZE(channels); i++) {
		struct sensor_chan_spec spec = {channels[i].chan, 0};
		uint32_t fit = 0;
		uint16_t frames = 0;

		if (decoder->get_frame_count(buf, spec, &frames) != 0 || frames == 0) {
			continue;
		}

		if (decoder->decode(buf, spec, &fit, 1, out) <= 0) {
			continue;
		}

		printk("%-18s %.3f\n", channels[i].name,
		       (double)out->readings[0].value * (double)((int64_t)1 << out->shift) /
			       2147483648.0);
	}
	printk("\n");
}

#if USE_TRIGGER
static void trigger_handler(const struct device *dev, const struct sensor_trigger *trig)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(trig);

	/* Signal main() to read and print; keeps the handler short. */
	k_sem_give(&data_sem);
}
#endif

int main(void)
{
	const struct device *dev = DEVICE_DT_GET(SENSOR_NODE);

	printk("Presence sensor sample\n");

	if (!device_is_ready(dev)) {
		printk("Device %s is not ready\n", dev->name);
		return 0;
	}

#if USE_TRIGGER
	struct sensor_trigger trig = {.type = SENSOR_TRIG_DATA_READY, .chan = SENSOR_CHAN_ALL};

	if (sensor_trigger_set(dev, &trig, trigger_handler) != 0) {
		printk("Cannot set the data-ready trigger\n");
		return -EIO;
	}
#endif

	while (1) {
#if USE_TRIGGER
		k_sem_take(&data_sem, K_FOREVER);
#else
		k_sleep(K_SECONDS(1));
#endif
		print_all(dev);
	}

	return 0;
}
