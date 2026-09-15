/*
 * Copyright (c) 2026 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/rtio/rtio.h>

#include <lvgl.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(vl53l5cx_sample, LOG_LEVEL_INF);

#define SENSOR_GRID_W   8
#define SENSOR_GRID_H   8
#define SENSOR_CELL_N   (SENSOR_GRID_W * SENSOR_GRID_H)

#define DIST_MIN_M      0.0f
#define DIST_MAX_M      4.0f

#define UI_CELL_SIZE    50
#define UI_CELL_GAP     2

#define RAW_FRAME_MAX   256

static const struct device *const sensor_dev = DEVICE_DT_GET_OR_NULL(DT_NODELABEL(vl53l5cx));

static lv_obj_t *cells[SENSOR_CELL_N];

/*
 * map distance in meters to gray color
 * 0.0m -> black, 4.0m -> white
 */
static lv_color_t distance_to_gray(float d_m)
{
	if (d_m < DIST_MIN_M) {
		d_m = DIST_MIN_M;
	} else if (d_m > DIST_MAX_M) {
		d_m = DIST_MAX_M;
	}

	uint8_t gray = (uint8_t)((d_m - DIST_MIN_M) * 255.0f / (DIST_MAX_M - DIST_MIN_M));

	return lv_color_make(gray, gray, gray);
}

static void create_ui(void)
{
	lv_obj_t *scr = lv_scr_act();

	lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
	lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

	int grid_w_px = SENSOR_GRID_W * UI_CELL_SIZE + (SENSOR_GRID_W - 1) * UI_CELL_GAP;
	int grid_h_px = SENSOR_GRID_H * UI_CELL_SIZE + (SENSOR_GRID_H - 1) * UI_CELL_GAP;

	lv_obj_t *cont = lv_obj_create(scr);

	lv_obj_set_size(cont, grid_w_px, grid_h_px);
	lv_obj_center(cont);
	lv_obj_set_style_pad_all(cont, 0, 0);
	lv_obj_set_style_border_width(cont, 0, 0);
	lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);
	lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);

	for (int y = 0; y < SENSOR_GRID_H; y++) {
		for (int x = 0; x < SENSOR_GRID_W; x++) {
			int idx = y * SENSOR_GRID_W + x;

			lv_obj_t *cell = lv_obj_create(cont);

			lv_obj_set_size(cell, UI_CELL_SIZE, UI_CELL_SIZE);
			lv_obj_set_pos(cell,
				       x * (UI_CELL_SIZE + UI_CELL_GAP),
				       y * (UI_CELL_SIZE + UI_CELL_GAP));

			lv_obj_set_style_radius(cell, 0, 0);
			lv_obj_set_style_border_width(cell, 1, 0);
			lv_obj_set_style_border_color(cell, lv_color_make(50, 50, 50), 0);
			lv_obj_set_style_pad_all(cell, 0, 0);
			lv_obj_set_style_bg_opa(cell, LV_OPA_COVER, 0);
			lv_obj_set_style_bg_color(cell, lv_color_black(), 0);

			cells[idx] = cell;
		}
	}
}

static float q29_to_float(int32_t q29)
{
	return (float)q29 / (float)(1 << 29);
}

void sensor_processing_callback(int result, uint8_t *buf, uint32_t buf_len, void *userdata)
{
	const struct sensor_decoder_api *decoder;
	struct sensor_chan_spec ch = {
		.chan_type = SENSOR_CHAN_DISTANCE,
		.chan_idx = 0,
	};
	struct sensor_q31_data decoded_data;
	size_t frame_size;
	size_t base_size;
	uint32_t fit;
	int ret;

	if (result != 0) {
		LOG_ERR("Failed to read data from sensor");
		return;
	}

	ret = sensor_get_decoder(sensor_dev, &decoder);
	if (ret != 0) {
		LOG_ERR("Failed to get the sensor decoder");
		return;
	}

	ret = decoder->get_size_info(ch, &base_size, &frame_size);
	if (ret != 0) {
		LOG_ERR("Couldn't get the channel distance");
		return;
	}

	if (base_size > sizeof(decoded_data)) {
		LOG_ERR("Decoded buffer is too small to decode the distance info");
		return;
	}

	for (int i = 0; i < SENSOR_CELL_N; i++) {
		uint16_t frame_count;
		lv_color_t color;

		ch.chan_idx = i;
		ret = decoder->get_frame_count(buf, ch, &frame_count);
		if (ret != 0) {
			LOG_ERR("Failed to get frame count");
			return;
		}
		fit = 0;
		ret = decoder->decode(buf, ch, &fit, 1, (void *)&decoded_data);
		if (ret == 0) {
			LOG_ERR("Nothing to decode");
			return;
		}

		/* Update the display */
		color = distance_to_gray(q29_to_float(decoded_data.readings[0].value));
		lv_obj_set_style_bg_color(cells[i], color, 0);
	}
}

/* Create a single common config for one-shot reading */
static struct sensor_chan_spec iodev_sensor_channel = {
	.chan_type = SENSOR_CHAN_DISTANCE,
};

static struct sensor_read_config iodev_sensor_read_config = {
	.sensor = sensor_dev,
	.is_streaming = false,
	.channels = &iodev_sensor_channel,
	.count = 1,
	.max = 1,
};

RTIO_IODEV_DEFINE(iodev_sensor_read, &__sensor_iodev_api, &iodev_sensor_read_config);

/* Create the RTIO context to service the reading */
RTIO_DEFINE_WITH_MEMPOOL(sensor_read_rtio, 2, 2, 32, 64, 4);

static int read_sensor(void)
{
	int ret;

	ret = sensor_read_async_mempool(&iodev_sensor_read, &sensor_read_rtio, NULL);
	if (ret != 0) {
		LOG_ERR("Failed to read sensor_read_async_mempool");
		return ret;
	}

	sensor_processing_with_callback(&sensor_read_rtio, sensor_processing_callback);

	return 0;
}

int main(void)
{
	const struct device *display_dev;
	struct sensor_value attribute = {
		.val1 = SENSOR_CELL_N,
		.val2 = 0,
	};
	int ret;

	LOG_INF("ST VL53L5CX sample app starting");

	display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	if (!device_is_ready(display_dev)) {
		LOG_ERR("Device not ready, aborting test");
		return 0;
	}

	ret = sensor_attr_set(sensor_dev, SENSOR_CHAN_DISTANCE,
			      SENSOR_ATTR_RESOLUTION, &attribute);
	if (ret != 0) {
		LOG_ERR("Failed to set sensor resolution to 8x8");
		return 0;
	}

	attribute.val1 = 60;
	ret = sensor_attr_set(sensor_dev, SENSOR_CHAN_DISTANCE,
			      SENSOR_ATTR_SAMPLING_FREQUENCY, &attribute);
	if (ret != 0) {
		LOG_ERR("Failed to set 60 for sampling frequency");
		return 0;
	}

	create_ui();

	ret = display_blanking_off(display_dev);
	if (ret != 0 && ret != -ENOSYS) {
		LOG_ERR("Failed to turn blanking off (error %d)", ret);
		return 0;
	}

	while (1) {
		ret = read_sensor();
		if (ret != 0) {
			LOG_ERR("Failed to read sensor & update display");
			return 0;
		}

		/* Let LVGL process redraws */
		lv_timer_handler();
	}
}
