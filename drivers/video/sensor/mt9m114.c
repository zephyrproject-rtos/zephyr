/*
 * Copyright (c) 2019, Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr.h>
#include <device.h>

#include <sys/byteorder.h>

#include <drivers/video.h>
#include <drivers/i2c.h>

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(mt9m114);

#define MT9M114_CHIP_ID_VAL				0x2481

/* Sysctl registers */
#define MT9M114_CHIP_ID					0x0000
#define MT9M114_COMMAND_REGISTER			0x0080
#define MT9M114_COMMAND_REGISTER_APPLY_PATCH		(1 << 0)
#define MT9M114_COMMAND_REGISTER_SET_STATE		(1 << 1)
#define MT9M114_COMMAND_REGISTER_REFRESH		(1 << 2)
#define MT9M114_COMMAND_REGISTER_WAIT_FOR_EVENT		(1 << 3)
#define MT9M114_COMMAND_REGISTER_OK			(1 << 15)
#define MT9M114_PAD_CONTROL				0x0032
#define MT9M114_RST_AND_MISC_CONTROL			0x001A

/* Camera Control registers */
#define MT9M114_CAM_OUTPUT_FORMAT			0xc86c

/* System Manager registers */
#define MT9M114_SYSMGR_NEXT_STATE			0xdc00
#define MT9M114_SYSMGR_CURRENT_STATE			0xdc01
#define MT9M114_SYSMGR_CMD_STATUS			0xdc02

/* System States */
#define MT9M114_SYS_STATE_ENTER_CONFIG_CHANGE		0x28
#define MT9M114_SYS_STATE_STREAMING			0x31
#define MT9M114_SYS_STATE_START_STREAMING		0x34
#define MT9M114_SYS_STATE_ENTER_SUSPEND			0x40
#define MT9M114_SYS_STATE_SUSPENDED			0x41
#define MT9M114_SYS_STATE_ENTER_STANDBY			0x50
#define MT9M114_SYS_STATE_STANDBY			0x52
#define MT9M114_SYS_STATE_LEAVE_STANDBY			0x54

struct mt9m114_data {
	struct device *i2c;
	struct video_format fmt;
	u8_t i2c_addr;
};

struct mt9m114_reg {
	u16_t addr;
	u16_t value_size;
	u32_t value;
};

static struct mt9m114_reg mt9m114_vga_24mhz_pll[] = {
	{ 0x98E,  2, 0x1000	},
	{ 0xC97E, 2, 0x01	}, /* cam_sysctl_pll_enable = 1 */
	{ 0xC980, 2, 0x0120	}, /* cam_sysctl_pll_divider_m_n = 288 */
	{ 0xC982, 2, 0x0700	}, /* cam_sysctl_pll_divider_p = 1792 */
	{ 0xC984, 2, 0x8000	}, /* cam_port_output_control = 32776 */
	{ 0xC800, 2, 0x0000	}, /* cam_sensor_cfg_y_addr_start = 0 */
	{ 0xC802, 2, 0x0000	}, /* cam_sensor_cfg_x_addr_start = 0 */
	{ 0xC804, 2, 0x03CD	}, /* cam_sensor_cfg_y_addr_end = 973 */
	{ 0xC806, 2, 0x050D	}, /* cam_sensor_cfg_x_addr_end = 1293 */
	{ 0xC808, 4, 0x2DC6C00	}, /* cam_sensor_cfg_pixclk = 48000000 */
	{ 0xC80C, 2, 0x0001	}, /* cam_sensor_cfg_row_speed = 1 */
	{ 0xC80E, 2, 0x00DB	}, /* cam_sensor_cfg_fine_integ_min = 219 */
	{ 0xC810, 2, 0x07C2	}, /* cam_sensor_cfg_fine_integ_max = 1986 */
	{ 0xC812, 2, 0x02FE	}, /* cam_sensor_cfg_frame_length_lines = 766 */
	{ 0xC814, 2, 0x0845	}, /* cam_sensor_cfg_line_length_pck = 2117 */
	{ 0xC816, 2, 0x0060	}, /* cam_sensor_cfg_fine_correction = 96 */
	{ 0xC818, 2, 0x01E3	}, /* cam_sensor_cfg_cpipe_last_row = 483 */
	{ 0xC826, 2, 0x0020	}, /* cam_sensor_cfg_reg_0_data = 32 */
	{ 0xC834, 2, 0x0110	}, /* cam_sensor_control_read_mode = 272 */
	{ 0xC854, 2, 0x0000	}, /* cam_crop_window_xoffset = 0 */
	{ 0xC856, 2, 0x0000	}, /* cam_crop_window_yoffset = 0 */
	{ 0xC858, 2, 0x0280	}, /* cam_crop_window_width = 640 */
	{ 0xC85A, 2, 0x01E0	}, /* cam_crop_window_height = 480 */
	{ 0xC85C, 1, 0x03	}, /* cam_crop_cropmode = 3 */
	{ 0xC868, 2, 0x0280	}, /* cam_output_width = 640 */
	{ 0xC86A, 2, 0x01E0	}, /* cam_output_height = 480 */
	{ 0xC878, 1, 0x00	}, /* cam_aet_aemode = 0 */
	{ 0xC88C, 2, 0x1D9A	}, /* cam_aet_max_frame_rate = 7578 */
	{ 0xC914, 2, 0x0000	}, /* cam_stat_awb_clip_window_xstart = 0 */
	{ 0xC88E, 2, 0x1D9A	}, /* cam_aet_min_frame_rate = 7578 */
	{ 0xC916, 2, 0x0000	}, /* cam_stat_awb_clip_window_ystart = 0 */
	{ 0xC918, 2, 0x027F	}, /* cam_stat_awb_clip_window_xend = 639 */
	{ 0xC91A, 2, 0x01DF	}, /* cam_stat_awb_clip_window_yend = 479 */
	{ 0xC91C, 2, 0x0000	}, /* cam_stat_ae_initial_window_xstart = 0 */
	{ 0xC91E, 2, 0x0000	}, /* cam_stat_ae_initial_window_ystart = 0 */
	{ 0xC920, 2, 0x007F	}, /* cam_stat_ae_initial_window_xend = 127 */
	{ 0xC922, 2, 0x005F	}, /* cam_stat_ae_initial_window_yend = 95 */
	{ /* NULL terminated */ }
};

static inline int i2c_burst_read16(struct device *dev, u16_t dev_addr,
				   u16_t start_addr, u8_t *buf, u32_t num_bytes)
{
	u8_t addr_buffer[2];

	addr_buffer[1] = start_addr & 0xFF;
	addr_buffer[0] = start_addr >> 8;
	return i2c_write_read(dev, dev_addr, addr_buffer, sizeof(addr_buffer),
			      buf, num_bytes);
}

static inline int i2c_burst_write16(struct device *dev, u16_t dev_addr,
				    u16_t start_addr, const u8_t *buf,
				    u32_t num_bytes)
{
	u8_t addr_buffer[2];
	struct i2c_msg msg[2];

	addr_buffer[1] = start_addr & 0xFF;
	addr_buffer[0] = start_addr >> 8;
	msg[0].buf = addr_buffer;
	msg[0].len = 2U;
	msg[0].flags = I2C_MSG_WRITE;

	msg[1].buf = (u8_t *)buf;
	msg[1].len = num_bytes;
	msg[1].flags = I2C_MSG_WRITE | I2C_MSG_STOP;

	return i2c_transfer(dev, msg, 2, dev_addr);
}

static int mt9m114_write_reg(struct device *dev, u16_t reg_addr, u8_t reg_size,
			     void *value)
{
	struct mt9m114_data *drv_data = dev->driver_data;

	switch (reg_size) {
	case 2:
		*(u16_t *)value = sys_cpu_to_be16(*(u16_t *)value);
		break;
	case 4:
		*(u16_t *)value = sys_cpu_to_be32(*(u16_t *)value);
		break;
	case 1:
		break;
	default:
		return -ENOTSUP;
	}

	return i2c_burst_write16(drv_data->i2c, drv_data->i2c_addr, reg_addr,
				 value, reg_size);
}

static int mt9m114_read_reg(struct device *dev, u16_t reg_addr, u8_t reg_size,
			    void *value)
{
	struct mt9m114_data *drv_data = dev->driver_data;
	int err;

	if (reg_size > 4) {
		return -ENOTSUP;
	}

	err = i2c_burst_read16(drv_data->i2c, drv_data->i2c_addr, reg_addr,
			       value, reg_size);
	if (err) {
		return err;
	}

	switch (reg_size) {
	case 2:
		*(u16_t *)value = sys_be16_to_cpu(*(u16_t *)value);
		break;
	case 4:
		*(u16_t *)value = sys_be32_to_cpu(*(u16_t *)value);
		break;
	case 1:
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int mt9m114_write_all(struct device *dev, struct mt9m114_reg *reg)
{
	int i = 0;

	while (reg[i].value_size) {
		int err;

		err = mt9m114_write_reg(dev, reg[i].addr, reg[i].value_size,
					&reg[i].value);
		if (err) {
			return err;
		}

		i++;
	}

	return 0;
}

static int mt9m114_set_state(struct device *dev, u8_t state)
{
	u16_t val;
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

		k_sleep(1);
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

		k_sleep(1);
	}

	/* Check the 'OK' bit to see if the command was successful. */
	err = mt9m114_read_reg(dev, MT9M114_COMMAND_REGISTER, 2, &val);
	if (err || !(val & MT9M114_COMMAND_REGISTER_OK)) {
		return -EIO;
	}

	return 0;
}

static int mt9m114_set_fmt(struct device *dev, enum video_endpoint_id ep,
			   struct video_format *fmt)
{
	struct mt9m114_data *drv_data = dev->driver_data;
	u16_t output_format;
	int ret;

	/* we only support one format for now (VGA RGB565) */
	if (fmt->pixelformat != VIDEO_PIX_FMT_RGB565 || fmt->height != 480 ||
	    fmt->width != 640) {
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
	output_format = ((1U << 8U) | (1U << 1U)); /* RGB565 */
	ret = mt9m114_write_reg(dev, MT9M114_CAM_OUTPUT_FORMAT,
				sizeof(output_format), &output_format);
	if (ret) {
		LOG_ERR("Unable to set output format");
		return ret;
	}

	/* Apply Config */
	mt9m114_set_state(dev, MT9M114_SYS_STATE_ENTER_CONFIG_CHANGE);

	return 0;
}

static int mt9m114_get_fmt(struct device *dev, enum video_endpoint_id ep,
			   struct video_format *fmt)
{
	struct mt9m114_data *drv_data = dev->driver_data;

	*fmt = drv_data->fmt;

	return 0;
}

static int mt9m114_stream_start(struct device *dev)
{
	return mt9m114_set_state(dev, MT9M114_SYS_STATE_START_STREAMING);
}

static int mt9m114_stream_stop(struct device *dev)
{
	return mt9m114_set_state(dev, MT9M114_SYS_STATE_ENTER_SUSPEND);
}

static const struct video_format_cap fmts[] = {
	{
		.pixelformat = VIDEO_PIX_FMT_RGB565,
		.width_min = 640,
		.width_max = 640,
		.height_min = 480,
		.height_max = 480,
		.width_step = 0,
		.height_step = 0,
	},
	{ 0 }
};

static int mt9m114_get_caps(struct device *dev, enum video_endpoint_id ep,
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

static int mt9m114_init(struct device *dev)
{
	struct video_format fmt;
	u16_t val;
	int ret;

	/* no power control, wait for camera ready */
	k_sleep(100);

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

static struct mt9m114_data mt9m114_data_0;

static int mt9m114_init_0(struct device *dev)
{
	struct mt9m114_data *drv_data = dev->driver_data;

	drv_data->i2c = device_get_binding(DT_INST_0_APTINA_MT9M114_BUS_NAME);
	if (drv_data->i2c == NULL) {
		LOG_ERR("Failed to get pointer to %s device!",
			DT_INST_0_APTINA_MT9M114_LABEL);
			return -EINVAL;
	}

	drv_data->i2c_addr = DT_INST_0_APTINA_MT9M114_BASE_ADDRESS;


	return mt9m114_init(dev);
}

DEVICE_AND_API_INIT(mt9m114, DT_INST_0_APTINA_MT9M114_LABEL, &mt9m114_init_0,
		    &mt9m114_data_0, NULL,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &mt9m114_driver_api);
#endif
