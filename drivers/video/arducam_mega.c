/**
 * Copyright (c) 2023 Arducam Technology Co., Ltd. <www.arducam.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT arducam_mega

#include <zephyr/drivers/video/arducam_mega.h>

#include <zephyr/device.h>
#include <zephyr/drivers/video.h>
#include <zephyr/drivers/spi.h>

#include <zephyr/logging/log.h>

#include <zephyr/drivers/video-controls.h>
#include "video_common.h"
#include "video_ctrls.h"
#include "video_device.h"

LOG_MODULE_REGISTER(mega_camera);


#define ARDUCHIP_FIFO   0x04 /* FIFO and I2C control */
#define ARDUCHIP_FIFO_2 0x07 /* FIFO and I2C control */

#define FIFO_CLEAR_ID_MASK 0x01
#define FIFO_START_MASK    0x02

#define ARDUCHIP_TRIG 0x44 /* Trigger source */
#define VSYNC_MASK    0x01
#define SHUTTER_MASK  0x02
#define CAP_DONE_MASK 0x04

#define FIFO_SIZE1 0x45 /* Camera write FIFO size[7:0] for burst to read */
#define FIFO_SIZE2 0x46 /* Camera write FIFO size[15:8] */
#define FIFO_SIZE3 0x47 /* Camera write FIFO size[18:16] */

#define BURST_FIFO_READ  0x3C /* Burst FIFO read operation */
#define SINGLE_FIFO_READ 0x3D /* Single FIFO read operation */

/* DSP register bank FF=0x00*/
#define CAM_REG_POWER_CONTROL                 0X02
#define CAM_REG_SENSOR_RESET                  0X07
#define CAM_REG_FORMAT                        0X20
#define CAM_REG_CAPTURE_RESOLUTION            0X21
#define CAM_REG_BRIGHTNESS_CONTROL            0X22
#define CAM_REG_CONTRAST_CONTROL              0X23
#define CAM_REG_SATURATION_CONTROL            0X24
#define CAM_REG_EV_CONTROL                    0X25
#define CAM_REG_WHITEBALANCE_CONTROL          0X26
#define CAM_REG_COLOR_EFFECT_CONTROL          0X27
#define CAM_REG_SHARPNESS_CONTROL             0X28
#define CAM_REG_AUTO_FOCUS_CONTROL            0X29
#define CAM_REG_IMAGE_QUALITY                 0x2A
#define CAM_REG_EXPOSURE_GAIN_WHITEBAL_ENABLE 0X30
#define CAM_REG_MANUAL_GAIN_BIT_9_8           0X31
#define CAM_REG_MANUAL_GAIN_BIT_7_0           0X32
#define CAM_REG_MANUAL_EXPOSURE_BIT_19_16     0X33
#define CAM_REG_MANUAL_EXPOSURE_BIT_15_8      0X34
#define CAM_REG_MANUAL_EXPOSURE_BIT_7_0       0X35
#define CAM_REG_BURST_FIFO_READ_OPERATION     0X3C
#define CAM_REG_SINGLE_FIFO_READ_OPERATION    0X3D
#define CAM_REG_SENSOR_ID                     0x40
#define CAM_REG_YEAR_SDK                      0x41
#define CAM_REG_MONTH_SDK                     0x42
#define CAM_REG_DAY_SDK                       0x43
#define CAM_REG_SENSOR_STATE                  0x44
#define CAM_REG_FPGA_VERSION_NUMBER           0x49
#define CAM_REG_DEBUG_DEVICE_ADDRESS          0X0A
#define CAM_REG_DEBUG_REGISTER_HIGH           0X0B
#define CAM_REG_DEBUG_REGISTER_LOW            0X0C
#define CAM_REG_DEBUG_REGISTER_VALUE          0X0D

#define SENSOR_STATE_IDLE   (1 << 1)
#define SENSOR_RESET_ENABLE (1 << 6)

#define CTR_WHITEBALANCE 0X02
#define CTR_EXPOSURE     0X01
#define CTR_GAIN         0X00

#define AC_STACK_SIZE 4096
#define AC_PRIORITY 5

K_THREAD_STACK_DEFINE(ac_stack_area, AC_STACK_SIZE);

struct k_work_q ac_work_q;

/**
 * @struct mega_sdk_data
 * @brief Basic information of the camera firmware
 */
struct mega_sdk_data {
	uint8_t year;
	uint8_t month;
	uint8_t day;
	uint8_t version;
};

struct arducam_mega_config {
	struct spi_dt_spec bus;
};

struct arducam_mega_ctrls {
	struct video_ctrl reset;
	struct video_ctrl brightness;
	struct video_ctrl contrast;
	struct video_ctrl saturation;
	struct video_ctrl ev;
	struct video_ctrl whitebal;
	struct video_ctrl colorfx;
	struct video_ctrl quality;
	struct video_ctrl lowpower;
	struct video_ctrl whitebalauto;
	struct {
		struct video_ctrl enable_sharpness;
		struct video_ctrl sharpness;
	};
	struct {
		struct video_ctrl exp_auto;
		struct video_ctrl exposure;
	};
	struct {
		struct video_ctrl gain;
		struct video_ctrl gain_auto;
	};
	/* Read only registers */
	struct video_ctrl support_resolution;
	struct video_ctrl support_special_effects;
	struct video_ctrl enable_focus;
	struct video_ctrl linkfreq;
	struct video_ctrl device_address;
	struct video_ctrl camera_id;
};

struct arducam_mega_data {
	struct arducam_mega_ctrls ctrls;
	struct video_format fmt;

	const struct device *dev;
	struct k_fifo fifo_in;
	struct k_fifo fifo_out;
	struct k_work buf_work;
	struct k_timer stream_schedule_timer;
	struct k_poll_signal *signal;
	struct mega_sdk_data ver;
	uint8_t fifo_first_read;
	uint32_t fifo_length;
	uint8_t stream_on;
};

#define ARDUCAM_MEGA_VIDEO_FORMAT_CAP(width, height, format)                                   \
	{                                                                                          \
		.pixelformat = (format), .width_min = (width), .width_max = (width),               	   \
		.height_min = (height), .height_max = (height), .width_step = 0, .height_step = 0      \
	}

static struct video_format_cap fmts[] = {
	ARDUCAM_MEGA_VIDEO_FORMAT_CAP(96, 96, VIDEO_PIX_FMT_RGB565),
	ARDUCAM_MEGA_VIDEO_FORMAT_CAP(128, 128, VIDEO_PIX_FMT_RGB565),
	ARDUCAM_MEGA_VIDEO_FORMAT_CAP(320, 240, VIDEO_PIX_FMT_RGB565),
	ARDUCAM_MEGA_VIDEO_FORMAT_CAP(320, 320, VIDEO_PIX_FMT_RGB565),
	ARDUCAM_MEGA_VIDEO_FORMAT_CAP(640, 480, VIDEO_PIX_FMT_RGB565),
	ARDUCAM_MEGA_VIDEO_FORMAT_CAP(1280, 720, VIDEO_PIX_FMT_RGB565),
	ARDUCAM_MEGA_VIDEO_FORMAT_CAP(1600, 1200, VIDEO_PIX_FMT_RGB565),
	ARDUCAM_MEGA_VIDEO_FORMAT_CAP(1920, 1080, VIDEO_PIX_FMT_RGB565),
	{0},
	ARDUCAM_MEGA_VIDEO_FORMAT_CAP(96, 96, VIDEO_PIX_FMT_JPEG),
	ARDUCAM_MEGA_VIDEO_FORMAT_CAP(128, 128, VIDEO_PIX_FMT_JPEG),
	ARDUCAM_MEGA_VIDEO_FORMAT_CAP(320, 240, VIDEO_PIX_FMT_JPEG),
	ARDUCAM_MEGA_VIDEO_FORMAT_CAP(320, 320, VIDEO_PIX_FMT_JPEG),
	ARDUCAM_MEGA_VIDEO_FORMAT_CAP(640, 480, VIDEO_PIX_FMT_JPEG),
	ARDUCAM_MEGA_VIDEO_FORMAT_CAP(1280, 720, VIDEO_PIX_FMT_JPEG),
	ARDUCAM_MEGA_VIDEO_FORMAT_CAP(1600, 1200, VIDEO_PIX_FMT_JPEG),
	ARDUCAM_MEGA_VIDEO_FORMAT_CAP(1920, 1080, VIDEO_PIX_FMT_JPEG),
	{0},
	ARDUCAM_MEGA_VIDEO_FORMAT_CAP(96, 96, VIDEO_PIX_FMT_YUYV),
	ARDUCAM_MEGA_VIDEO_FORMAT_CAP(128, 128, VIDEO_PIX_FMT_YUYV),
	ARDUCAM_MEGA_VIDEO_FORMAT_CAP(320, 240, VIDEO_PIX_FMT_YUYV),
	ARDUCAM_MEGA_VIDEO_FORMAT_CAP(320, 320, VIDEO_PIX_FMT_YUYV),
	ARDUCAM_MEGA_VIDEO_FORMAT_CAP(640, 480, VIDEO_PIX_FMT_YUYV),
	ARDUCAM_MEGA_VIDEO_FORMAT_CAP(1280, 720, VIDEO_PIX_FMT_YUYV),
	ARDUCAM_MEGA_VIDEO_FORMAT_CAP(1600, 1200, VIDEO_PIX_FMT_YUYV),
	ARDUCAM_MEGA_VIDEO_FORMAT_CAP(1920, 1080, VIDEO_PIX_FMT_YUYV),
	{0},
	{0},
};

#define SUPPORT_RESOLUTION_NUM 9

static uint8_t support_resolution[SUPPORT_RESOLUTION_NUM] = {
	MEGA_RESOLUTION_96X96,   MEGA_RESOLUTION_128X128, MEGA_RESOLUTION_QVGA,
	MEGA_RESOLUTION_320X320, MEGA_RESOLUTION_VGA,     MEGA_RESOLUTION_HD,
	MEGA_RESOLUTION_UXGA,    MEGA_RESOLUTION_FHD,     MEGA_RESOLUTION_NONE,
};

static int arducam_mega_write_reg(const struct spi_dt_spec *spec, uint8_t reg_addr, uint8_t value)
{
	uint8_t tries = 3;

	reg_addr |= 0x80;

	struct spi_buf tx_buf[2] = {
		{.buf = &reg_addr, .len = 1},
		{.buf = &value, .len = 1},
	};

	struct spi_buf_set tx_bufs = {.buffers = tx_buf, .count = 2};

	while (tries--) {
		if (!spi_write_dt(spec, &tx_bufs)) {
			return 0;
		}
		// If writing failed wait 5ms before next attempt 
		k_msleep(5);
	}
	LOG_ERR("failed to write 0x%x to 0x%x", value, reg_addr);

	return -1;
}

static int arducam_mega_read_reg(const struct spi_dt_spec *spec, uint8_t reg_addr)
{
	uint8_t tries = 3;
	uint8_t value;
	uint8_t ret;

	reg_addr &= 0x7F;

	struct spi_buf tx_buf[] = {
		{.buf = &reg_addr, .len = 1},
	};

	struct spi_buf_set tx_bufs = {.buffers = tx_buf, .count = 1};

	struct spi_buf rx_buf[] = {
		{.buf = &value, .len = 1},
		{.buf = &value, .len = 1},
		{.buf = &value, .len = 1},
	};

	struct spi_buf_set rx_bufs = {.buffers = rx_buf, .count = 3};

	while (tries--) {
		ret = spi_transceive_dt(spec, &tx_bufs, &rx_bufs);
		if (!ret) {
			return value;
		}
		// If reading failed wait 5ms before next attempt 
		k_msleep(5);
	}
	LOG_ERR("failed to read 0x%x register", reg_addr);

	return -1;
}

static int arducam_mega_read_block(const struct spi_dt_spec *spec, uint8_t *img_buff,
				   uint32_t img_len, uint8_t first)
{
	uint8_t cmd_fifo_read[] = {BURST_FIFO_READ, 0x00};
	uint8_t buf_len = first == 0 ? 1 : 2;

	struct spi_buf tx_buf[] = {
		{.buf = cmd_fifo_read, .len = buf_len},
	};

	struct spi_buf_set tx_bufs = {.buffers = tx_buf, .count = 1};

	struct spi_buf rx_buf[2] = {
		{.buf = cmd_fifo_read, .len = buf_len},
		{.buf = img_buff, .len = img_len},
	};
	struct spi_buf_set rx_bufs = {.buffers = rx_buf, .count = 2};

	return spi_transceive_dt(spec, &tx_bufs, &rx_bufs);
}

static int arducam_mega_await_bus_idle(const struct spi_dt_spec *spec, uint8_t tries)
{
	while ((arducam_mega_read_reg(spec, CAM_REG_SENSOR_STATE) & 0x03) != SENSOR_STATE_IDLE) {
		if (tries-- == 0) {
			return -1;
		}
		k_msleep(2);
	}

	return 0;
}

static int arducam_mega_write_reg_wait(const struct arducam_mega_bus *bus,
                                       uint16_t reg,
                                       uint8_t value,
                                       uint32_t idle_timeout_ms,
                                       const char *label)
{
    int ret;

    ret = arducam_mega_await_bus_idle(bus, idle_timeout_ms);
    if (ret < 0) {
        LOG_ERR("Bus idle wait failed before writing %s", label);
        return ret;
    }

    ret = arducam_mega_write_reg(bus, reg, value);
    if (ret < 0) {
        LOG_ERR("Failed to write %s (reg 0x%04x)", label, reg);
    }

    return ret;
}

static int arducam_mega_set_brightness(const struct device *dev, enum MEGA_BRIGHTNESS_LEVEL level)
{
	const struct arducam_mega_config *cfg = dev->config;

    return arducam_mega_write_reg_wait(&cfg->bus,
                                       CAM_REG_BRIGHTNESS_CONTROL,
                                       level,
                                       3,
                                       "brightness level");
}

static int arducam_mega_set_saturation(const struct device *dev, enum MEGA_SATURATION_LEVEL level)
{
    const struct arducam_mega_config *cfg = dev->config;

    return arducam_mega_write_reg_wait(&cfg->bus,
                                       CAM_REG_SATURATION_CONTROL,
                                       level,
                                       3,
                                       "saturation level");
}

static int arducam_mega_set_contrast(const struct device *dev, enum MEGA_CONTRAST_LEVEL level)
{
    const struct arducam_mega_config *cfg = dev->config;

    return arducam_mega_write_reg_wait(&cfg->bus,
                                       CAM_REG_CONTRAST_CONTROL,
                                       level,
                                       3,
                                       "contrast level");
}

static int arducam_mega_set_EV(const struct device *dev, enum MEGA_EV_LEVEL level)
{
    const struct arducam_mega_config *cfg = dev->config;

    return arducam_mega_write_reg_wait(&cfg->bus,
                                       CAM_REG_EV_CONTROL,
                                       level,
                                       3,
                                       "EV level");
}

static int arducam_mega_set_sharpness(const struct device *dev, enum MEGA_SHARPNESS_LEVEL level)
{
    struct arducam_mega_data *drv_data = dev->data;
    struct arducam_mega_ctrls *drv_ctrls = &drv_data->ctrls;
    const struct arducam_mega_config *cfg = dev->config;

    if (!drv_ctrls->enable_sharpness.val) {
        LOG_ERR("This device does not support setting sharpness.");
        return -ENOTSUP;
    }

    return arducam_mega_write_reg_wait(&cfg->bus,
                                       CAM_REG_SHARPNESS_CONTROL,
                                       level,
                                       3,
                                       "sharpness level");
}

static int arducam_mega_set_special_effects(const struct device *dev, enum MEGA_COLOR_FX effect)
{
    const struct arducam_mega_config *cfg = dev->config;

    return arducam_mega_write_reg_wait(&cfg->bus,
                                       CAM_REG_COLOR_EFFECT_CONTROL,
                                       effect,
                                       3,
                                       "special effects");
}

static int arducam_mega_set_output_format(const struct device *dev, int output_format)
{
    const struct arducam_mega_config *cfg = dev->config;
    uint8_t format_val;

    switch (output_format) {
    case VIDEO_PIX_FMT_JPEG:
        format_val = MEGA_PIXELFORMAT_JPG;
        break;
    case VIDEO_PIX_FMT_RGB565:
        format_val = MEGA_PIXELFORMAT_RGB565;
        break;
    case VIDEO_PIX_FMT_YUYV:
        format_val = MEGA_PIXELFORMAT_YUV;
        break;
    default:
        LOG_ERR("Image format not supported");
        return -ENOTSUP;
    }

    int ret = arducam_mega_write_reg_wait(&cfg->bus,
                                       CAM_REG_FORMAT,
                                       format_val,
                                       3,
                                       "output format");
    if (ret < 0)
        return ret;

    ret = arducam_mega_await_bus_idle(&cfg->bus, 30);
    if (ret < 0) {
        LOG_ERR("Bus idle wait failed after setting output format");
    }

	return ret;
}

static int arducam_mega_set_JPEG_quality(const struct device *dev, enum MEGA_IMAGE_QUALITY qc)
{
    const struct arducam_mega_config *cfg = dev->config;
    struct arducam_mega_data *drv_data = dev->data;

    LOG_DBG("%s: %d", __func__, qc);

    if (drv_data->fmt.pixelformat != VIDEO_PIX_FMT_JPEG) {
        LOG_ERR("Image format does not support setting JPEG quality");
        return -ENOTSUP;
    }

    return arducam_mega_write_reg_wait(&cfg->bus,
                                       CAM_REG_IMAGE_QUALITY,
                                       qc,
                                       3,
                                       "JPEG quality");
}

static int arducam_mega_set_white_bal_enable(const struct device *dev, int enable)
{
	const struct arducam_mega_config *cfg = dev->config;

    uint8_t reg = CTR_WHITEBALANCE;
    if (enable) {
        reg |= 0x80;
    }

    int ret = arducam_mega_write_reg_wait(&cfg->bus,
                                       CAM_REG_EXPOSURE_GAIN_WHITEBAL_ENABLE,
                                       reg,
                                       3,
                                       "white balance enable");
    if (ret < 0)
        return ret;

    ret = arducam_mega_await_bus_idle(&cfg->bus, 10);
    if (ret < 0) {
        LOG_ERR("Bus idle wait failed after setting white balance");
    }

	return ret;
}

static int arducam_mega_set_white_bal(const struct device *dev, enum MEGA_EV_LEVEL level)
{
    const struct arducam_mega_config *cfg = dev->config;

    return arducam_mega_write_reg_wait(&cfg->bus,
                                       CAM_REG_WHITEBALANCE_CONTROL,
                                       level,
                                       3,
                                       "white balance level");
}

static int arducam_mega_set_gain_enable(const struct device *dev, int enable)
{
    const struct arducam_mega_config *cfg = dev->config;

    uint8_t reg = CTR_GAIN;
    if (enable) {
        reg |= 0x80;
    }

    int ret = arducam_mega_write_reg_wait(&cfg->bus,
                                       CAM_REG_EXPOSURE_GAIN_WHITEBAL_ENABLE,
                                       reg,
                                       3,
                                       "gain enable");
    if (ret < 0)
        return ret;

    ret = arducam_mega_await_bus_idle(&cfg->bus, 10);
    if (ret < 0) {
        LOG_ERR("Bus idle wait failed after setting gain enable");
    }

	return ret;
}

static int arducam_mega_set_lowpower_enable(const struct device *dev, int enable)
{
    const struct arducam_mega_config *cfg = dev->config;
    const struct arducam_mega_data *drv_data = dev->data;
    struct arducam_mega_ctrls *drv_ctrls = &drv_data->ctrls;

    if (drv_ctrls->camera_id.val == ARDUCAM_SENSOR_5MP_2 ||
        drv_ctrls->camera_id.val == ARDUCAM_SENSOR_3MP_2) {
        enable = !enable;
    }

    uint8_t reg_val = enable ? 0x07 : 0x05;

    return arducam_mega_write_reg_wait(&cfg->bus,
                                       CAM_REG_POWER_CONTROL,
                                       reg_val,
                                       3,
                                       "lowpower enable");
}

static int arducam_mega_set_gain(const struct device *dev, uint16_t value)
{
	const struct arducam_mega_config *cfg = dev->config;
    int ret;

    ret = arducam_mega_write_reg_wait(&cfg->bus,
                                       CAM_REG_MANUAL_GAIN_BIT_9_8,
                                       (value >> 8) & 0xFF,
                                       3,
                                       "gain bits [9:8]");
    if (ret < 0)
        return ret;

    ret = arducam_mega_write_reg_wait(&cfg->bus,
                                       CAM_REG_MANUAL_GAIN_BIT_7_0,
                                       value & 0xFF,
                                       10,
                                       "gain bits [7:0]");
    if (ret < 0)
        return ret;

    ret = arducam_mega_await_bus_idle(&cfg->bus, 10);
    if (ret < 0) {
        LOG_ERR("Bus idle wait failed after setting gain");
    }

    return ret;
}

static int arducam_mega_set_exposure_enable(const struct device *dev, int enable)
{
	const struct arducam_mega_config *cfg = dev->config;

    uint8_t reg = CTR_EXPOSURE;
    if (enable) {
        reg |= 0x80;
    }

    int ret = arducam_mega_write_reg_wait(&cfg->bus,
                                       CAM_REG_EXPOSURE_GAIN_WHITEBAL_ENABLE,
                                       reg,
                                       3,
                                       "exposure enable");
    if (ret < 0)
        return ret;

    ret = arducam_mega_await_bus_idle(&cfg->bus, 10);
    if (ret < 0) {
        LOG_ERR("Bus idle wait failed after setting exposure enable");
    }

	return ret;
}

static int arducam_mega_set_exposure(const struct device *dev, uint32_t value)
{
    const struct arducam_mega_config *cfg = dev->config;
    int ret;

    ret = arducam_mega_write_reg_wait(&cfg->bus,
                                       CAM_REG_MANUAL_EXPOSURE_BIT_19_16,
                                       (value >> 16) & 0xFF,
                                       3,
                                       "exposure bits [19:16]");
    if (ret < 0)
        return ret;

    ret = arducam_mega_write_reg_wait(&cfg->bus,
                                       CAM_REG_MANUAL_EXPOSURE_BIT_15_8,
                                       (value >> 8) & 0xFF,
                                       10,
                                       "exposure bits [15:8]");
    if (ret < 0)
        return ret;

    ret = arducam_mega_write_reg_wait(&cfg->bus,
                                       CAM_REG_MANUAL_EXPOSURE_BIT_7_0,
                                       value & 0xFF,
                                       10,
                                       "exposure bits [7:0]");
    if (ret < 0)
        return ret;

    ret = arducam_mega_await_bus_idle(&cfg->bus, 10);
    if (ret < 0) {
        LOG_ERR("Bus idle wait failed after setting exposure");
    }

    return ret;
}

static int arducam_mega_set_resolution(const struct device *dev, enum MEGA_RESOLUTION resolution)
{
    const struct arducam_mega_config *cfg = dev->config;

    int ret = arducam_mega_write_reg_wait(&cfg->bus,
                                       CAM_REG_CAPTURE_RESOLUTION,
                                       resolution,
                                       10,
                                       "capture resolution");
    if (ret < 0)
        return ret;

    ret = arducam_mega_await_bus_idle(&cfg->bus, 10);
    if (ret < 0) {
        LOG_ERR("Bus idle wait failed after setting resolution");
    }

	return ret;
}

static int arducam_mega_check_connection(const struct device *dev)
{
	uint8_t cam_id;
	const struct arducam_mega_config *cfg = dev->config;
	struct arducam_mega_data *drv_data = dev->data;

	int ret = arducam_mega_await_bus_idle(&cfg->bus, 255);
    if (ret < 0) {
        LOG_ERR("Bus idle wait failed during connection check");
        return ret;
    }

	cam_id = arducam_mega_read_reg(&cfg->bus, CAM_REG_SENSOR_ID);
	if (!(cam_id & 0x87)) {
		LOG_ERR("arducam mega not detected, 0x%x\n", cam_id);
		return -ENODEV;
	}

	switch (cam_id) {
		case ARDUCAM_SENSOR_5MP_1:
			fmts[8] = (struct video_format_cap)ARDUCAM_MEGA_VIDEO_FORMAT_CAP(
				2592, 1944, VIDEO_PIX_FMT_RGB565);
			fmts[17] = (struct video_format_cap)ARDUCAM_MEGA_VIDEO_FORMAT_CAP(
				2592, 1944, VIDEO_PIX_FMT_JPEG);
			fmts[26] = (struct video_format_cap)ARDUCAM_MEGA_VIDEO_FORMAT_CAP(
				2592, 1944, VIDEO_PIX_FMT_YUYV);
			support_resolution[8] = MEGA_RESOLUTION_WQXGA2;

			drv_data->ctrls.support_resolution.val = SUPPORT_RESOLUTION_5M;
			drv_data->ctrls.support_special_effects.val = SUPPORT_SP_EFF_5M;
			drv_data->ctrls.enable_focus.val = ENABLE_FOCUS_5M;
			drv_data->ctrls.enable_sharpness.val = ENABLE_SHARPNESS_5M;
			drv_data->ctrls.device_address.val = DEVICE_ADDRESS;
			break;
		case ARDUCAM_SENSOR_3MP_1:
			fmts[8] = (struct video_format_cap)ARDUCAM_MEGA_VIDEO_FORMAT_CAP(
				2048, 1536, VIDEO_PIX_FMT_RGB565);
			fmts[17] = (struct video_format_cap)ARDUCAM_MEGA_VIDEO_FORMAT_CAP(
				2048, 1536, VIDEO_PIX_FMT_JPEG);
			fmts[26] = (struct video_format_cap)ARDUCAM_MEGA_VIDEO_FORMAT_CAP(
				2048, 1536, VIDEO_PIX_FMT_YUYV);
			support_resolution[8] = MEGA_RESOLUTION_QXGA;
			drv_data->ctrls.support_resolution.val = SUPPORT_RESOLUTION_3M;
			drv_data->ctrls.support_special_effects.val = SUPPORT_SP_EFF_3M;
			drv_data->ctrls.enable_focus.val = ENABLE_FOCUS_3M;
			drv_data->ctrls.enable_sharpness.val = ENABLE_SHARPNESS_3M;
			drv_data->ctrls.device_address.val = DEVICE_ADDRESS;
			break;
		case ARDUCAM_SENSOR_5MP_2:
			fmts[8] = (struct video_format_cap)ARDUCAM_MEGA_VIDEO_FORMAT_CAP(
				2592, 1936, VIDEO_PIX_FMT_RGB565);
			fmts[17] = (struct video_format_cap)ARDUCAM_MEGA_VIDEO_FORMAT_CAP(
				2592, 1936, VIDEO_PIX_FMT_JPEG);
			fmts[26] = (struct video_format_cap)ARDUCAM_MEGA_VIDEO_FORMAT_CAP(
				2592, 1936, VIDEO_PIX_FMT_YUYV);
			support_resolution[8] = MEGA_RESOLUTION_WQXGA2;
			drv_data->ctrls.support_resolution.val = SUPPORT_RESOLUTION_5M;
			drv_data->ctrls.support_special_effects.val = SUPPORT_SP_EFF_5M;
			drv_data->ctrls.enable_focus.val = ENABLE_FOCUS_5M;
			drv_data->ctrls.enable_sharpness.val = ENABLE_SHARPNESS_5M;
			drv_data->ctrls.device_address.val = DEVICE_ADDRESS;
			break;
		case ARDUCAM_SENSOR_3MP_2:
			fmts[8] = (struct video_format_cap)ARDUCAM_MEGA_VIDEO_FORMAT_CAP(
				2048, 1536, VIDEO_PIX_FMT_RGB565);
			fmts[17] = (struct video_format_cap)ARDUCAM_MEGA_VIDEO_FORMAT_CAP(
				2048, 1536, VIDEO_PIX_FMT_JPEG);
			fmts[26] = (struct video_format_cap)ARDUCAM_MEGA_VIDEO_FORMAT_CAP(
				2048, 1536, VIDEO_PIX_FMT_YUYV);
			support_resolution[8] = MEGA_RESOLUTION_QXGA;

			drv_data->ctrls.support_resolution.val = SUPPORT_RESOLUTION_3M;
			drv_data->ctrls.support_special_effects.val = SUPPORT_SP_EFF_3M;
			drv_data->ctrls.enable_focus.val = ENABLE_FOCUS_3M;
			drv_data->ctrls.enable_sharpness.val = ENABLE_SHARPNESS_3M;
			drv_data->ctrls.device_address.val = DEVICE_ADDRESS;
			break;
		default:
			return -ENODEV;
	}
	drv_data->ctrls.camera_id.val = cam_id;

	return ret;
}

static int arducam_mega_set_fmt(const struct device *dev,
				struct video_format *fmt)
{
	struct arducam_mega_data *drv_data = dev->data;
	uint16_t width, height;
	int ret = 0;
	int i = 0;

	// We only support RGB565, JPEG, and YUYV pixel formats 
	if (fmt->pixelformat != VIDEO_PIX_FMT_RGB565 && fmt->pixelformat != VIDEO_PIX_FMT_JPEG &&
	    fmt->pixelformat != VIDEO_PIX_FMT_YUYV) {
		LOG_ERR("Arducam Mega camera only supports RGB565, JPG, and YUYV pixel formats!");
		return -ENOTSUP;
	}

	width = fmt->width;
	height = fmt->height;

	if (!memcmp(&drv_data->fmt, fmt, sizeof(drv_data->fmt))) {
		// nothing to do 
		return 0;
	}

	// Check if camera is capable of handling given format 
	while (fmts[i].pixelformat) {
		if (fmts[i].width_min == width && fmts[i].height_min == height &&
		    fmts[i].pixelformat == fmt->pixelformat) {
			// Set output format 
			int ret = arducam_mega_set_output_format(dev, fmt->pixelformat);
			if (ret < 0) {
                LOG_ERR("Failed to set output format");
                return ret;
            }
			// Set window size 
			ret = arducam_mega_set_resolution(
				dev, support_resolution[i % SUPPORT_RESOLUTION_NUM]);
		    if (ret < 0) {
                LOG_ERR("Failed to set resolution");
                return ret;
            }
			drv_data->fmt = *fmt;
			drv_data->fmt.pitch = drv_data->fmt.width * 2;
			return ret;
		}
		i++;
	}
	// Camera is not capable of handling given format 
	LOG_ERR("Image resolution not supported\n");
	return -ENOTSUP;
}

static int arducam_mega_get_fmt(const struct device *dev,
				struct video_format *fmt)
{
	struct arducam_mega_data *drv_data = dev->data;

	*fmt = drv_data->fmt;

	return 0;
}

static void on_stream_schedule_timer_func(struct k_timer *timer)
{
	struct arducam_mega_data *drv_data = timer->user_data;

	k_work_submit_to_queue(&ac_work_q, &drv_data->buf_work);
}

static int arducam_mega_stream_start(const struct device *dev, bool enable, enum video_buf_type type)
{
	struct arducam_mega_data *drv_data = dev->data;

	if (drv_data->stream_on) {
		return 0;
	}

	if(enable){
		drv_data->stream_on = 1;
		drv_data->fifo_length = 0;
		k_timer_start(&drv_data->stream_schedule_timer, K_MSEC(30), K_MSEC(30));
	}else{
		drv_data->stream_on = 0;
		k_timer_stop(&drv_data->stream_schedule_timer);
	}
	
	return 0;
}

static int arducam_mega_stream_stop(const struct device *dev)
{
	struct arducam_mega_data *drv_data = dev->data;

	drv_data->stream_on = 0;

	k_timer_stop(&drv_data->stream_schedule_timer);

	return 0;
}

static int arducam_mega_flush(const struct device *dev, bool cancel)
{
	struct arducam_mega_data *drv_data = dev->data;
	struct video_buffer *vbuf;

	// Clear fifo cache 
	while (!k_fifo_is_empty(&drv_data->fifo_out)) {
		vbuf = k_fifo_get(&drv_data->fifo_out, K_USEC(10));
		if (vbuf != NULL) {
			k_fifo_put(&drv_data->fifo_in, vbuf);
		}
	}
	return 0;
}

static int arducam_mega_soft_reset(const struct device *dev)
{
	const struct arducam_mega_config *cfg = dev->config;
	struct arducam_mega_data *drv_data = dev->data;

	if (drv_data->stream_on) {
		arducam_mega_stream_stop(dev);
	}
	// Initiate system reset //
	int ret = arducam_mega_write_reg(&cfg->bus, CAM_REG_SENSOR_RESET, SENSOR_RESET_ENABLE);
	k_msleep(1000);

	return ret;
}

static int arducam_mega_capture(const struct device *dev, uint32_t *length)
{
	const struct arducam_mega_config *cfg = dev->config;
	struct arducam_mega_data *drv_data = dev->data;
	uint8_t tries = 200;

	arducam_mega_write_reg(&cfg->bus, ARDUCHIP_FIFO, FIFO_CLEAR_ID_MASK);
	arducam_mega_write_reg(&cfg->bus, ARDUCHIP_FIFO, FIFO_START_MASK);

	do {
		if (tries-- == 0) {
			LOG_ERR("Capture timeout!");
			return -1;
		}
		k_msleep(2);
	} while (!(arducam_mega_read_reg(&cfg->bus, ARDUCHIP_TRIG) & CAP_DONE_MASK));

	drv_data->fifo_length = arducam_mega_read_reg(&cfg->bus, FIFO_SIZE1);
	drv_data->fifo_length |= (arducam_mega_read_reg(&cfg->bus, FIFO_SIZE2) << 8);
	drv_data->fifo_length |= (arducam_mega_read_reg(&cfg->bus, FIFO_SIZE3) << 16);

	drv_data->fifo_first_read = 1;
	*length = drv_data->fifo_length;
	return 0;
}

static int arducam_mega_fifo_read(const struct device *dev, struct video_buffer *buf)
{
	int ret;
	int32_t rlen;
	const struct arducam_mega_config *cfg = dev->config;
	struct arducam_mega_data *drv_data = dev->data;

	rlen = buf->size > drv_data->fifo_length ? drv_data->fifo_length : buf->size;

	LOG_DBG("read fifo :%u. - fifo_length %u", buf->size, drv_data->fifo_length);

	ret = arducam_mega_read_block(&cfg->bus, buf->buffer, rlen, drv_data->fifo_first_read);

	if (ret == 0) {
		drv_data->fifo_length -= rlen;
		buf->bytesused = rlen;
		if (drv_data->fifo_first_read) {
			drv_data->fifo_first_read = 0;
		}
	}

	return ret;
}

static void __buffer_work(struct k_work *work)
{
	struct k_work *dwork = work;
	struct arducam_mega_data *drv_data =
		CONTAINER_OF(dwork, struct arducam_mega_data, buf_work);
	static uint32_t f_timestamp, f_length;
	struct video_buffer *vbuf;

	vbuf = k_fifo_get(&drv_data->fifo_in, K_FOREVER);


	if (vbuf == NULL) {
		return;
	}

	if (drv_data->fifo_length == 0) {
		arducam_mega_capture(drv_data->dev, &f_length);
		f_timestamp = k_uptime_get_32();
	}

	arducam_mega_fifo_read(drv_data->dev, vbuf);

	if (drv_data->fifo_length == 0) {
		vbuf->flags = VIDEO_BUF_EOF;
	} else {
		vbuf->flags = VIDEO_BUF_FRAG;
		k_work_submit_to_queue(&ac_work_q, &drv_data->buf_work);
	}

	vbuf->timestamp = f_timestamp;
	vbuf->bytesframe = f_length;
	k_fifo_put(&drv_data->fifo_out, vbuf);

	k_yield();
}

static int arducam_mega_enqueue(const struct device *dev,
				struct video_buffer *vbuf)
{
	struct arducam_mega_data *data = dev->data;

	k_fifo_put(&data->fifo_in, vbuf);

	LOG_DBG("enqueue buffer %p", vbuf->buffer);

	return 0;
}

static int arducam_mega_dequeue(const struct device *dev,
				struct video_buffer **vbuf, k_timeout_t timeout)
{
	struct arducam_mega_data *data = dev->data;

	*vbuf = k_fifo_get(&data->fifo_out, timeout);

	LOG_DBG("dequeue buffer %p", (*vbuf)->buffer);

	if (*vbuf == NULL) {
		return -EAGAIN;
	}

	return 0;
}

static int arducam_mega_get_caps(const struct device *dev,
				 struct video_caps *caps)
{
	/* Arducam in capture mode needs only one buffer allocated before starting */
	caps->min_vbuf_count = 1;
	/* Arducam produces full frames */
	caps->min_line_count = caps->max_line_count = LINE_COUNT_HEIGHT;
	caps->format_caps = fmts;
	return 0;
}

static int arducam_mega_set_ctrl(const struct device *dev, uint32_t id)
{
	struct arducam_mega_data *drv_data = dev->data;

	switch (id) {
	case VIDEO_CID_EXPOSURE_AUTO:
		return arducam_mega_set_exposure_enable(dev, drv_data->ctrls.exp_auto.val);
	case VIDEO_CID_EXPOSURE:
		return arducam_mega_set_exposure(dev, drv_data->ctrls.exposure.val);
	case VIDEO_CID_AUTOGAIN:
		return arducam_mega_set_gain_enable(dev, drv_data->ctrls.gain_auto.val);
	case VIDEO_CID_GAIN:
		return arducam_mega_set_gain(dev, drv_data->ctrls.gain.val);
	case VIDEO_CID_BRIGHTNESS:
		return arducam_mega_set_brightness(dev, drv_data->ctrls.brightness.val);
	case VIDEO_CID_SATURATION:
		return arducam_mega_set_saturation(dev, drv_data->ctrls.saturation.val);
	case VIDEO_CID_AUTO_WHITE_BALANCE:
		return arducam_mega_set_white_bal_enable(dev, drv_data->ctrls.whitebalauto.val);
	case VIDEO_CID_WHITE_BALANCE_TEMPERATURE:
		return arducam_mega_set_white_bal(dev, drv_data->ctrls.whitebal.val);
	case VIDEO_CID_CONTRAST:
		return arducam_mega_set_contrast(dev, drv_data->ctrls.contrast.val);
	case VIDEO_CID_JPEG_COMPRESSION_QUALITY:
		return arducam_mega_set_JPEG_quality(dev, drv_data->ctrls.quality.val);
	case VIDEO_CID_ARDUCAM_EV:
		return arducam_mega_set_EV(dev, drv_data->ctrls.ev.val);
	case VIDEO_CID_SHARPNESS:
		return arducam_mega_set_sharpness(dev, drv_data->ctrls.sharpness.val);
	case VIDEO_CID_ARDUCAM_COLOR_FX:
		return arducam_mega_set_special_effects(dev, drv_data->ctrls.support_special_effects.val);
	case VIDEO_CID_ARDUCAM_LOWPOWER:
		return arducam_mega_set_lowpower_enable(dev, drv_data->ctrls.lowpower.val);
	case VIDEO_CID_ARDUCAM_RESET: {
		int ret;
		drv_data->ctrls.reset.val = 0;
		ret = arducam_mega_soft_reset(dev);
		if (ret < 0)
			return ret;
		return arducam_mega_check_connection(dev);
	}
	default:
		return -ENOTSUP;
	}
}

static const struct video_driver_api arducam_mega_driver_api = {
	.set_format = arducam_mega_set_fmt,
	.get_format = arducam_mega_get_fmt,
	.set_ctrl = arducam_mega_set_ctrl,
	.get_caps = arducam_mega_get_caps,
	.set_stream = arducam_mega_stream_start,
	.get_caps = arducam_mega_get_caps,
	.flush = arducam_mega_flush,
	.enqueue = arducam_mega_enqueue,
	.dequeue = arducam_mega_dequeue
};

#define ARDUCAM_MEGA_640_480_LINK_FREQ	120000000
#define ARDUCAM_MEGA_640_480_LINK_FREQ_ID	0
#define ARDUCAM_MEGA_1600_1200_LINK_FREQ	240000000
#define ARDUCAM_MEGA_1600_1200_LINK_FREQ_ID	1
const int64_t arducam_mega_link_frequency[] = {
	ARDUCAM_MEGA_640_480_LINK_FREQ, ARDUCAM_MEGA_1600_1200_LINK_FREQ,
};

static int arducam_mega_controls(const struct device *dev)
{
	int ret;
	struct arducam_mega_data *drv_data = dev->data;
	struct arducam_mega_ctrls *ctrls = &drv_data->ctrls;

	ret = video_init_ctrl(&ctrls->reset, dev, VIDEO_CID_ARDUCAM_RESET,
			      (struct video_ctrl_range){.min = 0, .max = 1, .step = 1, .def = 0});
	if (ret < 0) {
		return ret;
	}	
	ret = video_init_ctrl(&ctrls->brightness, dev, VIDEO_CID_BRIGHTNESS,
			      (struct video_ctrl_range){.min = 0, .max = 8, .step = 1, .def = 0});
	if (ret < 0) {
		return ret;
	}
	ret = video_init_ctrl(&ctrls->contrast, dev, VIDEO_CID_CONTRAST,
			      (struct video_ctrl_range){.min = 0, .max = 6, .step = 1, .def = 0});
	if (ret < 0) {
		return ret;
	}
	ret = video_init_ctrl(&ctrls->saturation, dev, VIDEO_CID_SATURATION,
			      (struct video_ctrl_range){.min = 0, .max = 6, .step = 1, .def = 0});
	if (ret < 0) {
		return ret;
	}
	ret = video_init_ctrl(&ctrls->ev, dev, VIDEO_CID_ARDUCAM_EV,
			      (struct video_ctrl_range){.min = 0, .max = 6, .step = 1, .def = 0});
	if (ret < 0) {
		return ret;
	}
	ret = video_init_ctrl(&ctrls->whitebal, dev, VIDEO_CID_WHITE_BALANCE_TEMPERATURE,
			      (struct video_ctrl_range){.min = 0, .max = 4, .step = 1, .def = 0});
	if (ret < 0) {
		return ret;
	}
	ret = video_init_ctrl(&ctrls->colorfx, dev, VIDEO_CID_ARDUCAM_COLOR_FX,
			      (struct video_ctrl_range){.min = 0, .max = 9, .step = 1, .def = 0});
	if (ret < 0) {
		return ret;
	}
	ret = video_init_ctrl(&ctrls->exp_auto, dev, VIDEO_CID_EXPOSURE_AUTO,
			      (struct video_ctrl_range){.min = 0, .max = 1, .step = 1, .def = 0});
	if (ret < 0) {
		return ret;
	}
	ret = video_init_ctrl(&ctrls->gain_auto, dev, VIDEO_CID_AUTOGAIN,
			      (struct video_ctrl_range){.min = 0, .max = 1, .step = 1, .def = 0});
	if (ret < 0) {
		return ret;
	}
	ret = video_init_ctrl(&ctrls->whitebalauto, dev, VIDEO_CID_AUTO_WHITE_BALANCE,
			      (struct video_ctrl_range){.min = 0, .max = 1, .step = 1, .def = 0});
	if (ret < 0) {
		return ret;
	}
	ret = video_init_ctrl(&ctrls->sharpness, dev, VIDEO_CID_SHARPNESS,
			      (struct video_ctrl_range){.min = 0, .max = 8, .step = 1, .def = 0});
	if (ret < 0) {
		return ret;
	}
	ret = video_init_ctrl(&ctrls->gain, dev, VIDEO_CID_GAIN,
			      (struct video_ctrl_range){.min = GAIN_MIN, .max = GAIN_MAX, .step = 1, .def = 0});
	if (ret < 0) {
		return ret;
	}
	ret = video_init_ctrl(&ctrls->exposure, dev, VIDEO_CID_EXPOSURE,
			      (struct video_ctrl_range){.min = GAIN_MIN, .max = EXPOSURE_MAX, .step = 1, .def = 0});
	if (ret < 0) {
		return ret;
	}
	ret = video_init_ctrl(&ctrls->quality, dev, VIDEO_CID_JPEG_COMPRESSION_QUALITY,
			      (struct video_ctrl_range){.min = 0, .max = 65535, .step = 1, .def = 0});
	if (ret < 0) {
		return ret;
	}
	ret = video_init_ctrl(&ctrls->lowpower, dev, VIDEO_CID_ARDUCAM_LOWPOWER,
			      (struct video_ctrl_range){.min = 0, .max = 65535, .step = 1, .def = 0});
	if (ret < 0) {
		return ret;
	}
	/* Read only controls */
	ret = video_init_ctrl(&ctrls->support_resolution, dev, VIDEO_CID_ARDUCAM_SUPP_RES,
			      (struct video_ctrl_range){.min = 0, .max = 65535, .step = 1, .def = 0});
	ctrls->support_resolution.flags |= VIDEO_CTRL_FLAG_READ_ONLY;
	if (ret < 0) {
		return ret;
	}
	ret = video_init_ctrl(&ctrls->support_special_effects, dev, VIDEO_CID_ARDUCAM_SUPP_SP_EFF,
			      (struct video_ctrl_range){.min = 0, .max = 65535, .step = 1, .def = 0});
	ctrls->support_special_effects.flags |= VIDEO_CTRL_FLAG_READ_ONLY;
	if (ret < 0) {
		return ret;
	}
	ret = video_init_ctrl(&ctrls->enable_focus, dev, VIDEO_CID_ARDUCAM_EN_FOCUS,
			      (struct video_ctrl_range){.min = 0, .max = 65535, .step = 1, .def = 0});
	ctrls->enable_focus.flags |= VIDEO_CTRL_FLAG_READ_ONLY;
	if (ret < 0) {
		return ret;
	}
	ret = video_init_ctrl(&ctrls->enable_sharpness, dev, VIDEO_CID_ARDUCAM_EN_SHARPNESS,
			      (struct video_ctrl_range){.min = 0, .max = 65535, .step = 1, .def = 0});
	ctrls->enable_sharpness.flags |= VIDEO_CTRL_FLAG_READ_ONLY;
	if (ret < 0) {
		return ret;
	}
	ret = video_init_ctrl(&ctrls->device_address, dev, VIDEO_CID_ARDUCAM_DEV_ADDR,
			      (struct video_ctrl_range){.min = 0, .max = 65535, .step = 1, .def = 0});
	ctrls->device_address.flags |= VIDEO_CTRL_FLAG_READ_ONLY;
	if (ret < 0) {
		return ret;
	}
	ret = video_init_ctrl(&ctrls->camera_id, dev, VIDEO_CID_ARDUCAM_CAM_ID,
			      (struct video_ctrl_range){.min = 0, .max = 65535, .step = 1, .def = 0});
	ctrls->camera_id.flags |= VIDEO_CTRL_FLAG_READ_ONLY;
	if (ret < 0) {
		return ret;
	}
	ret = video_init_int_menu_ctrl(&ctrls->linkfreq, dev, VIDEO_CID_LINK_FREQ,
				       ARDUCAM_MEGA_640_480_LINK_FREQ_ID, arducam_mega_link_frequency,
				       ARRAY_SIZE(arducam_mega_link_frequency));
	if (ret < 0) {
		return ret;
	}
	ctrls->linkfreq.flags |= VIDEO_CTRL_FLAG_READ_ONLY;

	return 0;
}

static int arducam_mega_init(const struct device *dev)
{
	const struct arducam_mega_config *cfg = dev->config;
	struct arducam_mega_data *drv_data = dev->data;

	struct video_format fmt;
	int ret = 0;

	if (!spi_is_ready_dt(&cfg->bus)) {
		LOG_ERR("%s: device is not ready", cfg->bus.bus->name);
		return -ENODEV;
	}

	drv_data->dev = dev;
	k_fifo_init(&drv_data->fifo_in);
	k_fifo_init(&drv_data->fifo_out);
	k_work_queue_init(&ac_work_q);
	k_work_queue_start(&ac_work_q, ac_stack_area, K_THREAD_STACK_SIZEOF(ac_stack_area),
		AC_PRIORITY, NULL);

	k_timer_init(&drv_data->stream_schedule_timer, on_stream_schedule_timer_func, NULL);
	drv_data->stream_schedule_timer.user_data = (void *)drv_data;

	k_work_init(&drv_data->buf_work, __buffer_work);

	arducam_mega_soft_reset(dev);
	ret = arducam_mega_check_connection(dev);

	if (ret < 0) {
		LOG_ERR("arducam mega camera not connection.\n");
		return ret;
	}

	drv_data->ver.year = arducam_mega_read_reg(&cfg->bus, CAM_REG_YEAR_SDK) & 0x3F;
	drv_data->ver.month = arducam_mega_read_reg(&cfg->bus, CAM_REG_MONTH_SDK) & 0x0F;
	drv_data->ver.day = arducam_mega_read_reg(&cfg->bus, CAM_REG_DAY_SDK) & 0x1F;
	drv_data->ver.version =
		arducam_mega_read_reg(&cfg->bus, CAM_REG_FPGA_VERSION_NUMBER) & 0xfF;

	LOG_INF("arducam mega ver: %d-%d-%d \t %x", drv_data->ver.year, drv_data->ver.month,
		drv_data->ver.day, drv_data->ver.version);

	/* set default/init format */
	fmt.type = VIDEO_BUF_TYPE_OUTPUT;
	fmt.pixelformat = VIDEO_PIX_FMT_RGB565;
	fmt.width = 320;
	fmt.height = 240;
	fmt.pitch = 320 * video_bits_per_pixel(VIDEO_PIX_FMT_RGB565) / BITS_PER_BYTE;
	ret = arducam_mega_set_fmt(dev, &fmt);
	if (ret < 0) {
		LOG_ERR("Unable to configure default format");
		return -EIO;
	}
	ret = arducam_mega_controls(dev);
	if (ret < 0) {
		LOG_ERR("Unable to initialize controls");
		return -EIO;
	}
	return ret;
}

#define ARDUCAM_MEGA_INIT(inst)                                                                     \
	static const struct arducam_mega_config arducam_mega_cfg_##inst = {                        		\
		.bus = SPI_DT_SPEC_INST_GET(inst,                                                  			\
					    SPI_OP_MODE_MASTER | SPI_WORD_SET(8) |                 						\
						    SPI_CS_ACTIVE_HIGH | SPI_LINES_SINGLE |        							\
						    SPI_LOCK_ON,                                   							\
					    0),                                                    						\
	};                                                                                         		\
                                                                                                    \
	static struct arducam_mega_data arducam_mega_data_##inst = {.fmt = {VIDEO_BUF_TYPE_OUTPUT,		\
																VIDEO_PIX_FMT_RGB565,				\
																320,								\
																240,								\
																0 }}; 								\
																									\
	DEVICE_DT_INST_DEFINE(inst, &arducam_mega_init, NULL, &arducam_mega_data_##inst,           		\
			      &arducam_mega_cfg_##inst, POST_KERNEL, CONFIG_VIDEO_INIT_PRIORITY,   				\
			      &arducam_mega_driver_api);														\
				  																					\
	VIDEO_DEVICE_DEFINE(arducam_mega_##inst, DEVICE_DT_INST_GET(inst), NULL);

DT_INST_FOREACH_STATUS_OKAY(ARDUCAM_MEGA_INIT)
