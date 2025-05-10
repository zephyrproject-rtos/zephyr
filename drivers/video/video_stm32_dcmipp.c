/*
 * Copyright (c) 2025 STMicroelectronics.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_stm32_dcmipp

#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/drivers/video.h>
#include <zephyr/drivers/video-controls.h>
#include <zephyr/logging/log.h>

#include "video_ctrls.h"
#include "video_device.h"
#include "video_stm32_dcmipp.h"

LOG_MODULE_REGISTER(stm32_dcmipp, CONFIG_VIDEO_LOG_LEVEL);

typedef void (*irq_config_func_t)(const struct device *dev);

enum stm32_dcmipp_state {
	STM32_DCMIPP_STOPPED = 0,
	STM32_DCMIPP_WAIT_FOR_BUFFER,
	STM32_DCMIPP_RUNNING,
};

struct stm32_dcmipp_ctrls {
	struct video_ctrl isp_hdec;
	struct video_ctrl isp_vdec;
};

#define STM32_DCMIPP_MAX_PIPE_NB	3

#define STM32_DCMIPP_MAX_ISP_DEC_FACTOR	8
#define STM32_DCMIPP_MAX_PIPE_DEC_FACTOR	8
#define STM32_DCMIPP_MAX_PIPE_DSIZE_FACTOR	8
#define STM32_DCMIPP_MAX_PIPE_SCALE_FACTOR (STM32_DCMIPP_MAX_PIPE_DEC_FACTOR * \
					    STM32_DCMIPP_MAX_PIPE_DSIZE_FACTOR)
struct stm32_dcmipp_pipe_data {
	const struct device *dev;
	uint32_t id;
	struct stm32_dcmipp_data *dcmipp;
	struct k_mutex lock;
	struct video_format fmt;
	struct k_fifo fifo_in;
	struct k_fifo fifo_out;
	struct video_buffer *next;
	struct video_buffer *active;
	enum stm32_dcmipp_state state;
	bool is_streaming;
};

struct stm32_dcmipp_data {
	const struct device *dev;
	DCMIPP_HandleTypeDef hdcmipp;
	struct stm32_dcmipp_ctrls ctrls;

	/* FIXME - there should be a mutex to protect enabled_pipe */
	unsigned int enabled_pipe;
	struct video_format sensor_fmt;
	const struct video_format_cap *sensor_format_caps;

	int32_t isp_dec_hratio;
	int32_t isp_dec_vratio;

	struct stm32_dcmipp_pipe_data *pipe[STM32_DCMIPP_MAX_PIPE_NB];
};

struct stm32_dcmipp_config {
	const struct stm32_pclken *pclken;
	size_t pclk_len;
	irq_config_func_t irq_config;
	const struct device *sensor_dev;
	const struct reset_dt_spec reset_dcmipp;
	const struct reset_dt_spec reset_csi;
	struct {
		uint32_t nb_lanes;
		uint8_t lanes[2];
	} csi;
};

#define STM32_DCMIPP_WIDTH_MIN	16
#define STM32_DCMIPP_HEIGHT_MIN	2
#define STM32_DCMIPP_WIDTH_MAX	4094
#define STM32_DCMIPP_HEIGHT_MAX	4094

/* Callback getting called for each frame written into memory */
void HAL_DCMIPP_PIPE_FrameEventCallback(DCMIPP_HandleTypeDef *hdcmipp, uint32_t Pipe)
{
	struct stm32_dcmipp_data *dcmipp =
			CONTAINER_OF(hdcmipp, struct stm32_dcmipp_data, hdcmipp);
	struct stm32_dcmipp_pipe_data *pipe = dcmipp->pipe[Pipe];
	uint32_t bytesused;
	int ret;

	if (!pipe->active) {
		LOG_ERR("SHOULD NOT HAPPEN, active buf = NULL");
	}

	/* Counter is only available on Pipe0 */
	if (Pipe == DCMIPP_PIPE0) {
		ret = HAL_DCMIPP_PIPE_GetDataCounter(hdcmipp, Pipe, &bytesused);
		if (ret) {
			LOG_ERR("Failed to read counter");
		}
		pipe->active->bytesused = bytesused;
	} else {
		pipe->active->bytesused = pipe->fmt.height * pipe->fmt.pitch;
	}

	pipe->active->timestamp = k_uptime_get_32();
	pipe->active->line_offset = 0;

	k_fifo_put(&pipe->fifo_out, pipe->active);
	pipe->active = NULL;
}

/* Callback getting called for each vsync */
void HAL_DCMIPP_PIPE_VsyncEventCallback(DCMIPP_HandleTypeDef *hdcmipp, uint32_t Pipe)
{
	struct stm32_dcmipp_data *dcmipp =
			CONTAINER_OF(hdcmipp, struct stm32_dcmipp_data, hdcmipp);
	struct stm32_dcmipp_pipe_data *pipe = dcmipp->pipe[Pipe];
	int ret;

	if (pipe->state != STM32_DCMIPP_RUNNING) {
		return;
	}

	pipe->active = pipe->next;

	pipe->next = k_fifo_get(&pipe->fifo_in, K_NO_WAIT);
	if (!pipe->next) {
		LOG_DBG("No buffer available, pause streaming");
		/*
		 * Disable Capture Request
		 * Use direct register access here since we don't want to wait until
		 * getting CPTACT down here since we still need to handle other stuff
		 */
		pipe->state = STM32_DCMIPP_WAIT_FOR_BUFFER;
		if (Pipe == DCMIPP_PIPE0) {
			CLEAR_BIT(hdcmipp->Instance->P0FCTCR, DCMIPP_P0FCTCR_CPTREQ);
		} else if (Pipe == DCMIPP_PIPE1) {
			CLEAR_BIT(hdcmipp->Instance->P1FCTCR, DCMIPP_P1FCTCR_CPTREQ);
		} else if (Pipe == DCMIPP_PIPE2) {
			CLEAR_BIT(hdcmipp->Instance->P2FCTCR, DCMIPP_P2FCTCR_CPTREQ);
		}
		return;
	}

	/*
	 * TODO - we only support 1 buffer formats for the time being, setting of
	 * MEMORY_ADDRESS_1 and MEMORY_ADDRESS_2 required depending on the pixelformat
	 * for Pipe1
	 */
	ret = HAL_DCMIPP_PIPE_SetMemoryAddress(&dcmipp->hdcmipp, Pipe, DCMIPP_MEMORY_ADDRESS_0,
					       (uint32_t)pipe->next->buffer);
	if (ret) {
		LOG_ERR("Failed to update memory address");
		return;
	}
}

#define	DCMIPP_PHY_BITRATE(val)				\
	{						\
		.rate = val,				\
		.PHYBitrate = DCMIPP_CSI_PHY_BT_##val,	\
	}
static const struct {
	uint16_t rate;
	uint16_t PHYBitrate;
} stm32_dcmipp_bitrate[] = {
	DCMIPP_PHY_BITRATE(80), DCMIPP_PHY_BITRATE(80), DCMIPP_PHY_BITRATE(90),
	DCMIPP_PHY_BITRATE(100), DCMIPP_PHY_BITRATE(110), DCMIPP_PHY_BITRATE(120),
	DCMIPP_PHY_BITRATE(130), DCMIPP_PHY_BITRATE(140), DCMIPP_PHY_BITRATE(150),
	DCMIPP_PHY_BITRATE(160), DCMIPP_PHY_BITRATE(170), DCMIPP_PHY_BITRATE(180),
	DCMIPP_PHY_BITRATE(190), DCMIPP_PHY_BITRATE(205), DCMIPP_PHY_BITRATE(220),
	DCMIPP_PHY_BITRATE(235), DCMIPP_PHY_BITRATE(250), DCMIPP_PHY_BITRATE(275),
	DCMIPP_PHY_BITRATE(300), DCMIPP_PHY_BITRATE(325), DCMIPP_PHY_BITRATE(350),
	DCMIPP_PHY_BITRATE(400), DCMIPP_PHY_BITRATE(450), DCMIPP_PHY_BITRATE(500),
	DCMIPP_PHY_BITRATE(550), DCMIPP_PHY_BITRATE(600), DCMIPP_PHY_BITRATE(650),
	DCMIPP_PHY_BITRATE(700), DCMIPP_PHY_BITRATE(750), DCMIPP_PHY_BITRATE(800),
	DCMIPP_PHY_BITRATE(850), DCMIPP_PHY_BITRATE(900), DCMIPP_PHY_BITRATE(950),
	DCMIPP_PHY_BITRATE(1000), DCMIPP_PHY_BITRATE(1050), DCMIPP_PHY_BITRATE(1100),
	DCMIPP_PHY_BITRATE(1150), DCMIPP_PHY_BITRATE(1200), DCMIPP_PHY_BITRATE(1250),
	DCMIPP_PHY_BITRATE(1300), DCMIPP_PHY_BITRATE(1350), DCMIPP_PHY_BITRATE(1400),
	DCMIPP_PHY_BITRATE(1450), DCMIPP_PHY_BITRATE(1500), DCMIPP_PHY_BITRATE(1550),
	DCMIPP_PHY_BITRATE(1600), DCMIPP_PHY_BITRATE(1650), DCMIPP_PHY_BITRATE(1700),
	DCMIPP_PHY_BITRATE(1750), DCMIPP_PHY_BITRATE(1800), DCMIPP_PHY_BITRATE(1850),
	DCMIPP_PHY_BITRATE(1900), DCMIPP_PHY_BITRATE(1950), DCMIPP_PHY_BITRATE(2000),
	DCMIPP_PHY_BITRATE(2050), DCMIPP_PHY_BITRATE(2100), DCMIPP_PHY_BITRATE(2150),
	DCMIPP_PHY_BITRATE(2200), DCMIPP_PHY_BITRATE(2250), DCMIPP_PHY_BITRATE(2300),
	DCMIPP_PHY_BITRATE(2350), DCMIPP_PHY_BITRATE(2400), DCMIPP_PHY_BITRATE(2450),
	DCMIPP_PHY_BITRATE(2500),
};

static int stm32_dcmipp_conf_csi(const struct device *dev)
{
	const struct stm32_dcmipp_config *config = dev->config;
	struct stm32_dcmipp_data *dcmipp = dev->data;
	DCMIPP_CSI_ConfTypeDef csiconf = { 0 };
	uint64_t phy_bitrate;
	struct video_control ctrl = {
		.id = VIDEO_CID_PIXEL_RATE,
	};
	int err, i;

	csiconf.NumberOfLanes = config->csi.nb_lanes == 2 ? DCMIPP_CSI_TWO_DATA_LANES :
							    DCMIPP_CSI_ONE_DATA_LANE;
	csiconf.DataLaneMapping = config->csi.lanes[0] == 1 ? DCMIPP_CSI_PHYSICAL_DATA_LANES :
							      DCMIPP_CSI_INVERTED_DATA_LANES;

	/* Get the pixel_rate from the sensor to guess the bitrate */
	err = video_get_ctrl(config->sensor_dev, &ctrl);
	if (err) {
		LOG_ERR("Can not get sensor_dev pixel rate");
		return err;
	}
	phy_bitrate = ctrl.val64 * video_bits_per_pixel(dcmipp->sensor_fmt.pixelformat) /
		      (2 * config->csi.nb_lanes);
	phy_bitrate /= MHZ(1);
	LOG_DBG("Sensor PixelRate = %lld, PHY Bitrate computed = %lld MHz",
		ctrl.val64, phy_bitrate);

	for (i = 0; i < ARRAY_SIZE(stm32_dcmipp_bitrate); i++) {
		if (stm32_dcmipp_bitrate[i].rate >= phy_bitrate) {
			break;
		}
	}
	if (i == ARRAY_SIZE(stm32_dcmipp_bitrate)) {
		LOG_ERR("Couldn't find proper CSI PHY settings for bitrate %lld", phy_bitrate);
		return -EINVAL;
	}
	csiconf.PHYBitrate = stm32_dcmipp_bitrate[i].PHYBitrate;

	err = HAL_DCMIPP_CSI_SetConfig(&dcmipp->hdcmipp, &csiconf);
	if (err != HAL_OK) {
		LOG_ERR("Failed to configure DCMIPP CSI");
		return -EIO;
	}

	return 0;
}

/* Description of DCMIPP inputs */
#define DUMP_PIPE_FMT(fmt, input_fmt)					\
	{								\
		.pixelformat = VIDEO_PIX_FMT_##fmt,			\
		.pipes = BIT(0),					\
		.dump.input_pixelformat = VIDEO_PIX_FMT_##input_fmt,	\
	}
#define RAW_BAYER_UNPACKED(size)				\
	DUMP_PIPE_FMT(SBGGR##size, SBGGR##size),		\
	DUMP_PIPE_FMT(SGBRG##size, SGBRG##size),		\
	DUMP_PIPE_FMT(SGRBG##size, SGRBG##size),		\
	DUMP_PIPE_FMT(SRGGB##size, SRGGB##size)

#define RAW_BAYER_PACKED(size)					\
	DUMP_PIPE_FMT(SBGGR##size##P, SBGGR##size),		\
	DUMP_PIPE_FMT(SGBRG##size##P, SGBRG##size),		\
	DUMP_PIPE_FMT(SGRBG##size##P, SGRBG##size),		\
	DUMP_PIPE_FMT(SRGGB##size##P, SRGGB##size)

/* Description of Pixel Pipes formats */
#define PIXEL_PIPE_FMT(fmt, dcmipp, swap, valid_pipes)				\
	{									\
		.pixelformat = VIDEO_PIX_FMT_##fmt,				\
		.pipes = valid_pipes,						\
		.pixels.dcmipp_format = DCMIPP_PIXEL_PACKER_FORMAT_##dcmipp,	\
		.pixels.swap_uv = swap,						\
	}
static const struct stm32_dcmipp_mapping {
	uint32_t pixelformat;
	uint32_t pipes;
	union {
		struct {
			uint32_t input_pixelformat;
		} dump;
		struct {
			uint32_t dcmipp_format;
			uint32_t swap_uv;
		} pixels;
	};
} stm32_dcmipp_format_support[] = {
	/* Dump pipe format descriptions */
	RAW_BAYER_UNPACKED(8),
	RAW_BAYER_PACKED(10), RAW_BAYER_PACKED(12), RAW_BAYER_PACKED(14),
	DUMP_PIPE_FMT(RGB565, RGB565),
	DUMP_PIPE_FMT(YUYV, YUYV),
	/* Pixel pipes format descriptions */
	PIXEL_PIPE_FMT(RGB565, RGB565_1, 0, (BIT(1) | BIT(2))),
	PIXEL_PIPE_FMT(YUYV, YUV422_1, 0, (BIT(1) | BIT(2))),
	PIXEL_PIPE_FMT(YVYU, YUV422_1, 1, (BIT(1) | BIT(2))),
	PIXEL_PIPE_FMT(GREY, MONO_Y8_G8_1, 0, (BIT(1) | BIT(2))),
	PIXEL_PIPE_FMT(RGB24, RGB888_YUV444_1, 1, (BIT(1) | BIT(2))),
	PIXEL_PIPE_FMT(BGR24, RGB888_YUV444_1, 0, (BIT(1) | BIT(2))),
	PIXEL_PIPE_FMT(ARGB32, RGBA888, 1, (BIT(1) | BIT(2))),
	PIXEL_PIPE_FMT(ABGR32, ARGB8888, 0, (BIT(1) | BIT(2))),
	PIXEL_PIPE_FMT(RGBA32, ARGB8888, 1, (BIT(1) | BIT(2))),
	PIXEL_PIPE_FMT(BGRA32, RGBA888, 0, (BIT(1) | BIT(2))),
	/* TODO - need to add the semiplanar & planar formats */
};

/*
 * FIXME: Helper to know colorspace of a format
 * This below to the video_common part and should be moved there
 */
#define VIDEO_COLORSPACE_RAW	0
#define VIDEO_COLORSPACE_RGB	1
#define VIDEO_COLORSPACE_YUV	2
#define VIDEO_COLORSPACE(fmt)							\
	(((fmt) == VIDEO_PIX_FMT_RGB565X || (fmt) == VIDEO_PIX_FMT_RGB565 ||	\
	  (fmt) == VIDEO_PIX_FMT_BGR24 || (fmt) == VIDEO_PIX_FMT_RGB24 ||	\
	  (fmt) == VIDEO_PIX_FMT_ARGB32 || (fmt) == VIDEO_PIX_FMT_ABGR32 ||	\
	  (fmt) == VIDEO_PIX_FMT_RGBA32 || (fmt) == VIDEO_PIX_FMT_BGRA32 ||	\
	  (fmt) == VIDEO_PIX_FMT_XRGB32) ? VIDEO_COLORSPACE_RGB :		\
										\
	 ((fmt) == VIDEO_PIX_FMT_GREY ||					\
	  (fmt) == VIDEO_PIX_FMT_YUYV || (fmt) == VIDEO_PIX_FMT_YVYU ||		\
	  (fmt) == VIDEO_PIX_FMT_VYUY || (fmt) == VIDEO_PIX_FMT_UYVY ||		\
	  (fmt) == VIDEO_PIX_FMT_XYUV32) ? VIDEO_COLORSPACE_YUV :		\
										\
	  VIDEO_COLORSPACE_RAW)

static const struct stm32_dcmipp_mapping *stm32_dcmipp_get_mapping(uint32_t pixelformat,
								   uint32_t pipe)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(stm32_dcmipp_format_support); i++) {
		if (pixelformat == stm32_dcmipp_format_support[i].pixelformat &&
		    BIT(pipe) & stm32_dcmipp_format_support[i].pipes) {
			return &stm32_dcmipp_format_support[i];
		}
	}

	return NULL;
}

#define INPUT_FMT(fmt, bpp, dt)				\
	{						\
		.pixelformat = VIDEO_PIX_FMT_##fmt,	\
		.csi_bpp = DCMIPP_CSI_DT_BPP##bpp,	\
		.csi_dt = DCMIPP_DT_##dt,		\
	}
static int stm32_dcmipp_get_input_info(uint32_t pixelformat, uint32_t *csi_bpp, uint32_t *csi_dt)
{
	/* DCMIPP input format descriptions */
	static const struct {
		uint32_t pixelformat;
		uint32_t csi_bpp;
		uint32_t csi_dt;
	} input_fmt_desc[] = {
		INPUT_FMT(SBGGR8, 8, RAW8), INPUT_FMT(SGBRG8, 8, RAW8),
		INPUT_FMT(SGRBG8, 8, RAW8), INPUT_FMT(SRGGB8, 8, RAW8),
		INPUT_FMT(SBGGR10P, 10, RAW10), INPUT_FMT(SGBRG10P, 10, RAW10),
		INPUT_FMT(SGRBG10P, 10, RAW10), INPUT_FMT(SRGGB10P, 10, RAW10),
		INPUT_FMT(SBGGR12P, 12, RAW12), INPUT_FMT(SGBRG12P, 12, RAW12),
		INPUT_FMT(SGRBG12P, 12, RAW12), INPUT_FMT(SRGGB12P, 12, RAW12),
		INPUT_FMT(SBGGR14P, 14, RAW14), INPUT_FMT(SGBRG14P, 14, RAW14),
		INPUT_FMT(SGRBG14P, 14, RAW14), INPUT_FMT(SRGGB14P, 14, RAW14),
		INPUT_FMT(RGB565, 8, RGB565),
		INPUT_FMT(YUYV, 8, YUV422_8),
	};
	int i;

	for (i = 0; i < ARRAY_SIZE(input_fmt_desc); i++) {
		if (pixelformat == input_fmt_desc[i].pixelformat) {
			*csi_bpp = input_fmt_desc[i].csi_bpp;
			*csi_dt = input_fmt_desc[i].csi_dt;
			return 0;
		}
	}

	return -EINVAL;
}

static int stm32_dcmipp_set_fmt(const struct device *dev, enum video_endpoint_id ep,
				struct video_format *fmt)
{
	struct stm32_dcmipp_pipe_data *pipe = dev->data;
	struct stm32_dcmipp_data *dcmipp = pipe->dcmipp;
	const struct stm32_dcmipp_mapping *mapping;
	int ret = 0;

	if (!fmt) {
		return -EINVAL;
	}

	/* Sanities format given */
	mapping = stm32_dcmipp_get_mapping(fmt->pixelformat, pipe->id);
	if (!mapping) {
		LOG_ERR("Pixelformat %s/0x%x is not supported",
			VIDEO_FOURCC_TO_STR(fmt->pixelformat), fmt->pixelformat);
		return -EINVAL;
	}

	/* In case of DUMP pipe, verify that requested format matched with input format */
	if (pipe->id == DCMIPP_PIPE0 &&
	    mapping->dump.input_pixelformat != dcmipp->sensor_fmt.pixelformat) {
		LOG_ERR("Dump pipe output format incompatible with sensor format");
		return -EINVAL;
	}

	if (fmt->width < STM32_DCMIPP_WIDTH_MIN || fmt->width > STM32_DCMIPP_WIDTH_MAX ||
	    fmt->height < STM32_DCMIPP_HEIGHT_MIN || fmt->height > STM32_DCMIPP_HEIGHT_MAX) {
		LOG_ERR("Format width/height (%d x %d) does not match caps",
			fmt->width, fmt->height);
		return -EINVAL;
	}

	if (fmt->pitch != fmt->width * video_bits_per_pixel(fmt->pixelformat) / BITS_PER_BYTE) {
		LOG_ERR("Format pitch (%d) does not match with format", fmt->pitch);
		return -EINVAL;
	}

	if ((fmt->pitch * fmt->height) > CONFIG_VIDEO_BUFFER_POOL_SZ_MAX) {
		LOG_ERR("Frame too large for VIDEO_BUFFER_POOL");
		return -EINVAL;
	}

	k_mutex_lock(&pipe->lock, K_FOREVER);

	if (pipe->is_streaming) {
		ret = -EBUSY;
		goto out;
	}

	if (pipe->id == DCMIPP_PIPE1 || pipe->id == DCMIPP_PIPE2) {
		uint32_t post_isp_decimate_width = dcmipp->sensor_fmt.width /
						   dcmipp->isp_dec_hratio;
		uint32_t post_isp_decimate_height = dcmipp->sensor_fmt.height /
						    dcmipp->isp_dec_vratio;

		if (fmt->width < (post_isp_decimate_width / STM32_DCMIPP_MAX_PIPE_SCALE_FACTOR) ||
		    fmt->height < (post_isp_decimate_height / STM32_DCMIPP_MAX_PIPE_SCALE_FACTOR) ||
		    fmt->width > post_isp_decimate_width ||
		    fmt->height > post_isp_decimate_height) {
			LOG_ERR("Requested resolution cannot be achieved");
			ret = -EINVAL;
			goto out;
		}
	}

	pipe->fmt = *fmt;

out:
	k_mutex_unlock(&pipe->lock);

	return ret;
}

static int stm32_dcmipp_get_fmt(const struct device *dev, enum video_endpoint_id ep,
				struct video_format *fmt)
{
	struct stm32_dcmipp_pipe_data *pipe = dev->data;

	if (!fmt) {
		return -EINVAL;
	}

	*fmt = pipe->fmt;

	return 0;
}

static inline uint32_t stm32_dcmipp_isp_decimation_to_hal(int32_t ratio)
{
	if (ratio == 1) {
		return DCMIPP_VDEC_ALL;
	} else if (ratio == 2) {
		return DCMIPP_VDEC_1_OUT_2;
	} else if (ratio == 4) {
		return DCMIPP_VDEC_1_OUT_4;
	} else {
		return DCMIPP_VDEC_1_OUT_8;
	}
}

static int stm32_dcmipp_set_isp_decimation(struct stm32_dcmipp_data *dcmipp)
{
	int ret;
	DCMIPP_DecimationConfTypeDef ispdec_cfg;

	/* If no ISP decimation is necessary, disable the HW block */
	if (dcmipp->isp_dec_hratio == 1 && dcmipp->isp_dec_vratio == 1) {
		ret = HAL_DCMIPP_PIPE_DisableISPDecimation(&dcmipp->hdcmipp, DCMIPP_PIPE1);
		if (ret != HAL_OK) {
			LOG_ERR("Failed to disable ISP decimation");
			return -EIO;
		}

		return 0;
	}

	ispdec_cfg.HRatio = stm32_dcmipp_isp_decimation_to_hal(dcmipp->isp_dec_hratio),
	ispdec_cfg.VRatio = stm32_dcmipp_isp_decimation_to_hal(dcmipp->isp_dec_vratio),

	/* Configure the ISP decimation */
	ret = HAL_DCMIPP_PIPE_SetISPDecimationConfig(&dcmipp->hdcmipp, DCMIPP_PIPE1, &ispdec_cfg);
	if (ret != HAL_OK) {
		LOG_ERR("Failed to configure the ISP decimation");
		return -EIO;
	}
	ret = HAL_DCMIPP_PIPE_EnableISPDecimation(&dcmipp->hdcmipp, DCMIPP_PIPE1);
	if (ret != HAL_OK) {
		LOG_ERR("Failed to enable the ISP decimation");
		return -EIO;
	}

	return 0;
}

#define STM32_DCMIPP_DSIZE_HVRATIO_CONS   8192
#define STM32_DCMIPP_DSIZE_HVRATIO_MAX    65535
#define STM32_DCMIPP_DSIZE_HVDIV_CONS     1024
#define STM32_DCMIPP_DSIZE_HVDIV_MAX      1023
static int stm32_dcmipp_set_downscale(struct stm32_dcmipp_pipe_data *pipe)
{
	struct stm32_dcmipp_data *dcmipp = pipe->dcmipp;
	uint32_t post_decimate_width = dcmipp->sensor_fmt.width / dcmipp->isp_dec_hratio;
	uint32_t post_decimate_height = dcmipp->sensor_fmt.height / dcmipp->isp_dec_vratio;
	DCMIPP_DecimationConfTypeDef dec_cfg;
	DCMIPP_DownsizeTypeDef downsize_cfg;
	struct video_format *fmt = &pipe->fmt;
	uint32_t hdec = 1, vdec = 1;
	int ret;

	if (fmt->width == post_decimate_width && fmt->height == post_decimate_height) {
		ret = HAL_DCMIPP_PIPE_DisableDecimation(&dcmipp->hdcmipp, pipe->id);
		if (ret != HAL_OK) {
			LOG_ERR("Failed to disable the pipe decimation");
			return -EIO;
		}

		ret = HAL_DCMIPP_PIPE_DisableDownsize(&dcmipp->hdcmipp, pipe->id);
		if (ret != HAL_OK) {
			LOG_ERR("Failed to disable the pipe downsize");
			return -EIO;
		}

		return 0;
	}

	/* Compute decimation factors (HDEC/VDEC) */
	while (fmt->width * STM32_DCMIPP_MAX_PIPE_DSIZE_FACTOR < post_decimate_width) {
		hdec *= 2;
		post_decimate_width /= 2;
	}
	while (fmt->height * STM32_DCMIPP_MAX_PIPE_DSIZE_FACTOR < post_decimate_height) {
		vdec *= 2;
		post_decimate_height /= 2;
	}

	if (hdec == 1 && vdec == 1) {
		ret = HAL_DCMIPP_PIPE_DisableDecimation(&dcmipp->hdcmipp, pipe->id);
		if (ret != HAL_OK) {
			LOG_ERR("Failed to disable the pipe decimation");
			return -EIO;
		}
	} else {
		/* Use same decimation factor in width / height to keep aspect ratio */
		dec_cfg.HRatio = stm32_dcmipp_isp_decimation_to_hal(hdec);
		dec_cfg.VRatio = stm32_dcmipp_isp_decimation_to_hal(vdec);

		ret = HAL_DCMIPP_PIPE_SetDecimationConfig(&dcmipp->hdcmipp, pipe->id, &dec_cfg);
		if (ret != HAL_OK) {
			LOG_ERR("Failed to disable the pipe decimation");
			return -EIO;
		}
		ret = HAL_DCMIPP_PIPE_EnableDecimation(&dcmipp->hdcmipp, pipe->id);
		if (ret != HAL_OK) {
			LOG_ERR("Failed to enable the pipe decimation");
			return -EIO;
		}
	}

	/* Compute downsize factor */
	downsize_cfg.HRatio = post_decimate_width * STM32_DCMIPP_DSIZE_HVRATIO_CONS / fmt->width;
	if (downsize_cfg.HRatio > STM32_DCMIPP_DSIZE_HVRATIO_MAX) {
		downsize_cfg.HRatio = STM32_DCMIPP_DSIZE_HVRATIO_MAX;
	}
	downsize_cfg.VRatio = post_decimate_height * STM32_DCMIPP_DSIZE_HVRATIO_CONS / fmt->height;
	if (downsize_cfg.VRatio > STM32_DCMIPP_DSIZE_HVRATIO_MAX) {
		downsize_cfg.VRatio = STM32_DCMIPP_DSIZE_HVRATIO_MAX;
	}
	downsize_cfg.HDivFactor = STM32_DCMIPP_DSIZE_HVDIV_CONS * fmt->width / post_decimate_width;
	if (downsize_cfg.HDivFactor > STM32_DCMIPP_DSIZE_HVDIV_MAX) {
		downsize_cfg.HDivFactor = STM32_DCMIPP_DSIZE_HVDIV_MAX;
	}
	downsize_cfg.VDivFactor =
		STM32_DCMIPP_DSIZE_HVDIV_CONS * fmt->height / post_decimate_height;
	if (downsize_cfg.VDivFactor > STM32_DCMIPP_DSIZE_HVDIV_MAX) {
		downsize_cfg.VDivFactor = STM32_DCMIPP_DSIZE_HVDIV_MAX;
	}
	downsize_cfg.HSize = fmt->width;
	downsize_cfg.VSize = fmt->height;

	ret = HAL_DCMIPP_PIPE_SetDownsizeConfig(&dcmipp->hdcmipp, pipe->id, &downsize_cfg);
	if (ret != HAL_OK) {
		LOG_ERR("Failed to configure the pipe downsize");
		return -EIO;
	}

	ret = HAL_DCMIPP_PIPE_EnableDownsize(&dcmipp->hdcmipp, pipe->id);
	if (ret != HAL_OK) {
		LOG_ERR("Failed to enable the pipe downsize");
		return -EIO;
	}

	return 0;
}

/* No clamping enabled */
/* RGB Full to YUV 601 Full */
const DCMIPP_ColorConversionConfTypeDef stm32_dcmipp_rgb_to_yuv = {
	.RR = 131, .RG = -110, .RB = -21, .RA = 128,
	.GR =  77, .GG =  150, .GB =  29, .GA =   0,
	.BR = -44, .BG =  -87, .BB = 131, .BA = 128,
};

/* YUV 601 Full to RGB Full */
const DCMIPP_ColorConversionConfTypeDef stm32_dcmipp_yuv_to_rgb = {
	.RR =  351, .RG = 256, .RB =   0, .RA = -175,
	.GR = -179, .GG = 256, .GB = -86, .GA =  132,
	.BR =    0, .BG = 256, .BB = 443, .BA = -222,
};

static int stm32_dcmipp_set_yuv_conversion(struct stm32_dcmipp_pipe_data *pipe,
					   uint32_t sensor_colorspace,
					   uint32_t output_colorspace)
{
	struct stm32_dcmipp_data *dcmipp = pipe->dcmipp;
	const DCMIPP_ColorConversionConfTypeDef *cfg = NULL;
	int ret;

	/* No YUV conversion on pipe 2 */
	if (pipe->id == DCMIPP_PIPE2) {
		return 0;
	}

	/* Perform YUV conversion if necessary and possible */
	if ((sensor_colorspace == VIDEO_COLORSPACE_RAW ||
	     sensor_colorspace == VIDEO_COLORSPACE_RGB) &&
	     output_colorspace == VIDEO_COLORSPACE_YUV) {
		/* Need to perform RGB to YUV conversion */
		cfg = &stm32_dcmipp_rgb_to_yuv;
	} else if (sensor_colorspace == VIDEO_COLORSPACE_YUV &&
		   output_colorspace == VIDEO_COLORSPACE_RGB) {
		/* Need to perform YUV to RGB conversion */
		cfg = &stm32_dcmipp_yuv_to_rgb;
	}

	if (!cfg) {
		ret = HAL_DCMIPP_PIPE_DisableYUVConversion(&dcmipp->hdcmipp, pipe->id);
		if (ret != HAL_OK) {
			LOG_ERR("Failed to disable YUV conversion");
			return -EIO;
		}

		return 0;
	}

	ret = HAL_DCMIPP_PIPE_SetYUVConversionConfig(&dcmipp->hdcmipp, pipe->id, cfg);
	if (ret != HAL_OK) {
		LOG_ERR("Failed to setup YUV conversion");
		return -EIO;
	}

	ret = HAL_DCMIPP_PIPE_EnableYUVConversion(&dcmipp->hdcmipp, pipe->id);
	if (ret != HAL_OK) {
		LOG_ERR("Failed to disable YUV conversion");
		return -EIO;
	}

	return 0;
}

static int stm32_dcmipp_stream_enable(const struct device *dev)
{
	struct stm32_dcmipp_pipe_data *pipe = dev->data;
	struct stm32_dcmipp_data *dcmipp = pipe->dcmipp;
	const struct stm32_dcmipp_config *config = dev->config;
	struct video_format *fmt = &pipe->fmt;
	const struct stm32_dcmipp_mapping *mapping;
	DCMIPP_CSI_PIPE_ConfTypeDef csi_pipe_cfg = { 0 };
	DCMIPP_PipeConfTypeDef pipe_cfg = { 0 };
	uint32_t csi_bpp, csi_dt;
	int ret;

	k_mutex_lock(&pipe->lock, K_FOREVER);

	if (pipe->is_streaming) {
		ret = -EALREADY;
		goto out;
	}

	/* Configure the sensor if nothing is already running */
	if (!dcmipp->enabled_pipe) {
		ret = video_set_format(config->sensor_dev, VIDEO_EP_OUT, &dcmipp->sensor_fmt);
		if (ret) {
			goto out;
		}

		ret = stm32_dcmipp_conf_csi(dcmipp->dev);
		if (ret) {
			LOG_ERR("Failed to configure CSI lanes & PHY");
			goto out;
		}
	}

	pipe->active = NULL;
	pipe->next = k_fifo_get(&pipe->fifo_in, K_NO_WAIT);
	if (!pipe->next) {
		LOG_ERR("No buffer available to start streaming");
		ret = -ENOMEM;
		goto out;
	}

	/* Set Virtual Channel config */
	ret = stm32_dcmipp_get_input_info(dcmipp->sensor_fmt.pixelformat, &csi_bpp, &csi_dt);
	if (ret) {
		LOG_ERR("Unsupported input format");
		goto out;
	}

	/* TODO - need to be able to use an alternate VC, info coming from the sensor */
	ret = HAL_DCMIPP_CSI_SetVCConfig(&dcmipp->hdcmipp, DCMIPP_VIRTUAL_CHANNEL0, csi_bpp);
	if (ret != HAL_OK) {
		LOG_ERR("Failed to set CSI configuration");
		ret = -EIO;
		goto out;
	}

	/* Configure the Pipe input */
	csi_pipe_cfg.DataTypeMode = DCMIPP_DTMODE_DTIDA;
	csi_pipe_cfg.DataTypeIDA  = csi_dt;
	ret = HAL_DCMIPP_CSI_PIPE_SetConfig(&dcmipp->hdcmipp, pipe->id, &csi_pipe_cfg);
	if (ret != HAL_OK) {
		LOG_ERR("Failed to configure pipe #%d input", pipe->id);
		ret = -EIO;
		goto out;
	}

	/* Configure Pipe */
	mapping = stm32_dcmipp_get_mapping(fmt->pixelformat, pipe->id);
	if (!mapping) {
		LOG_ERR("Failed to find pipe format mapping");
		ret = -EIO;
		goto out;
	}

	pipe_cfg.FrameRate  = DCMIPP_FRAME_RATE_ALL;
	if (pipe->id == DCMIPP_PIPE1 || pipe->id == DCMIPP_PIPE2) {
		pipe_cfg.PixelPipePitch =
			fmt->width * video_bits_per_pixel(fmt->pixelformat) / BITS_PER_BYTE;
		pipe_cfg.PixelPackerFormat = mapping->pixels.dcmipp_format;
	}
	ret = HAL_DCMIPP_PIPE_SetConfig(&dcmipp->hdcmipp, pipe->id, &pipe_cfg);
	if (ret != HAL_OK) {
		LOG_ERR("Failed to configure pipe #%d", pipe->id);
		ret = -EIO;
		goto out;
	}

	if (pipe->id == DCMIPP_PIPE0) {
		/* Only the PIPE0 has a limiter */
		/* Set Limiter to avoid buffer overflow, in words */
		ret = HAL_DCMIPP_PIPE_EnableLimitEvent(&dcmipp->hdcmipp, DCMIPP_PIPE0,
						       (fmt->pitch * fmt->height) / 4);
		if (ret != HAL_OK) {
			LOG_ERR("Failed to set limiter");
			ret = -EIO;
			goto out;
		}
	} else if (pipe->id == DCMIPP_PIPE1 || pipe->id == DCMIPP_PIPE2) {
		uint32_t sensor_colorspace = VIDEO_COLORSPACE(dcmipp->sensor_fmt.pixelformat);
		uint32_t output_colorspace = VIDEO_COLORSPACE(fmt->pixelformat);

		/* Enable / disable SWAPRB if necessary */
		if (mapping->pixels.swap_uv) {
			ret = HAL_DCMIPP_PIPE_EnableRedBlueSwap(&dcmipp->hdcmipp, pipe->id);
			if (ret != HAL_OK) {
				LOG_ERR("Failed to enable Red-Blue swap");
				ret = -EIO;
				goto out;
			}
		} else {
			ret = HAL_DCMIPP_PIPE_DisableRedBlueSwap(&dcmipp->hdcmipp, pipe->id);
			if (ret != HAL_OK) {
				LOG_ERR("Failed to disable Red-Blue swap");
				ret = -EIO;
				goto out;
			}
		}

		if (sensor_colorspace == VIDEO_COLORSPACE_RAW) {
			/* Enable demosaicing if input format is Bayer */
			ret = HAL_DCMIPP_PIPE_EnableISPRawBayer2RGB(&dcmipp->hdcmipp, DCMIPP_PIPE1);
			if (ret != HAL_OK) {
				LOG_ERR("Failed to enable demosaicing");
				ret = -EIO;
				goto out;
			}
		} else {
			/* Disable demosaicing */
			ret = HAL_DCMIPP_PIPE_DisableISPRawBayer2RGB(&dcmipp->hdcmipp,
								     DCMIPP_PIPE1);
			if (ret != HAL_OK) {
				LOG_ERR("Failed to disable demosaicing");
				ret = -EIO;
				goto out;
			}
		}

		/*
		 * FIXME - should not allow changing ISP decimation while one of pipe1 or pip2
		 * is already running
		 */
		/* Configure the ISP decimation */
		ret = stm32_dcmipp_set_isp_decimation(dcmipp);
		if (ret) {
			goto out;
		}

		/* Configure the Pipe decimation / downsize */
		ret = stm32_dcmipp_set_downscale(pipe);
		if (ret) {
			goto out;
		}

		/* Configure YUV conversion */
		ret = stm32_dcmipp_set_yuv_conversion(pipe, sensor_colorspace, output_colorspace);
		if (ret) {
			goto out;
		}
	}

	/* Enable the DCMIPP Pipeline */
	ret = HAL_DCMIPP_CSI_PIPE_Start(&dcmipp->hdcmipp, pipe->id, DCMIPP_VIRTUAL_CHANNEL0,
					(uint32_t)pipe->next->buffer, DCMIPP_MODE_CONTINUOUS);
	if (ret != HAL_OK) {
		LOG_ERR("Failed to start the pipeline");
		ret = -EIO;
		goto out;
	}

	/* It is necessary to start the sensor as well if nothing else is running */
	if (!dcmipp->enabled_pipe) {
		ret = video_stream_start(config->sensor_dev);
		if (ret) {
			LOG_ERR("Failed to start the sensor");
			HAL_DCMIPP_CSI_PIPE_Stop(&dcmipp->hdcmipp, pipe->id,
						 DCMIPP_VIRTUAL_CHANNEL0);
			ret = -EIO;
			goto out;
		}
	}

	pipe->state = STM32_DCMIPP_RUNNING;
	pipe->is_streaming = true;
	dcmipp->enabled_pipe++;

out:
	k_mutex_unlock(&pipe->lock);

	return ret;
}

static int stm32_dcmipp_stream_disable(const struct device *dev)
{
	struct stm32_dcmipp_pipe_data *pipe = dev->data;
	struct stm32_dcmipp_data *dcmipp = pipe->dcmipp;
	const struct stm32_dcmipp_config *config = dev->config;
	int ret;

	k_mutex_lock(&pipe->lock, K_FOREVER);

	if (!pipe->is_streaming) {
		ret = -EINVAL;
		goto out;
	}

	/* Disable the DCMIPP Pipeline */
	ret = HAL_DCMIPP_CSI_PIPE_Stop(&dcmipp->hdcmipp, pipe->id, DCMIPP_VIRTUAL_CHANNEL0);
	if (ret != HAL_OK) {
		LOG_ERR("Failed to stop the pipeline");
		ret = -EIO;
		goto out;
	}

	/* Turn off the sensor nobody else is using the DCMIPP */
	if (dcmipp->enabled_pipe == 1) {
		ret = video_stream_stop(config->sensor_dev);
		if (ret) {
			LOG_ERR("Failed to stop the sensor");
			ret = -EIO;
			goto out;
		}
	}

	/* Release the video buffer allocated when start streaming */
	/* TODO - to be fixed, not the proper buffer here */
	k_fifo_put(&pipe->fifo_in, pipe->active);

	pipe->state = STM32_DCMIPP_STOPPED;
	pipe->is_streaming = false;
	dcmipp->enabled_pipe--;

out:
	k_mutex_unlock(&pipe->lock);

	return ret;
}

static int stm32_dcmipp_set_stream(const struct device *dev, bool enable)
{
	return enable ? stm32_dcmipp_stream_enable(dev) : stm32_dcmipp_stream_disable(dev);
}

static int stm32_dcmipp_enqueue(const struct device *dev, enum video_endpoint_id ep,
				struct video_buffer *vbuf)
{
	struct stm32_dcmipp_pipe_data *pipe = dev->data;
	struct stm32_dcmipp_data *dcmipp = pipe->dcmipp;
	int ret;

	if (!vbuf) {
		return -EINVAL;
	}

	k_mutex_lock(&pipe->lock, K_FOREVER);

	if (pipe->fmt.pitch * pipe->fmt.height > vbuf->size) {
		return -EINVAL;
	}

	if (pipe->state == STM32_DCMIPP_WAIT_FOR_BUFFER) {
		LOG_DBG("Restart CPTREQ after wait for buffer");
		pipe->next = vbuf;
		ret = HAL_DCMIPP_PIPE_SetMemoryAddress(&dcmipp->hdcmipp, pipe->id,
						       DCMIPP_MEMORY_ADDRESS_0,
						       (uint32_t)pipe->next->buffer);
		if (ret) {
			LOG_ERR("Failed to update memory address");
			return -EIO;
		}
		if (pipe->id == DCMIPP_PIPE0) {
			SET_BIT(dcmipp->hdcmipp.Instance->P0FCTCR, DCMIPP_P0FCTCR_CPTREQ);
		} else if (pipe->id == DCMIPP_PIPE1) {
			SET_BIT(dcmipp->hdcmipp.Instance->P1FCTCR, DCMIPP_P1FCTCR_CPTREQ);
		} else if (pipe->id == DCMIPP_PIPE2) {
			SET_BIT(dcmipp->hdcmipp.Instance->P2FCTCR, DCMIPP_P2FCTCR_CPTREQ);
		}
		pipe->state = STM32_DCMIPP_RUNNING;
	} else {
		k_fifo_put(&pipe->fifo_in, vbuf);
	}

	k_mutex_unlock(&pipe->lock);

	return 0;
}

static int stm32_dcmipp_dequeue(const struct device *dev, enum video_endpoint_id ep,
				struct video_buffer **vbuf, k_timeout_t timeout)
{
	struct stm32_dcmipp_pipe_data *pipe = dev->data;

	if (!vbuf) {
		return -EINVAL;
	}

	*vbuf = k_fifo_get(&pipe->fifo_out, timeout);
	if (!*vbuf) {
		return -EAGAIN;
	}

	return 0;
}

/*
 * TODO: caps aren't yet handled hence give back straight the caps given by the
 * sensor.  Normally this should be the intersection of what the sensor produces
 * vs what the DCMIPP can input (for pipe0) and, for pipe 1 and 2, for a given
 * input format, generate caps based on capabilities, color conversion, decimation
 * etc
 */
static int stm32_dcmipp_get_caps(const struct device *dev, enum video_endpoint_id ep,
				 struct video_caps *caps)
{
	const struct stm32_dcmipp_config *config = dev->config;
	int ret;

	ret = video_get_caps(config->sensor_dev, ep, caps);

	caps->min_vbuf_count = 1;
	caps->min_line_count = LINE_COUNT_HEIGHT;
	caps->max_line_count = LINE_COUNT_HEIGHT;

	return ret;
}

static const uint8_t stm32_dcmipp_isp_dec_val[] = {
	1, 2, 4, 8
};

static const char *const stm32_dcmipp_isp_dec_menu[] = {
	"Disabled",
	"1/2",
	"1/4",
	"1/8",
	NULL
};

static int stm32_dcmipp_init_controls(const struct device *dev)
{
	struct stm32_dcmipp_data *dcmipp = dev->data;
	struct stm32_dcmipp_ctrls *ctrls = &dcmipp->ctrls;
	int ret;

	ret = video_init_menu_ctrl(&ctrls->isp_hdec, dev, VIDEO_CID_STM32_DCMIPP_ISP_HDEC, 0,
				   stm32_dcmipp_isp_dec_menu);
	if (ret) {
		return ret;
	}

	return video_init_menu_ctrl(&ctrls->isp_vdec, dev, VIDEO_CID_STM32_DCMIPP_ISP_VDEC, 0,
				    stm32_dcmipp_isp_dec_menu);
}

static int stm32_dcmipp_set_ctrl(const struct device *dev, uint32_t id)
{
	struct stm32_dcmipp_pipe_data *pipe = dev->data;
	struct stm32_dcmipp_data *dcmipp = pipe->dcmipp;

	switch (id) {
	case VIDEO_CID_STM32_DCMIPP_ISP_HDEC:
		dcmipp->isp_dec_hratio = stm32_dcmipp_isp_dec_val[dcmipp->ctrls.isp_hdec.val];
		break;
	case VIDEO_CID_STM32_DCMIPP_ISP_VDEC:
		dcmipp->isp_dec_vratio = stm32_dcmipp_isp_dec_val[dcmipp->ctrls.isp_vdec.val];
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static DEVICE_API(video, stm32_dcmipp_driver_api) = {
	.set_format = stm32_dcmipp_set_fmt,
	.get_format = stm32_dcmipp_get_fmt,
	.set_stream = stm32_dcmipp_set_stream,
	.enqueue = stm32_dcmipp_enqueue,
	.dequeue = stm32_dcmipp_dequeue,
	.get_caps = stm32_dcmipp_get_caps,
	.set_ctrl = stm32_dcmipp_set_ctrl,
};

static int stm32_dcmipp_enable_clock(const struct device *dev)
{
	const struct stm32_dcmipp_config *config = dev->config;
	const struct device *cc_node = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);
	int err;

	if (!device_is_ready(cc_node)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	/* Turn on DCMIPP peripheral clock */
	/* TODO - retrieve clock index from the DT instead of hardcode here */
	err = clock_control_configure(cc_node, (clock_control_subsys_t)&config->pclken[1], NULL);
	if (err < 0) {
		LOG_ERR("Failed to configure DCMIPP clock. Error %d", err);
		return err;
	}

	err = clock_control_on(cc_node, (clock_control_subsys_t *)&config->pclken[0]);
	if (err < 0) {
		LOG_ERR("Failed to enable DCMIPP clock. Error %d", err);
		return err;
	}

	/* Turn on CSI peripheral clock */
	err = clock_control_on(cc_node, (clock_control_subsys_t *)&config->pclken[2]);
	if (err < 0) {
		LOG_ERR("Failed to enable CSI clock. Error %d", err);
		return err;
	}

	return 0;
}

static int stm32_dcmipp_init(const struct device *dev)
{
	const struct stm32_dcmipp_config *cfg = dev->config;
	struct stm32_dcmipp_data *dcmipp = dev->data;

	int err;

#if defined(CONFIG_SOC_SERIES_STM32N6X)
	RIMC_MasterConfig_t rimc = {0};
#endif

	dcmipp->enabled_pipe = 0;

	dcmipp->isp_dec_hratio = 1;
	dcmipp->isp_dec_vratio = 1;

	/* TODO: Add parallel mode support */

	/* Enable DCMIPP / CSI clocks */
	err = stm32_dcmipp_enable_clock(dev);
	if (err) {
		LOG_ERR("Clock enabling failed.");
		return -EIO;
	}

	/* Reset DCMIPP & CSI */
	if (!device_is_ready(cfg->reset_dcmipp.dev)) {
		LOG_ERR("reset controller not ready");
		return -ENODEV;
	}
	reset_line_toggle_dt(&cfg->reset_dcmipp);
	reset_line_toggle_dt(&cfg->reset_csi);

	dcmipp->dev = dev;

	/* Run IRQ init */
	cfg->irq_config(dev);

#if defined(CONFIG_SOC_SERIES_STM32N6X)
	rimc.MasterCID = RIF_CID_1;
	rimc.SecPriv = RIF_ATTRIBUTE_SEC | RIF_ATTRIBUTE_PRIV;
	HAL_RIF_RIMC_ConfigMasterAttributes(RIF_MASTER_INDEX_DCMIPP, &rimc);
	HAL_RIF_RISC_SetSlaveSecureAttributes(RIF_RISC_PERIPH_INDEX_DCMIPP,
					      RIF_ATTRIBUTE_SEC | RIF_ATTRIBUTE_PRIV);
#endif

	/* Initialize DCMI peripheral */
	err = HAL_DCMIPP_Init(&dcmipp->hdcmipp);
	if (err != HAL_OK) {
		LOG_ERR("DCMIPP initialization failed.");
		return -EIO;
	}

	err = stm32_dcmipp_init_controls(dev);
	if (err) {
		return err;
	}

	LOG_DBG("%s initialized", dev->name);

	return 0;
}

static int stm32_dcmipp_pipe_init(const struct device *dev)
{
	struct stm32_dcmipp_pipe_data *pipe = dev->data;
	struct stm32_dcmipp_data *dcmipp = pipe->dcmipp;

	k_mutex_init(&pipe->lock);
	k_fifo_init(&pipe->fifo_in);
	k_fifo_init(&pipe->fifo_out);

	/* TODO - need to init formats to valid values */

	/* Store the pipe data pointer into dcmipp data structure */
	dcmipp->pipe[pipe->id] = pipe;

	return 0;
}

static void stm32_dcmipp_isr(const struct device *dev)
{
	struct stm32_dcmipp_data *dcmipp = dev->data;

	HAL_DCMIPP_IRQHandler(&dcmipp->hdcmipp);
}

#define DCMIPP_PIPE_INIT_DEFINE(node_id, inst)						\
	static struct stm32_dcmipp_pipe_data stm32_dcmipp_##inst_pipe_##node_id = {	\
		.id = DT_CAT(node_id, _CHILD_IDX),					\
		.dcmipp = &stm32_dcmipp_data_##inst,					\
	};										\
											\
	DEVICE_DT_DEFINE(node_id, &stm32_dcmipp_pipe_init, NULL,			\
			 &stm32_dcmipp_##inst_pipe_##node_id,				\
			 &stm32_dcmipp_config_##inst,					\
			 POST_KERNEL, CONFIG_VIDEO_INIT_PRIORITY,			\
			 &stm32_dcmipp_driver_api);					\

#define SOURCE_DEV(inst) DEVICE_DT_GET(DT_INST_PHANDLE(inst, sensor))

#define STM32_DCMIPP_INIT(inst)									\
	static const struct stm32_pclken stm32_dcmipp_pclken_##inst[] =				\
					 STM32_DT_INST_CLOCKS(inst);				\
												\
	static void stm32_dcmipp_irq_config_##inst(const struct device *dev)			\
	{											\
		IRQ_CONNECT(DT_INST_IRQN(inst), DT_INST_IRQ(inst, priority),			\
			    stm32_dcmipp_isr, DEVICE_DT_INST_GET(inst), 0);			\
		irq_enable(DT_INST_IRQN(inst));							\
	}											\
												\
	static struct stm32_dcmipp_data stm32_dcmipp_data_##inst = {				\
		.hdcmipp = {									\
			.Instance = (DCMIPP_TypeDef *)DT_INST_REG_ADDR(inst),			\
		},										\
		.sensor_fmt = {									\
			.pixelformat =								\
				VIDEO_FOURCC_FROM_STR(						\
						CONFIG_VIDEO_STM32_DCMIPP_SENSOR_PIXEL_FORMAT),	\
			.width = CONFIG_VIDEO_STM32_DCMIPP_SENSOR_WIDTH,			\
			.height = CONFIG_VIDEO_STM32_DCMIPP_SENSOR_HEIGHT,			\
		},										\
	};											\
												\
	static const struct stm32_dcmipp_config stm32_dcmipp_config_##inst = {			\
		.pclken = stm32_dcmipp_pclken_##inst,						\
		.pclk_len = DT_INST_NUM_CLOCKS(inst),						\
		.irq_config = stm32_dcmipp_irq_config_##inst,					\
		.sensor_dev = SOURCE_DEV(inst),							\
		.reset_dcmipp = RESET_DT_SPEC_INST_GET_BY_IDX(inst, 0),				\
		.reset_csi = RESET_DT_SPEC_INST_GET_BY_IDX(inst, 1),				\
		.csi.nb_lanes = DT_PROP_LEN(DT_INST_ENDPOINT_BY_ID(inst, 0, 0), data_lanes),	\
		.csi.lanes[0] = DT_PROP_BY_IDX(DT_INST_ENDPOINT_BY_ID(inst, 0, 0),		\
					       data_lanes, 0),					\
		.csi.lanes[1] = COND_CODE_1(DT_PROP_LEN(DT_INST_ENDPOINT_BY_ID(inst, 0, 0),	\
							data_lanes),				\
					    (0),						\
					    (DT_PROP_BY_IDX(DT_INST_ENDPOINT_BY_ID(inst, 0, 0),	\
							    data_lanes, 1))),			\
	};											\
												\
	DEVICE_DT_INST_DEFINE(inst, &stm32_dcmipp_init,						\
		    NULL, &stm32_dcmipp_data_##inst,						\
		    &stm32_dcmipp_config_##inst,						\
		    POST_KERNEL, CONFIG_VIDEO_INIT_PRIORITY,					\
		    NULL);									\
												\
	DT_FOREACH_CHILD_VARGS(DT_INST_PORT_BY_ID(inst, 1), DCMIPP_PIPE_INIT_DEFINE, inst);	\
												\
	VIDEO_DEVICE_DEFINE(dcmipp_##inst, DEVICE_DT_INST_GET(inst), SOURCE_DEV(inst));

DT_INST_FOREACH_STATUS_OKAY(STM32_DCMIPP_INIT)
