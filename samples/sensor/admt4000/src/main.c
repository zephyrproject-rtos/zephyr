/*
 * Copyright (c) 2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/admt4000.h>
#include <stdio.h>
#include <errno.h>

#ifdef CONFIG_SENSOR_ASYNC_API
#include <zephyr/drivers/sensor_data_types.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/dsp/print_format.h>
#endif

#define SAMPLE_INTERVAL K_SECONDS(1)

#ifdef CONFIG_SENSOR_ASYNC_API
SENSOR_DT_READ_IODEV(iodev, DT_COMPAT_GET_ANY_STATUS_OKAY(adi_admt4000),
		     {SENSOR_CHAN_ADMT4000_ANGLE, 0}, {SENSOR_CHAN_ROTATION, 0},
		     {SENSOR_CHAN_AMBIENT_TEMP, 0}, {SENSOR_CHAN_ADMT4000_COS, 0},
		     {SENSOR_CHAN_ADMT4000_SIN, 0}, {SENSOR_CHAN_ADMT4000_RADIUS, 0});

RTIO_DEFINE(ctx, 1, 1);
#endif /* CONFIG_SENSOR_ASYNC_API */

#ifdef CONFIG_SENSOR_ASYNC_API
static int sample_read_rtio(const struct device *dev)
{
	uint8_t buf[128];
	int rc;

	rc = sensor_read(&iodev, &ctx, buf, sizeof(buf));
	if (rc != 0) {
		printf("sensor_read() failed: %d\n", rc);
		return rc;
	}

	const struct sensor_decoder_api *decoder;

	rc = sensor_get_decoder(dev, &decoder);
	if (rc != 0) {
		printf("sensor_get_decoder() failed: %d\n", rc);
		return rc;
	}

	/* Decode angle */
	uint32_t angle_fit = 0;
	struct sensor_q31_data angle_data = {0};

	rc = decoder->decode(buf, (struct sensor_chan_spec){SENSOR_CHAN_ADMT4000_ANGLE, 0},
			     &angle_fit, 1, &angle_data);
	if (rc > 0) {
		printf("Angle: %s%d.%d deg",
		       PRIq_arg(angle_data.readings[0].angle, 6, angle_data.shift));
	}

	/* Decode turns */
	uint32_t turns_fit = 0;
	struct sensor_q31_data turns_data = {0};

	rc = decoder->decode(buf, (struct sensor_chan_spec){SENSOR_CHAN_ROTATION, 0}, &turns_fit, 1,
			     &turns_data);
	if (rc > 0) {
		printf("  Turns: %s%d.%d",
		       PRIq_arg(turns_data.readings[0].value, 2, turns_data.shift));
	}

	/* Decode temperature */
	uint32_t temp_fit = 0;
	struct sensor_q31_data temp_data = {0};

	rc = decoder->decode(buf, (struct sensor_chan_spec){SENSOR_CHAN_AMBIENT_TEMP, 0}, &temp_fit,
			     1, &temp_data);
	if (rc > 0) {
		printf("  Temp: %s%d.%d C",
		       PRIq_arg(temp_data.readings[0].temperature, 6, temp_data.shift));
	}

	/* Decode cosine */
	uint32_t cos_fit = 0;
	struct sensor_q31_data cos_data = {0};

	rc = decoder->decode(buf, (struct sensor_chan_spec){SENSOR_CHAN_ADMT4000_COS, 0}, &cos_fit,
			     1, &cos_data);
	if (rc > 0) {
		printf("  Cos: %s%d.%d", PRIq_arg(cos_data.readings[0].value, 6, cos_data.shift));
	}

	/* Decode sine */
	uint32_t sin_fit = 0;
	struct sensor_q31_data sin_data = {0};

	rc = decoder->decode(buf, (struct sensor_chan_spec){SENSOR_CHAN_ADMT4000_SIN, 0}, &sin_fit,
			     1, &sin_data);
	if (rc > 0) {
		printf("  Sin: %s%d.%d", PRIq_arg(sin_data.readings[0].value, 6, sin_data.shift));
	}

	/* Decode radius (AMR signal amplitude, mV/V) */
	uint32_t radius_fit = 0;
	struct sensor_q31_data radius_data = {0};

	rc = decoder->decode(buf, (struct sensor_chan_spec){SENSOR_CHAN_ADMT4000_RADIUS, 0},
			     &radius_fit, 1, &radius_data);
	if (rc > 0) {
		printf("  Radius: %s%d.%d mV/V",
		       PRIq_arg(radius_data.readings[0].value, 6, radius_data.shift));
	}

	printf("\n");
	return 0;
}
#endif /* CONFIG_SENSOR_ASYNC_API */

#ifndef CONFIG_SENSOR_ASYNC_API
static int sample_read_polling(const struct device *dev)
{
	struct sensor_value val;
	int ret;

	ret = sensor_sample_fetch(dev);
	if (ret) {
		printf("sensor_sample_fetch failed: %d\n", ret);
		return ret;
	}

	/* Angle */
	ret = sensor_channel_get(dev, (enum sensor_channel)SENSOR_CHAN_ADMT4000_ANGLE, &val);
	if (ret == 0) {
		printf("Angle: %.6f deg", sensor_value_to_double(&val));
	}

	/* Turn count */
	ret = sensor_channel_get(dev, SENSOR_CHAN_ROTATION, &val);
	if (ret == 0) {
		printf("  Turns: %.2f", sensor_value_to_double(&val));
	}

	/* Temperature */
	ret = sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &val);
	if (ret == 0) {
		printf("  Temp: %.6f C", sensor_value_to_double(&val));
	}

	/* Cosine */
	ret = sensor_channel_get(dev, (enum sensor_channel)SENSOR_CHAN_ADMT4000_COS, &val);
	if (ret == 0) {
		printf("  Cos: %.6f", sensor_value_to_double(&val));
	}

	/* Sine */
	ret = sensor_channel_get(dev, (enum sensor_channel)SENSOR_CHAN_ADMT4000_SIN, &val);
	if (ret == 0) {
		printf("  Sin: %.6f", sensor_value_to_double(&val));
	}

	/* Radius (AMR signal amplitude, mV/V) */
	ret = sensor_channel_get(dev, (enum sensor_channel)SENSOR_CHAN_ADMT4000_RADIUS, &val);
	if (ret == 0) {
		printf("  Radius: %.6f mV/V", sensor_value_to_double(&val));
	}

	printf("\n");
	return 0;
}
#endif /* !CONFIG_SENSOR_ASYNC_API */

int main(void)
{
	const struct device *const dev = DEVICE_DT_GET_ONE(adi_admt4000);
	struct sensor_value val;
	int ret;

	if (!device_is_ready(dev)) {
		printf("Device %s is not ready\n", dev->name);
		return 0;
	}

	printf("ADMT4000 sample application\n");
	printf("Device: %s\n", dev->name);

#ifdef CONFIG_SENSOR_ASYNC_API
	printf("Mode: RTIO (async)\n\n");
#else
	printf("Mode: Polling (sync)\n\n");
#endif

	/* Read and display the current conversion mode */
	ret = sensor_attr_get(dev, (enum sensor_channel)SENSOR_CHAN_ADMT4000_ANGLE,
			      (enum sensor_attribute)SENSOR_ATTR_ADMT4000_CONV_MODE, &val);
	if (ret == 0) {
		printf("Conversion mode: %s\n", val.val1 == 0 ? "continuous" : "one-shot");
	}

	/* Read and display filter status */
	ret = sensor_attr_get(dev, (enum sensor_channel)SENSOR_CHAN_ADMT4000_ANGLE,
			      (enum sensor_attribute)SENSOR_ATTR_ADMT4000_FILTER_ENABLE, &val);
	if (ret == 0) {
		printf("Angle filter: %s\n", val.val1 ? "enabled" : "disabled");
	}

	/* Pulse the GMR coil to perform a magnetic reset (requires coil-rs-gpios). */
	ret = sensor_attr_set(dev, (enum sensor_channel)SENSOR_CHAN_ADMT4000_ANGLE,
			      (enum sensor_attribute)SENSOR_ATTR_ADMT4000_COIL_RESET, &val);
	if (ret == 0) {
		printf("Magnetic (coil) reset: done\n");
	} else if (ret == -ENODEV) {
		printf("Magnetic (coil) reset: skipped (no coil-rs-gpios)\n");
	} else {
		printf("Magnetic (coil) reset: failed (%d)\n", ret);
	}

	printf("\n");

	while (1) {
#ifdef CONFIG_SENSOR_ASYNC_API
		sample_read_rtio(dev);
#else
		sample_read_polling(dev);
#endif
		k_sleep(SAMPLE_INTERVAL);
	}

	return 0;
}
