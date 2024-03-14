/*
 * Copyright (c) 2019, Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT aptina_mt9m114
#include <zephyr/kernel.h>
#include <zephyr/device.h>

#include <zephyr/sys/byteorder.h>

#include <zephyr/drivers/video.h>
#include <zephyr/drivers/i2c.h>

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mt9m114);

#define MT9M114_CHIP_ID_VAL 0x2481

/* Sysctl registers */
#define MT9M114_CHIP_ID                         0x0000
#define MT9M114_COMMAND_REGISTER                0x0080
#define MT9M114_COMMAND_REGISTER_SET_STATE      (1 << 1)
#define MT9M114_COMMAND_REGISTER_OK             (1 << 15)

/* Camera Control registers */
#define MT9M114_CAM_OUTPUT_FORMAT 0xC86C

/* System Manager registers */
#define MT9M114_SYSMGR_NEXT_STATE    0xDC00

/* System States */
#define MT9M114_SYS_STATE_ENTER_CONFIG_CHANGE 0x28
#define MT9M114_SYS_STATE_START_STREAMING     0x34
#define MT9M114_SYS_STATE_ENTER_SUSPEND       0x40

/* Camera output format */
#define MT9M114_CAM_OUTPUT_FORMAT_FORMAT_YUV (0 << 8)
#define MT9M114_CAM_OUTPUT_FORMAT_FORMAT_RGB (1 << 8)

struct mt9m114_config {
	struct i2c_dt_spec i2c;
};

struct mt9m114_data {
	struct video_format fmt;
};

struct mt9m114_reg {
	uint16_t addr;
	uint16_t value_size;
	uint32_t value;
};

static struct mt9m114_reg mt9m114_vga_24mhz_pll[] = {
	{0x98E, 2, 0x1000},     /* logical_address_access = 0x1000 */
	{0xC97E, 2, 0x01},      /* cam_sysctl_pll_enable = 1 */
	{0xC980, 2, 0x0120},    /* cam_sysctl_pll_divider_m_n = 288 */
	{0xC982, 2, 0x0700},    /* cam_sysctl_pll_divider_p = 1792 */
	{0xC984, 2, 0x8000},    /* cam_port_output_control = 32776 */
	{0xC800, 2, 0x0000},    /* cam_sensor_cfg_y_addr_start = 0 */
	{0xC802, 2, 0x0000},    /* cam_sensor_cfg_x_addr_start = 0 */
	{0xC804, 2, 0x03CD},    /* cam_sensor_cfg_y_addr_end = 973 */
	{0xC806, 2, 0x050D},    /* cam_sensor_cfg_x_addr_end = 1293 */
	{0xC808, 4, 0x2DC6C00}, /* cam_sensor_cfg_pixclk = 48000000 */
	{0xC80C, 2, 0x0001},    /* cam_sensor_cfg_row_speed = 1 */
	{0xC80E, 2, 0x00DB},    /* cam_sensor_cfg_fine_integ_min = 219 */
	{0xC810, 2, 0x07C2},    /* cam_sensor_cfg_fine_integ_max = 1986 */
	{0xC812, 2, 0x02FE},    /* cam_sensor_cfg_frame_length_lines = 766 */
	{0xC814, 2, 0x0845},    /* cam_sensor_cfg_line_length_pck = 2117 */
	{0xC816, 2, 0x0060},    /* cam_sensor_cfg_fine_correction = 96 */
	{0xC818, 2, 0x01E3},    /* cam_sensor_cfg_cpipe_last_row = 483 */
	{0xC826, 2, 0x0020},    /* cam_sensor_cfg_reg_0_data = 32 */
	{0xC834, 2, 0x0110},    /* cam_sensor_control_read_mode = 272 */
	{0xC854, 2, 0x0000},    /* cam_crop_window_xoffset = 0 */
	{0xC856, 2, 0x0000},    /* cam_crop_window_yoffset = 0 */
	{0xC858, 2, 0x0280},    /* cam_crop_window_width = 640 */
	{0xC85A, 2, 0x01E0},    /* cam_crop_window_height = 480 */
	{0xC85C, 1, 0x03},      /* cam_crop_cropmode = 3 */
	{0xC868, 2, 0x0280},    /* cam_output_width = 640 */
	{0xC86A, 2, 0x01E0},    /* cam_output_height = 480 */
	{0xC878, 1, 0x00},      /* cam_aet_aemode = 0 */
	{0xC88C, 2, 0x1D9A},    /* cam_aet_max_frame_rate = 7578 */
	{0xC914, 2, 0x0000},    /* cam_stat_awb_clip_window_xstart = 0 */
	{0xC88E, 2, 0x1D9A},    /* cam_aet_min_frame_rate = 7578 */
	{0xC916, 2, 0x0000},    /* cam_stat_awb_clip_window_ystart = 0 */
	{0xC918, 2, 0x027F},    /* cam_stat_awb_clip_window_xend = 639 */
	{0xC91A, 2, 0x01DF},    /* cam_stat_awb_clip_window_yend = 479 */
	{0xC91C, 2, 0x0000},    /* cam_stat_ae_initial_window_xstart = 0 */
	{0xC91E, 2, 0x0000},    /* cam_stat_ae_initial_window_ystart = 0 */
	{0xC920, 2, 0x007F},    /* cam_stat_ae_initial_window_xend = 127 */
	{0xC922, 2, 0x005F},    /* cam_stat_ae_initial_window_yend = 95 */
	{/* NULL terminated */}};

static inline int i2c_burst_read16_dt(const struct i2c_dt_spec *spec, uint16_t start_addr,
				      uint8_t *buf, uint32_t num_bytes)
{
	uint8_t addr_buffer[2];

	addr_buffer[1] = start_addr & 0xFF;
	addr_buffer[0] = start_addr >> 8;
	return i2c_write_read_dt(spec, addr_buffer, sizeof(addr_buffer), buf, num_bytes);
}

static inline int i2c_burst_write16_dt(const struct i2c_dt_spec *spec, uint16_t start_addr,
				       const uint8_t *buf, uint32_t num_bytes)
{
	uint8_t addr_buffer[2];
	struct i2c_msg msg[2];

	addr_buffer[1] = start_addr & 0xFF;
	addr_buffer[0] = start_addr >> 8;
	msg[0].buf = addr_buffer;
	msg[0].len = 2U;
	msg[0].flags = I2C_MSG_WRITE;

	msg[1].buf = (uint8_t *)buf;
	msg[1].len = num_bytes;
	msg[1].flags = I2C_MSG_WRITE | I2C_MSG_STOP;

	return i2c_transfer_dt(spec, msg, 2);
}

static int mt9m114_write_reg(const struct device *dev, uint16_t reg_addr, uint8_t reg_size,
			     void *value)
{
	const struct mt9m114_config *cfg = dev->config;

	switch (reg_size) {
	case 2:
		*(uint16_t *)value = sys_cpu_to_be16(*(uint16_t *)value);
		break;
	case 4:
		*(uint32_t *)value = sys_cpu_to_be32(*(uint32_t *)value);
		break;
	case 1:
		break;
	default:
		return -ENOTSUP;
	}

	return i2c_burst_write16_dt(&cfg->i2c, reg_addr, value, reg_size);
}

static int mt9m114_read_reg(const struct device *dev, uint16_t reg_addr, uint8_t reg_size,
			    void *value)
{
	const struct mt9m114_config *cfg = dev->config;
	int err;

	if (reg_size > 4) {
		return -ENOTSUP;
	}

	err = i2c_burst_read16_dt(&cfg->i2c, reg_addr, value, reg_size);
	if (err) {
		return err;
	}

	switch (reg_size) {
	case 2:
		*(uint16_t *)value = sys_be16_to_cpu(*(uint16_t *)value);
		break;
	case 4:
		*(uint32_t *)value = sys_be32_to_cpu(*(uint32_t *)value);
		break;
	case 1:
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int mt9m114_write_all(const struct device *dev, struct mt9m114_reg *reg)
{
	int i = 0;

	while (reg[i].value_size) {
		int err;

		err = mt9m114_write_reg(dev, reg[i].addr, reg[i].value_size, &reg[i].value);
		if (err) {
			return err;
		}

		i++;
	}

	return 0;
}

static int mt9m114_set_state(const struct device *dev, uint8_t state)
{
	uint16_t val;
	int err;

	/* Set next state. */
	mt9m114_write_reg(dev, MT9M114_SYSMGR_NEXT_STATE, 1, &state);

	/* Check that the FW is ready to accept a new command. */
	while (1) {
		err = mt9m114_read_reg(dev, MT9M114_COMMAND_REGISTER, 2, &val);
		if (err) {
			return err;
		}

		if (!(val & MT9M114_COMMAND_REGISTER_SET_STATE)) {
			break;
		}

		k_sleep(K_MSEC(1));
	}

	/* Issue the Set State command. */
	val = MT9M114_COMMAND_REGISTER_SET_STATE | MT9M114_COMMAND_REGISTER_OK;
	mt9m114_write_reg(dev, MT9M114_COMMAND_REGISTER, 2, &val);

	/* Wait for the FW to complete the command. */
	while (1) {
		err = mt9m114_read_reg(dev, MT9M114_COMMAND_REGISTER, 2, &val);
		if (err) {
			return err;
		}

		if (!(val & MT9M114_COMMAND_REGISTER_SET_STATE)) {
			break;
		}

		k_sleep(K_MSEC(1));
	}

	/* Check the 'OK' bit to see if the command was successful. */
	err = mt9m114_read_reg(dev, MT9M114_COMMAND_REGISTER, 2, &val);
	if (err || !(val & MT9M114_COMMAND_REGISTER_OK)) {
		return -EIO;
	}

	return 0;
}

static int mt9m114_set_output_format(const struct device *dev, int pixel_format)
{
	int ret = 0;
	uint16_t output_format;

	if (pixel_format == VIDEO_PIX_FMT_YUYV) {
		output_format = (MT9M114_CAM_OUTPUT_FORMAT_FORMAT_YUV | (1U << 1U));
	} else if (pixel_format == VIDEO_PIX_FMT_RGB565) {
		output_format = (MT9M114_CAM_OUTPUT_FORMAT_FORMAT_RGB | (1U << 1U));
	} else {
		LOG_ERR("Image format not supported");
		return -ENOTSUP;
	}

	ret = mt9m114_write_reg(dev, MT9M114_CAM_OUTPUT_FORMAT, sizeof(output_format),
				&output_format);

	return ret;
}

static int mt9m114_set_fmt(const struct device *dev, enum video_endpoint_id ep,
			   struct video_format *fmt)
{
	struct mt9m114_data *drv_data = dev->data;
	int ret;

	/* we support RGB565 and YUV output pixel formats for now */
	if (fmt->pixelformat != VIDEO_PIX_FMT_RGB565 && fmt->pixelformat != VIDEO_PIX_FMT_YUYV) {
		LOG_ERR("Unsupported output pixel format");
		return -ENOTSUP;
	}

	/* we only support one format size for now (VGA) */
	if (fmt->height != 480 || fmt->width != 640) {
		LOG_ERR("Unsupported output size format");
		return -ENOTSUP;
	}

	if (!memcmp(&drv_data->fmt, fmt, sizeof(drv_data->fmt))) {
		/* nothing to do */
		return 0;
	}

	drv_data->fmt = *fmt;

	/* Configure Sensor */
	ret = mt9m114_write_all(dev, mt9m114_vga_24mhz_pll);
	if (ret) {
		LOG_ERR("Unable to write mt9m114 config");
		return ret;
	}

	/* Set output format */
	ret = mt9m114_set_output_format(dev, fmt->pixelformat);
	if (ret) {
		LOG_ERR("Unable to set output format");
		return ret;
	}

	/* Apply Config */
	mt9m114_set_state(dev, MT9M114_SYS_STATE_ENTER_CONFIG_CHANGE);

	return 0;
}

static int mt9m114_get_fmt(const struct device *dev, enum video_endpoint_id ep,
			   struct video_format *fmt)
{
	struct mt9m114_data *drv_data = dev->data;

	*fmt = drv_data->fmt;

	return 0;
}

static int mt9m114_stream_start(const struct device *dev)
{
	return mt9m114_set_state(dev, MT9M114_SYS_STATE_START_STREAMING);
}

static int mt9m114_stream_stop(const struct device *dev)
{
	return mt9m114_set_state(dev, MT9M114_SYS_STATE_ENTER_SUSPEND);
}

#define MT9M114_VIDEO_FORMAT_CAP(width, height, format)                                            \
	{                                                                                          \
		.pixelformat = (format), .width_min = (width), .width_max = (width),               \
		.height_min = (height), .height_max = (height), .width_step = 0, .height_step = 0  \
	}

static const struct video_format_cap fmts[] = {
	MT9M114_VIDEO_FORMAT_CAP(640, 480, VIDEO_PIX_FMT_RGB565), /* VGA RGB565 */
	MT9M114_VIDEO_FORMAT_CAP(640, 480, VIDEO_PIX_FMT_YUYV),   /* VGA YUYV */
	{0}};

static int mt9m114_get_caps(const struct device *dev, enum video_endpoint_id ep,
			    struct video_caps *caps)
{
	caps->format_caps = fmts;
	return 0;
}

static const struct video_driver_api mt9m114_driver_api = {
	.set_format = mt9m114_set_fmt,
	.get_format = mt9m114_get_fmt,
	.get_caps = mt9m114_get_caps,
	.stream_start = mt9m114_stream_start,
	.stream_stop = mt9m114_stream_stop,
};

static int mt9m114_init(const struct device *dev)
{
	struct video_format fmt;
	uint16_t val;
	int ret;

	/* no power control, wait for camera ready */
	k_sleep(K_MSEC(100));

	ret = mt9m114_read_reg(dev, MT9M114_CHIP_ID, sizeof(val), &val);
	if (ret) {
		LOG_ERR("Unable to read chip ID");
		return -ENODEV;
	}

	if (val != MT9M114_CHIP_ID_VAL) {
		LOG_ERR("Wrong ID: %04x (exp %04x)", val, MT9M114_CHIP_ID_VAL);
		return -ENODEV;
	}

	/* set default/init format VGA RGB565 */
	fmt.pixelformat = VIDEO_PIX_FMT_RGB565;
	fmt.width = 640;
	fmt.height = 480;
	fmt.pitch = 640 * 2;

	ret = mt9m114_set_fmt(dev, VIDEO_EP_OUT, &fmt);
	if (ret) {
		LOG_ERR("Unable to configure default format");
		return -EIO;
	}

	/* Suspend any stream */
	mt9m114_set_state(dev, MT9M114_SYS_STATE_ENTER_SUSPEND);

	return 0;
}

#if 1 /* Unique Instance */

static const struct mt9m114_config mt9m114_cfg_0 = {
	.i2c = I2C_DT_SPEC_INST_GET(0),
};

static struct mt9m114_data mt9m114_data_0;

static int mt9m114_init_0(const struct device *dev)
{
	const struct mt9m114_config *cfg = dev->config;

	if (!device_is_ready(cfg->i2c.bus)) {
		LOG_ERR("Bus device is not ready");
		return -ENODEV;
	}

	return mt9m114_init(dev);
}

DEVICE_DT_INST_DEFINE(0, &mt9m114_init_0, NULL, &mt9m114_data_0, &mt9m114_cfg_0, POST_KERNEL,
		      CONFIG_VIDEO_INIT_PRIORITY, &mt9m114_driver_api);
#endif
