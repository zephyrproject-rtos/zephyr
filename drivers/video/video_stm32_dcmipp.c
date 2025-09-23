/*
 * Copyright (c) 2025 STMicroelectronics.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/drivers/video.h>
#include <zephyr/drivers/video-controls.h>
#include <zephyr/dt-bindings/video/video-interfaces.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/video/stm32_dcmipp.h>

#include "video_ctrls.h"
#include "video_device.h"

#define DT_DRV_COMPAT st_stm32_dcmipp

/* On STM32MP13X HAL, below two functions have different names */
#if defined(CONFIG_SOC_SERIES_STM32MP13X)
#define HAL_DCMIPP_PIPE_SetConfig HAL_DCMIPP_PIPE_Config
#define HAL_DCMIPP_PARALLEL_SetConfig HAL_DCMIPP_SetParallelConfig
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32n6_dcmipp)
#define STM32_DCMIPP_HAS_CSI
#define STM32_DCMIPP_HAS_PIXEL_PIPES
#endif

#if defined(STM32_DCMIPP_HAS_PIXEL_PIPES)
/* Weak function declaration in order to interface with external ISP handler */
void __weak stm32_dcmipp_isp_vsync_update(DCMIPP_HandleTypeDef *hdcmipp, uint32_t Pipe)
{
}

int __weak stm32_dcmipp_isp_init(DCMIPP_HandleTypeDef *hdcmipp, const struct device *source)
{
	return 0;
}

int __weak stm32_dcmipp_isp_start(void)
{
	return 0;
}

int __weak stm32_dcmipp_isp_stop(void)
{
	return 0;
}
#endif

LOG_MODULE_REGISTER(stm32_dcmipp, CONFIG_VIDEO_LOG_LEVEL);

typedef void (*irq_config_func_t)(const struct device *dev);

enum stm32_dcmipp_state {
	STM32_DCMIPP_STOPPED = 0,
	STM32_DCMIPP_WAIT_FOR_BUFFER,
	STM32_DCMIPP_RUNNING,
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
	struct video_rect crop;
	struct video_rect compose;
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

	/* FIXME - there should be a mutex to protect enabled_pipe */
	unsigned int enabled_pipe;
	struct video_format source_fmt;

#if defined(STM32_DCMIPP_HAS_PIXEL_PIPES)
	/* Store the ISP decimation block ratio */
	int32_t isp_dec_hratio;
	int32_t isp_dec_vratio;
#endif

	struct stm32_dcmipp_pipe_data *pipe[STM32_DCMIPP_MAX_PIPE_NB];
};

struct stm32_dcmipp_config {
	const struct stm32_pclken dcmipp_pclken;
	const struct stm32_pclken dcmipp_pclken_ker;
	irq_config_func_t irq_config;
	const struct pinctrl_dev_config *pctrl;
	const struct device *source_dev;
	const struct reset_dt_spec reset_dcmipp;
	int bus_type;
#if defined(STM32_DCMIPP_HAS_CSI)
	const struct stm32_pclken csi_pclken;
	const struct reset_dt_spec reset_csi;
	struct {
		uint32_t nb_lanes;
		uint8_t lanes[2];
	} csi;
#endif
	struct {
		uint32_t vs_polarity;
		uint32_t hs_polarity;
		uint32_t pck_polarity;
	} parallel;
};

#define STM32_DCMIPP_WIDTH_MIN	16
#define STM32_DCMIPP_HEIGHT_MIN	2
#define STM32_DCMIPP_WIDTH_MAX	4094
#define STM32_DCMIPP_HEIGHT_MAX	4094

#define VIDEO_FMT_IS_SEMI_PLANAR(fmt)			\
	(((fmt)->pixelformat == VIDEO_PIX_FMT_NV12 ||	\
	  (fmt)->pixelformat == VIDEO_PIX_FMT_NV21 ||	\
	  (fmt)->pixelformat == VIDEO_PIX_FMT_NV16 ||	\
	  (fmt)->pixelformat == VIDEO_PIX_FMT_NV61) ? true : false)

#define VIDEO_FMT_IS_PLANAR(fmt)			\
	(((fmt)->pixelformat == VIDEO_PIX_FMT_YUV420 ||	\
	  (fmt)->pixelformat == VIDEO_PIX_FMT_YVU420) ? true : false)

#define VIDEO_Y_PLANE_PITCH(fmt)						\
	((VIDEO_FMT_IS_PLANAR(fmt) || VIDEO_FMT_IS_SEMI_PLANAR(fmt)) ?		\
	 (fmt)->width : (fmt)->pitch)

#define VIDEO_FMT_PLANAR_Y_PLANE_SIZE(fmt)	((fmt)->width * (fmt)->height)

static void stm32_dcmipp_set_next_buffer_addr(struct stm32_dcmipp_pipe_data *pipe)
{
	struct stm32_dcmipp_data *dcmipp = pipe->dcmipp;
#if defined(STM32_DCMIPP_HAS_PIXEL_PIPES)
	struct video_format *fmt = &pipe->fmt;
#endif
	uint8_t *plane = pipe->next->buffer;

	/* TODO - the HAL is missing a SetMemoryAddress for auxiliary addresses */
	/* Update main buffer address */
	if (pipe->id == DCMIPP_PIPE0) {
		WRITE_REG(dcmipp->hdcmipp.Instance->P0PPM0AR1, (uint32_t)plane);
	}
#if defined(STM32_DCMIPP_HAS_PIXEL_PIPES)
	else if (pipe->id == DCMIPP_PIPE1) {
		WRITE_REG(dcmipp->hdcmipp.Instance->P1PPM0AR1, (uint32_t)plane);
	} else {
		WRITE_REG(dcmipp->hdcmipp.Instance->P2PPM0AR1, (uint32_t)plane);
	}

	if (pipe->id != DCMIPP_PIPE1) {
		return;
	}

	if (VIDEO_FMT_IS_SEMI_PLANAR(fmt) || VIDEO_FMT_IS_PLANAR(fmt)) {
		/* Y plane has 8 bit per pixel, next plane is located at off + width * height */
		plane += VIDEO_FMT_PLANAR_Y_PLANE_SIZE(fmt);

		WRITE_REG(dcmipp->hdcmipp.Instance->P1PPM1AR1, (uint32_t)plane);

		if (VIDEO_FMT_IS_PLANAR(fmt)) {
			/* In case of YUV420 / YVU420, U plane has half width / half height */
			plane += VIDEO_FMT_PLANAR_Y_PLANE_SIZE(fmt) / 4;

			WRITE_REG(dcmipp->hdcmipp.Instance->P1PPM2AR1, (uint32_t)plane);
		}
	}
#endif
}

/* Callback getting called for each frame written into memory */
void HAL_DCMIPP_PIPE_FrameEventCallback(DCMIPP_HandleTypeDef *hdcmipp, uint32_t Pipe)
{
	struct stm32_dcmipp_data *dcmipp =
			CONTAINER_OF(hdcmipp, struct stm32_dcmipp_data, hdcmipp);
	struct stm32_dcmipp_pipe_data *pipe = dcmipp->pipe[Pipe];
	uint32_t bytesused;
	int ret;

	__ASSERT(pipe->active, "Unexpected behavior, active_buf must not be NULL");

	/* Counter is only available on Pipe0 */
	if (Pipe == DCMIPP_PIPE0) {
		ret = HAL_DCMIPP_PIPE_GetDataCounter(hdcmipp, Pipe, &bytesused);
		if (ret != HAL_OK) {
			LOG_WRN("Failed to read counter - buffer in error");
			pipe->active->bytesused = 0;
		} else {
			pipe->active->bytesused = bytesused;
		}
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

#if defined(STM32_DCMIPP_HAS_PIXEL_PIPES)
	/*
	 * Let the external ISP handler know that a VSYNC happened a new statistics are
	 * thus available
	 */
	stm32_dcmipp_isp_vsync_update(hdcmipp, Pipe);
#endif

	if (pipe->state != STM32_DCMIPP_RUNNING) {
		return;
	}

	pipe->active = pipe->next;

	pipe->next = k_fifo_get(&pipe->fifo_in, K_NO_WAIT);
	if (pipe->next == NULL) {
		LOG_DBG("No buffer available, pause streaming");
		/*
		 * Disable Capture Request
		 * Use direct register access here since we don't want to wait until
		 * getting CPTACT down here since we still need to handle other stuff
		 */
		pipe->state = STM32_DCMIPP_WAIT_FOR_BUFFER;
		if (Pipe == DCMIPP_PIPE0) {
			CLEAR_BIT(hdcmipp->Instance->P0FCTCR, DCMIPP_P0FCTCR_CPTREQ);
		}
#if defined(STM32_DCMIPP_HAS_PIXEL_PIPES)
		else if (Pipe == DCMIPP_PIPE1) {
			CLEAR_BIT(hdcmipp->Instance->P1FCTCR, DCMIPP_P1FCTCR_CPTREQ);
		} else if (Pipe == DCMIPP_PIPE2) {
			CLEAR_BIT(hdcmipp->Instance->P2FCTCR, DCMIPP_P2FCTCR_CPTREQ);
		}
#endif
		return;
	}

	/* Update buffer address */
	stm32_dcmipp_set_next_buffer_addr(pipe);
}

#if defined(STM32_DCMIPP_HAS_CSI)
#define INPUT_FMT(fmt, dcmipp_fmt, bpp, dt)			\
	{							\
		.pixelformat = VIDEO_PIX_FMT_##fmt,		\
		.dcmipp_format = DCMIPP_FORMAT_##dcmipp_fmt,	\
		.dcmipp_csi_bpp = DCMIPP_CSI_DT_BPP##bpp,	\
		.dcmipp_csi_dt = DCMIPP_DT_##dt,		\
	}
#else
#define INPUT_FMT(fmt, dcmipp_fmt, bpp, dt)			\
	{							\
		.pixelformat = VIDEO_PIX_FMT_##fmt,		\
		.dcmipp_format = DCMIPP_FORMAT_##dcmipp_fmt,	\
	}
#endif

/* DCMIPP input format descriptions */
static const struct stm32_dcmipp_input_fmt {
	uint32_t pixelformat;
	uint32_t dcmipp_format;
#if defined(STM32_DCMIPP_HAS_CSI)
	uint32_t dcmipp_csi_bpp;
	uint32_t dcmipp_csi_dt;
#endif
} stm32_dcmipp_input_fmt_desc[] = {
	INPUT_FMT(SBGGR8, RAW8, 8, RAW8), INPUT_FMT(SGBRG8, RAW8, 8, RAW8),
	INPUT_FMT(SGRBG8, RAW8, 8, RAW8), INPUT_FMT(SRGGB8, RAW8, 8, RAW8),
	INPUT_FMT(SBGGR10P, RAW10, 10, RAW10), INPUT_FMT(SGBRG10P, RAW10, 10, RAW10),
	INPUT_FMT(SGRBG10P, RAW10, 10, RAW10), INPUT_FMT(SRGGB10P, RAW10, 10, RAW10),
	INPUT_FMT(SBGGR12P, RAW12, 12, RAW12), INPUT_FMT(SGBRG12P, RAW12, 12, RAW12),
	INPUT_FMT(SGRBG12P, RAW12, 12, RAW12), INPUT_FMT(SRGGB12P, RAW12, 12, RAW12),
	INPUT_FMT(SBGGR14P, RAW14, 14, RAW14), INPUT_FMT(SGBRG14P, RAW14, 14, RAW14),
	INPUT_FMT(SGRBG14P, RAW14, 14, RAW14), INPUT_FMT(SRGGB14P, RAW14, 14, RAW14),
	INPUT_FMT(RGB565, RGB565, 8, RGB565),
	INPUT_FMT(YUYV, YUV422, 8, YUV422_8),
};

static const struct stm32_dcmipp_input_fmt *stm32_dcmipp_get_input_info(uint32_t pixelformat)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(stm32_dcmipp_input_fmt_desc); i++) {
		if (pixelformat == stm32_dcmipp_input_fmt_desc[i].pixelformat) {
			return &stm32_dcmipp_input_fmt_desc[i];
		}
	}

	return NULL;
}

static int stm32_dcmipp_conf_parallel(const struct device *dev,
				      const struct stm32_dcmipp_input_fmt *input_fmt)
{
	struct stm32_dcmipp_data *dcmipp = dev->data;
	const struct stm32_dcmipp_config *config = dev->config;
	DCMIPP_ParallelConfTypeDef parallel_cfg = { 0 };
	int ret;

	parallel_cfg.Format           = input_fmt->dcmipp_format;
	parallel_cfg.SwapCycles       = DCMIPP_SWAPCYCLES_DISABLE;
	parallel_cfg.VSPolarity       = config->parallel.vs_polarity;
	parallel_cfg.HSPolarity       = config->parallel.hs_polarity;
	parallel_cfg.PCKPolarity      = config->parallel.pck_polarity;
	parallel_cfg.ExtendedDataMode = DCMIPP_INTERFACE_8BITS;
	parallel_cfg.SynchroMode      = DCMIPP_SYNCHRO_HARDWARE;

	ret = HAL_DCMIPP_PARALLEL_SetConfig(&dcmipp->hdcmipp, &parallel_cfg);
	if (ret != HAL_OK) {
		LOG_ERR("Failed to configure DCMIPP Parallel interface");
		return -EIO;
	}

	return 0;
}

#if defined(STM32_DCMIPP_HAS_CSI)
#define	DCMIPP_PHY_BITRATE(val)				\
	{						\
		.rate = val,				\
		.PHYBitrate = DCMIPP_CSI_PHY_BT_##val,	\
	}
static const struct {
	uint16_t rate;
	uint16_t PHYBitrate;
} stm32_dcmipp_bitrate[] = {
	DCMIPP_PHY_BITRATE(80), DCMIPP_PHY_BITRATE(90), DCMIPP_PHY_BITRATE(100),
	DCMIPP_PHY_BITRATE(110), DCMIPP_PHY_BITRATE(120), DCMIPP_PHY_BITRATE(130),
	DCMIPP_PHY_BITRATE(140), DCMIPP_PHY_BITRATE(150), DCMIPP_PHY_BITRATE(160),
	DCMIPP_PHY_BITRATE(170), DCMIPP_PHY_BITRATE(180), DCMIPP_PHY_BITRATE(190),
	DCMIPP_PHY_BITRATE(205), DCMIPP_PHY_BITRATE(220), DCMIPP_PHY_BITRATE(235),
	DCMIPP_PHY_BITRATE(250), DCMIPP_PHY_BITRATE(275), DCMIPP_PHY_BITRATE(300),
	DCMIPP_PHY_BITRATE(325), DCMIPP_PHY_BITRATE(350), DCMIPP_PHY_BITRATE(400),
	DCMIPP_PHY_BITRATE(450), DCMIPP_PHY_BITRATE(500), DCMIPP_PHY_BITRATE(550),
	DCMIPP_PHY_BITRATE(600), DCMIPP_PHY_BITRATE(650), DCMIPP_PHY_BITRATE(700),
	DCMIPP_PHY_BITRATE(750), DCMIPP_PHY_BITRATE(800), DCMIPP_PHY_BITRATE(850),
	DCMIPP_PHY_BITRATE(900), DCMIPP_PHY_BITRATE(950), DCMIPP_PHY_BITRATE(1000),
	DCMIPP_PHY_BITRATE(1050), DCMIPP_PHY_BITRATE(1100), DCMIPP_PHY_BITRATE(1150),
	DCMIPP_PHY_BITRATE(1200), DCMIPP_PHY_BITRATE(1250), DCMIPP_PHY_BITRATE(1300),
	DCMIPP_PHY_BITRATE(1350), DCMIPP_PHY_BITRATE(1400), DCMIPP_PHY_BITRATE(1450),
	DCMIPP_PHY_BITRATE(1500), DCMIPP_PHY_BITRATE(1550), DCMIPP_PHY_BITRATE(1600),
	DCMIPP_PHY_BITRATE(1650), DCMIPP_PHY_BITRATE(1700), DCMIPP_PHY_BITRATE(1750),
	DCMIPP_PHY_BITRATE(1800), DCMIPP_PHY_BITRATE(1850), DCMIPP_PHY_BITRATE(1900),
	DCMIPP_PHY_BITRATE(1950), DCMIPP_PHY_BITRATE(2000), DCMIPP_PHY_BITRATE(2050),
	DCMIPP_PHY_BITRATE(2100), DCMIPP_PHY_BITRATE(2150), DCMIPP_PHY_BITRATE(2200),
	DCMIPP_PHY_BITRATE(2250), DCMIPP_PHY_BITRATE(2300), DCMIPP_PHY_BITRATE(2350),
	DCMIPP_PHY_BITRATE(2400), DCMIPP_PHY_BITRATE(2450), DCMIPP_PHY_BITRATE(2500),
};

static int stm32_dcmipp_conf_csi(const struct device *dev, uint32_t dcmipp_csi_bpp)
{
	const struct stm32_dcmipp_config *config = dev->config;
	struct stm32_dcmipp_data *dcmipp = dev->data;
	DCMIPP_CSI_ConfTypeDef csiconf = { 0 };
	int64_t phy_bitrate;
	int err, i;

	csiconf.NumberOfLanes = config->csi.nb_lanes == 2 ? DCMIPP_CSI_TWO_DATA_LANES :
							    DCMIPP_CSI_ONE_DATA_LANE;
	csiconf.DataLaneMapping = config->csi.lanes[0] == 1 ? DCMIPP_CSI_PHYSICAL_DATA_LANES :
							      DCMIPP_CSI_INVERTED_DATA_LANES;

	/* Get link-frequency information from source */
	phy_bitrate = video_get_csi_link_freq(config->source_dev,
					      video_bits_per_pixel(dcmipp->source_fmt.pixelformat),
					      config->csi.nb_lanes);
	if (phy_bitrate < 0) {
		LOG_ERR("Failed to retrieve source link-frequency");
		return -EIO;
	}

	phy_bitrate /= MHZ(1);

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

	/* Set Virtual Channel config */
	/* TODO - need to be able to use an alternate VC, info coming from the source */
	err = HAL_DCMIPP_CSI_SetVCConfig(&dcmipp->hdcmipp, DCMIPP_VIRTUAL_CHANNEL0,
					 dcmipp_csi_bpp);
	if (err != HAL_OK) {
		LOG_ERR("Failed to set CSI configuration");
		return -EIO;
	}

	return 0;
}
#endif

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

#if defined(STM32_DCMIPP_HAS_PIXEL_PIPES)
/* Description of Pixel Pipes formats */
#define PIXEL_PIPE_FMT(fmt, dcmipp, swap, valid_pipes)				\
	{									\
		.pixelformat = VIDEO_PIX_FMT_##fmt,				\
		.pipes = valid_pipes,						\
		.pixels.dcmipp_format = DCMIPP_PIXEL_PACKER_FORMAT_##dcmipp,	\
		.pixels.swap_uv = swap,						\
	}
#endif
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
#if defined(STM32_DCMIPP_HAS_PIXEL_PIPES)
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
	/* Multi-planes are only available on Pipe main (1) */
	PIXEL_PIPE_FMT(NV12, YUV420_2, 0, BIT(1)),
	PIXEL_PIPE_FMT(NV21, YUV420_2, 1, BIT(1)),
	PIXEL_PIPE_FMT(NV16, YUV422_2, 0, BIT(1)),
	PIXEL_PIPE_FMT(NV61, YUV422_2, 1, BIT(1)),
	PIXEL_PIPE_FMT(YUV420, YUV420_3, 0, BIT(1)),
	PIXEL_PIPE_FMT(YVU420, YUV420_3, 1, BIT(1)),
#endif
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
	  (fmt) == VIDEO_PIX_FMT_NV12 || (fmt) == VIDEO_PIX_FMT_NV21 ||		\
	  (fmt) == VIDEO_PIX_FMT_NV16 || (fmt) == VIDEO_PIX_FMT_NV61 ||		\
	  (fmt) == VIDEO_PIX_FMT_YUV420 || (fmt) == VIDEO_PIX_FMT_YVU420 ||	\
	  (fmt) == VIDEO_PIX_FMT_XYUV32) ? VIDEO_COLORSPACE_YUV :		\
										\
	  VIDEO_COLORSPACE_RAW)

static const struct stm32_dcmipp_mapping *stm32_dcmipp_get_mapping(uint32_t pixelformat,
								   uint32_t pipe)
{
	for (int i = 0; i < ARRAY_SIZE(stm32_dcmipp_format_support); i++) {
		if (pixelformat == stm32_dcmipp_format_support[i].pixelformat &&
		    BIT(pipe) & stm32_dcmipp_format_support[i].pipes) {
			return &stm32_dcmipp_format_support[i];
		}
	}

	return NULL;
}

static inline void stm32_dcmipp_compute_fmt_pitch(uint32_t pipe_id, struct video_format *fmt)
{
	fmt->pitch = fmt->width * video_bits_per_pixel(fmt->pixelformat) / BITS_PER_BYTE;
#if defined(STM32_DCMIPP_HAS_PIXEL_PIPES)
	if (pipe_id == DCMIPP_PIPE1 || pipe_id == DCMIPP_PIPE2) {
		/* On Pipe1 and Pipe2, the pitch must be multiple of 16 bytes */
		fmt->pitch = ROUND_UP(fmt->pitch, 16);
	}
#endif
}

static int stm32_dcmipp_set_fmt(const struct device *dev, struct video_format *fmt)
{
	struct stm32_dcmipp_pipe_data *pipe = dev->data;
	struct stm32_dcmipp_data *dcmipp = pipe->dcmipp;
	const struct stm32_dcmipp_mapping *mapping;
	int ret = 0;

	/* Sanitize format given */
	mapping = stm32_dcmipp_get_mapping(fmt->pixelformat, pipe->id);
	if (mapping == NULL) {
		LOG_ERR("Pixelformat %s/0x%x is not supported",
			VIDEO_FOURCC_TO_STR(fmt->pixelformat), fmt->pixelformat);
		return -EINVAL;
	}

	/* In case of DUMP pipe, verify that requested format matched with input format */
	if (pipe->id == DCMIPP_PIPE0 &&
	    mapping->dump.input_pixelformat != dcmipp->source_fmt.pixelformat) {
		LOG_ERR("Dump pipe output format (%s) incompatible with source format (%s)",
			 VIDEO_FOURCC_TO_STR(fmt->pixelformat),
			 VIDEO_FOURCC_TO_STR(dcmipp->source_fmt.pixelformat));
		return -EINVAL;
	}

	if (!IN_RANGE(fmt->width, STM32_DCMIPP_WIDTH_MIN, STM32_DCMIPP_WIDTH_MAX) ||
	    !IN_RANGE(fmt->height, STM32_DCMIPP_HEIGHT_MIN, STM32_DCMIPP_HEIGHT_MAX)) {
		LOG_ERR("Format width/height (%d x %d) does not match caps",
			fmt->width, fmt->height);
		return -EINVAL;
	}

	if (fmt->width != pipe->compose.width || fmt->height != pipe->compose.height) {
		LOG_ERR("Format width/height (%d x %d) do not match compose width/height (%d x %d)",
			fmt->width, fmt->height, pipe->compose.width, pipe->compose.height);
		return -EINVAL;
	}

	stm32_dcmipp_compute_fmt_pitch(pipe->id, fmt);

	k_mutex_lock(&pipe->lock, K_FOREVER);

	if (pipe->is_streaming) {
		ret = -EBUSY;
		goto out;
	}

#if defined(STM32_DCMIPP_HAS_PIXEL_PIPES)
	if (pipe->id == DCMIPP_PIPE1 || pipe->id == DCMIPP_PIPE2) {
		uint32_t post_isp_decimate_width = dcmipp->source_fmt.width /
						   dcmipp->isp_dec_hratio;
		uint32_t post_isp_decimate_height = dcmipp->source_fmt.height /
						    dcmipp->isp_dec_vratio;

		if (!IN_RANGE(fmt->width,
			      post_isp_decimate_width / STM32_DCMIPP_MAX_PIPE_SCALE_FACTOR,
			      post_isp_decimate_width) ||
		    !IN_RANGE(fmt->height,
			      post_isp_decimate_height / STM32_DCMIPP_MAX_PIPE_SCALE_FACTOR,
			      post_isp_decimate_height)) {
			LOG_ERR("Requested resolution cannot be achieved");
			ret = -EINVAL;
			goto out;
		}
	}
#endif

	pipe->fmt = *fmt;

out:
	k_mutex_unlock(&pipe->lock);

	return ret;
}

#if defined(STM32_DCMIPP_HAS_PIXEL_PIPES)
static void stm32_dcmipp_get_isp_decimation(struct stm32_dcmipp_data *dcmipp)
{
	DCMIPP_DecimationConfTypeDef ispdec_cfg;
	uint32_t is_enabled;

	is_enabled = HAL_DCMIPP_PIPE_IsEnabledISPDecimation(&dcmipp->hdcmipp, DCMIPP_PIPE1);
	if (is_enabled == 0) {
		dcmipp->isp_dec_hratio = 1;
		dcmipp->isp_dec_vratio = 1;
	} else {
		HAL_DCMIPP_PIPE_GetISPDecimationConfig(&dcmipp->hdcmipp, DCMIPP_PIPE1, &ispdec_cfg);
		dcmipp->isp_dec_hratio = 1 << (ispdec_cfg.HRatio >> DCMIPP_P1DECR_HDEC_Pos);
		dcmipp->isp_dec_vratio = 1 << (ispdec_cfg.VRatio >> DCMIPP_P1DECR_VDEC_Pos);
	}
}
#endif

static int stm32_dcmipp_get_fmt(const struct device *dev, struct video_format *fmt)
{
	struct stm32_dcmipp_pipe_data *pipe = dev->data;
#if defined(STM32_DCMIPP_HAS_PIXEL_PIPES)
	struct stm32_dcmipp_data *dcmipp = pipe->dcmipp;
	const struct stm32_dcmipp_config *config = dev->config;
	static atomic_t isp_init_once;
	int ret;

	/* Initialize the external ISP handling stack */
	/*
	 * TODO - this is not the right place to do that, however we need to know
	 * the source format before calling the isp_init handler hence can't
	 * do that within the stm32_dcmipp_init function due to unknown
	 * driver initialization order
	 *
	 * Would need an ops that get called when both side of an endpoint get
	 * initiialized
	 */
	if (atomic_cas(&isp_init_once, 0, 1) &&
	    (pipe->id == DCMIPP_PIPE1 || pipe->id == DCMIPP_PIPE2)) {
		/*
		 * It is necessary to perform a dummy configuration here otherwise any
		 * ISP related configuration done by the stm32_dcmipp_isp_init will
		 * fail due to the HAL DCMIPP driver not being in READY state
		 */
		ret = stm32_dcmipp_conf_parallel(dcmipp->dev, &stm32_dcmipp_input_fmt_desc[0]);
		if (ret < 0) {
			LOG_ERR("Failed to perform dummy parallel configuration");
			return ret;
		}

		ret = stm32_dcmipp_isp_init(&dcmipp->hdcmipp, config->source_dev);
		if (ret < 0) {
			LOG_ERR("Failed to initialize the ISP");
			return ret;
		}
		stm32_dcmipp_get_isp_decimation(dcmipp);
	}
#endif

	*fmt = pipe->fmt;

	return 0;
}

static int stm32_dcmipp_set_crop(struct stm32_dcmipp_pipe_data *pipe)
{
	struct stm32_dcmipp_data *dcmipp = pipe->dcmipp;
	DCMIPP_CropConfTypeDef crop_cfg;
	uint32_t frame_width = dcmipp->source_fmt.width;
	uint32_t frame_height = dcmipp->source_fmt.height;
	int ret;

#if defined(STM32_DCMIPP_HAS_PIXEL_PIPES)
	if (pipe->id == DCMIPP_PIPE1 || pipe->id == DCMIPP_PIPE2) {
		frame_width /= dcmipp->isp_dec_hratio;
		frame_height /= dcmipp->isp_dec_vratio;
	}
#endif

	/* If crop area is equal to frame size, disable the crop */
	if (pipe->crop.width == frame_width && pipe->crop.height == frame_height) {
		ret = HAL_DCMIPP_PIPE_DisableCrop(&dcmipp->hdcmipp, pipe->id);
		if (ret != HAL_OK) {
			LOG_ERR("Failed to disable pipe crop");
			return -EIO;
		}

		return 0;
	}

	crop_cfg.VStart = pipe->crop.top;
	crop_cfg.VSize = pipe->crop.height;

	/*
	 * In case of Pipe0, left & width are expressed in word (32bit).
	 * set_selection ensure that value leads to a multiple of 32bit word
	 */
	if (pipe->id == DCMIPP_PIPE0) {
		unsigned int bpp = video_bits_per_pixel(pipe->fmt.pixelformat);

		crop_cfg.HStart = pipe->crop.left * bpp / 32;
		crop_cfg.HSize = pipe->crop.width * bpp / 32;
	}
#if defined(STM32_DCMIPP_HAS_PIXEL_PIPES)
	else {
		crop_cfg.HStart = pipe->crop.left;
		crop_cfg.HSize = pipe->crop.width;
	}
#endif

	ret = HAL_DCMIPP_PIPE_SetCropConfig(&dcmipp->hdcmipp, pipe->id, &crop_cfg);
	if (ret != HAL_OK) {
		LOG_ERR("Failed to configure pipe crop");
		return -EIO;
	}

	ret = HAL_DCMIPP_PIPE_EnableCrop(&dcmipp->hdcmipp, pipe->id);
	if (ret != HAL_OK) {
		LOG_ERR("Failed to enable pipe crop");
		return -EIO;
	}

	return 0;
}

#if defined(STM32_DCMIPP_HAS_PIXEL_PIPES)
#define STM32_DCMIPP_DSIZE_HVRATIO_CONS   8192
#define STM32_DCMIPP_DSIZE_HVRATIO_MAX    65535
#define STM32_DCMIPP_DSIZE_HVDIV_CONS     1024
#define STM32_DCMIPP_DSIZE_HVDIV_MAX      1023
static int stm32_dcmipp_set_downscale(struct stm32_dcmipp_pipe_data *pipe)
{
	struct stm32_dcmipp_data *dcmipp = pipe->dcmipp;
	uint32_t post_decimate_width = pipe->crop.width;
	uint32_t post_decimate_height = pipe->crop.height;
	DCMIPP_DecimationConfTypeDef dec_cfg;
	DCMIPP_DownsizeTypeDef downsize_cfg;
	struct video_rect *compose = &pipe->compose;
	uint32_t hdec = 1, vdec = 1;
	int ret;

	if (compose->width == pipe->crop.width && compose->height == pipe->crop.height) {
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
	while (compose->width * STM32_DCMIPP_MAX_PIPE_DSIZE_FACTOR < post_decimate_width) {
		hdec *= 2;
		post_decimate_width /= 2;
	}
	while (compose->height * STM32_DCMIPP_MAX_PIPE_DSIZE_FACTOR < post_decimate_height) {
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
		dec_cfg.HRatio = __builtin_ctz(hdec) << DCMIPP_P1DECR_HDEC_Pos;
		dec_cfg.VRatio = __builtin_ctz(vdec) << DCMIPP_P1DECR_VDEC_Pos;

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
	downsize_cfg.HRatio =
		post_decimate_width * STM32_DCMIPP_DSIZE_HVRATIO_CONS / compose->width;
	if (downsize_cfg.HRatio > STM32_DCMIPP_DSIZE_HVRATIO_MAX) {
		downsize_cfg.HRatio = STM32_DCMIPP_DSIZE_HVRATIO_MAX;
	}
	downsize_cfg.VRatio =
		post_decimate_height * STM32_DCMIPP_DSIZE_HVRATIO_CONS / compose->height;
	if (downsize_cfg.VRatio > STM32_DCMIPP_DSIZE_HVRATIO_MAX) {
		downsize_cfg.VRatio = STM32_DCMIPP_DSIZE_HVRATIO_MAX;
	}
	downsize_cfg.HDivFactor =
		STM32_DCMIPP_DSIZE_HVDIV_CONS * compose->width / post_decimate_width;
	if (downsize_cfg.HDivFactor > STM32_DCMIPP_DSIZE_HVDIV_MAX) {
		downsize_cfg.HDivFactor = STM32_DCMIPP_DSIZE_HVDIV_MAX;
	}
	downsize_cfg.VDivFactor =
		STM32_DCMIPP_DSIZE_HVDIV_CONS * compose->height / post_decimate_height;
	if (downsize_cfg.VDivFactor > STM32_DCMIPP_DSIZE_HVDIV_MAX) {
		downsize_cfg.VDivFactor = STM32_DCMIPP_DSIZE_HVDIV_MAX;
	}
	downsize_cfg.HSize = compose->width;
	downsize_cfg.VSize = compose->height;

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
					   uint32_t source_colorspace,
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
	if ((source_colorspace == VIDEO_COLORSPACE_RAW ||
	     source_colorspace == VIDEO_COLORSPACE_RGB) &&
	     output_colorspace == VIDEO_COLORSPACE_YUV) {
		/* Need to perform RGB to YUV conversion */
		cfg = &stm32_dcmipp_rgb_to_yuv;
	} else if (source_colorspace == VIDEO_COLORSPACE_YUV &&
		   output_colorspace == VIDEO_COLORSPACE_RGB) {
		/* Need to perform YUV to RGB conversion */
		cfg = &stm32_dcmipp_yuv_to_rgb;
	} else {
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
#endif

static int stm32_dcmipp_start_pipeline(const struct device *dev,
				       struct stm32_dcmipp_pipe_data *pipe)
{
	const struct stm32_dcmipp_config *config = dev->config;
	struct stm32_dcmipp_data *dcmipp = pipe->dcmipp;
#if defined(STM32_DCMIPP_HAS_PIXEL_PIPES)
	struct video_format *fmt = &pipe->fmt;
#endif
	int ret;

#if defined(STM32_DCMIPP_HAS_PIXEL_PIPES)
	if (VIDEO_FMT_IS_PLANAR(fmt)) {
		uint8_t *u_addr = pipe->next->buffer + VIDEO_FMT_PLANAR_Y_PLANE_SIZE(fmt);
		uint8_t *v_addr = u_addr + (VIDEO_FMT_PLANAR_Y_PLANE_SIZE(fmt) / 4);
		DCMIPP_FullPlanarDstAddressTypeDef planar_addr = {
			.YAddress = (uint32_t)pipe->next->buffer,
			.UAddress = (uint32_t)u_addr,
			.VAddress = (uint32_t)v_addr,
		};

		if (config->bus_type == VIDEO_BUS_TYPE_PARALLEL) {
			ret = HAL_DCMIPP_PIPE_FullPlanarStart(&dcmipp->hdcmipp, pipe->id,
							      &planar_addr, DCMIPP_MODE_CONTINUOUS);
		}
#if defined(STM32_DCMIPP_HAS_CSI)
		else if (config->bus_type == VIDEO_BUS_TYPE_CSI2_DPHY) {
			ret = HAL_DCMIPP_CSI_PIPE_FullPlanarStart(&dcmipp->hdcmipp, pipe->id,
								  DCMIPP_VIRTUAL_CHANNEL0,
								  &planar_addr,
								  DCMIPP_MODE_CONTINUOUS);
		}
#endif
		else {
			LOG_ERR("Invalid bus_type");
			ret = -EINVAL;
		}
	} else if (VIDEO_FMT_IS_SEMI_PLANAR(fmt)) {
		uint8_t *uv_addr = pipe->next->buffer + VIDEO_FMT_PLANAR_Y_PLANE_SIZE(fmt);
		DCMIPP_SemiPlanarDstAddressTypeDef semiplanar_addr = {
			.YAddress = (uint32_t)pipe->next->buffer,
			.UVAddress = (uint32_t)uv_addr,
		};

		if (config->bus_type == VIDEO_BUS_TYPE_PARALLEL) {
			ret = HAL_DCMIPP_PIPE_SemiPlanarStart(&dcmipp->hdcmipp, pipe->id,
							      &semiplanar_addr,
							      DCMIPP_MODE_CONTINUOUS);
		}
#if defined(STM32_DCMIPP_HAS_CSI)
		else if (config->bus_type == VIDEO_BUS_TYPE_CSI2_DPHY) {
			ret = HAL_DCMIPP_CSI_PIPE_SemiPlanarStart(&dcmipp->hdcmipp, pipe->id,
								  DCMIPP_VIRTUAL_CHANNEL0,
								  &semiplanar_addr,
								  DCMIPP_MODE_CONTINUOUS);
		}
#endif
		else {
			LOG_ERR("Invalid bus_type");
			ret = -EINVAL;
		}
	} else {
#endif
		if (config->bus_type == VIDEO_BUS_TYPE_PARALLEL) {
			ret = HAL_DCMIPP_PIPE_Start(&dcmipp->hdcmipp, pipe->id,
						    (uint32_t)pipe->next->buffer,
						    DCMIPP_MODE_CONTINUOUS);
		}
#if defined(STM32_DCMIPP_HAS_CSI)
		else if (config->bus_type == VIDEO_BUS_TYPE_CSI2_DPHY) {
			ret = HAL_DCMIPP_CSI_PIPE_Start(&dcmipp->hdcmipp, pipe->id,
							DCMIPP_VIRTUAL_CHANNEL0,
							(uint32_t)pipe->next->buffer,
							DCMIPP_MODE_CONTINUOUS);
		}
#endif
		else {
			LOG_ERR("Invalid bus_type");
			ret = -EINVAL;
		}
#if defined(STM32_DCMIPP_HAS_PIXEL_PIPES)
	}
#endif
	if (ret != HAL_OK) {
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
	const struct stm32_dcmipp_input_fmt *input_fmt;
#if defined(STM32_DCMIPP_HAS_CSI)
	DCMIPP_CSI_PIPE_ConfTypeDef csi_pipe_cfg = { 0 };
#endif
	DCMIPP_PipeConfTypeDef pipe_cfg = { 0 };
	int ret;

	k_mutex_lock(&pipe->lock, K_FOREVER);

	if (pipe->is_streaming) {
		ret = -EALREADY;
		goto out;
	}

	input_fmt = stm32_dcmipp_get_input_info(dcmipp->source_fmt.pixelformat);
	if (input_fmt == NULL) {
		LOG_ERR("Unsupported input format");
		ret = -EINVAL;
		goto out;
	}

	/* Configure the source if nothing is already running */
	if (dcmipp->enabled_pipe == 0) {
		ret = video_set_format(config->source_dev, &dcmipp->source_fmt);
		if (ret < 0) {
			goto out;
		}

		if (config->bus_type == VIDEO_BUS_TYPE_PARALLEL) {
			ret = stm32_dcmipp_conf_parallel(dcmipp->dev, input_fmt);
		}
#if defined(STM32_DCMIPP_HAS_CSI)
		else if (config->bus_type == VIDEO_BUS_TYPE_CSI2_DPHY) {
			ret = stm32_dcmipp_conf_csi(dcmipp->dev, input_fmt->dcmipp_csi_bpp);
		}
#endif
		else {
			LOG_ERR("Invalid bus_type");
			ret = -EINVAL;
			goto out;
		}
		if (ret < 0) {
			LOG_ERR("Failed to configure input stage");
			goto out;
		}
	}

	pipe->active = NULL;
	pipe->next = k_fifo_get(&pipe->fifo_in, K_NO_WAIT);
	if (pipe->next == NULL) {
		LOG_ERR("No buffer available to start streaming");
		ret = -ENOMEM;
		goto out;
	}

#if defined(STM32_DCMIPP_HAS_CSI)
	/* Configure the Pipe input */
	csi_pipe_cfg.DataTypeMode = DCMIPP_DTMODE_DTIDA;
	csi_pipe_cfg.DataTypeIDA  = input_fmt->dcmipp_csi_dt;
	ret = HAL_DCMIPP_CSI_PIPE_SetConfig(&dcmipp->hdcmipp,
					    pipe->id == DCMIPP_PIPE2 ? DCMIPP_PIPE1 : pipe->id,
					    &csi_pipe_cfg);
	if (ret != HAL_OK) {
		LOG_ERR("Failed to configure pipe #%d input", pipe->id);
		ret = -EIO;
		goto out;
	}
#endif

	/* Configure Pipe */
	mapping = stm32_dcmipp_get_mapping(fmt->pixelformat, pipe->id);
	if (mapping == NULL) {
		LOG_ERR("Failed to find pipe format mapping");
		ret = -EIO;
		goto out;
	}

	pipe_cfg.FrameRate  = DCMIPP_FRAME_RATE_ALL;
#if defined(STM32_DCMIPP_HAS_PIXEL_PIPES)
	if (pipe->id == DCMIPP_PIPE1 || pipe->id == DCMIPP_PIPE2) {
		pipe_cfg.PixelPipePitch = VIDEO_Y_PLANE_PITCH(fmt);
		pipe_cfg.PixelPackerFormat = mapping->pixels.dcmipp_format;
	}
#endif
	ret = HAL_DCMIPP_PIPE_SetConfig(&dcmipp->hdcmipp, pipe->id, &pipe_cfg);
	if (ret != HAL_OK) {
		LOG_ERR("Failed to configure pipe #%d", pipe->id);
		ret = -EIO;
		goto out;
	}

	if (pipe->id == DCMIPP_PIPE0) {
		/* Configure the Pipe crop */
		ret = stm32_dcmipp_set_crop(pipe);
		if (ret < 0) {
			goto out;
		}

		/* Only the PIPE0 has a limiter */
		/* Set Limiter to avoid buffer overflow, in number of 32 bits words */
		ret = HAL_DCMIPP_PIPE_EnableLimitEvent(&dcmipp->hdcmipp, DCMIPP_PIPE0,
						       (fmt->pitch * fmt->height) / 4);
		if (ret != HAL_OK) {
			LOG_ERR("Failed to set limiter");
			ret = -EIO;
			goto out;
		}
	}
#if defined(STM32_DCMIPP_HAS_PIXEL_PIPES)
	else if (pipe->id == DCMIPP_PIPE1 || pipe->id == DCMIPP_PIPE2) {
		uint32_t source_colorspace = VIDEO_COLORSPACE(dcmipp->source_fmt.pixelformat);
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

		if (source_colorspace == VIDEO_COLORSPACE_RAW) {
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

		/* Configure the Pipe crop */
		ret = stm32_dcmipp_set_crop(pipe);
		if (ret < 0) {
			goto out;
		}

		/* Configure the Pipe decimation / downsize */
		ret = stm32_dcmipp_set_downscale(pipe);
		if (ret < 0) {
			goto out;
		}

		/* Configure YUV conversion */
		ret = stm32_dcmipp_set_yuv_conversion(pipe, source_colorspace, output_colorspace);
		if (ret < 0) {
			goto out;
		}
	}

	/* Initialize the external ISP handling stack */
	ret = stm32_dcmipp_isp_init(&dcmipp->hdcmipp, config->source_dev);
	if (ret < 0) {
		goto out;
	}
#endif

	/* Enable the DCMIPP Pipeline */
	ret = stm32_dcmipp_start_pipeline(dev, pipe);
	if (ret < 0) {
		LOG_ERR("Failed to start the pipeline");
		goto out;
	}

	/* It is necessary to start the source as well if nothing else is running */
	if (dcmipp->enabled_pipe == 0) {
		ret = video_stream_start(config->source_dev, VIDEO_BUF_TYPE_OUTPUT);
		if (ret < 0) {
			LOG_ERR("Failed to start the source");
			if (config->bus_type == VIDEO_BUS_TYPE_PARALLEL) {
				HAL_DCMIPP_PIPE_Stop(&dcmipp->hdcmipp, pipe->id);
			}
#if defined(STM32_DCMIPP_HAS_CSI)
			else if (config->bus_type == VIDEO_BUS_TYPE_CSI2_DPHY) {
				HAL_DCMIPP_CSI_PIPE_Stop(&dcmipp->hdcmipp, pipe->id,
							 DCMIPP_VIRTUAL_CHANNEL0);
			}
#endif
			else {
				LOG_ERR("Invalid bus_type");
			}
			ret = -EIO;
			goto out;
		}
	}

#if defined(STM32_DCMIPP_HAS_PIXEL_PIPES)
	/* Start the external ISP handling */
	ret = stm32_dcmipp_isp_start();
	if (ret < 0) {
		goto out;
	}
#endif

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

#if defined(STM32_DCMIPP_HAS_PIXEL_PIPES)
	/* Stop the external ISP handling */
	ret = stm32_dcmipp_isp_stop();
	if (ret < 0) {
		goto out;
	}
#endif

	/* Disable the DCMIPP Pipeline */
	if (config->bus_type == VIDEO_BUS_TYPE_PARALLEL) {
		ret = HAL_DCMIPP_PIPE_Stop(&dcmipp->hdcmipp, pipe->id);
	}
#if defined(STM32_DCMIPP_HAS_CSI)
	else if (config->bus_type == VIDEO_BUS_TYPE_CSI2_DPHY) {
		ret = HAL_DCMIPP_CSI_PIPE_Stop(&dcmipp->hdcmipp, pipe->id, DCMIPP_VIRTUAL_CHANNEL0);
	}
#endif
	else {
		LOG_ERR("Invalid bus_type");
		ret = -EIO;
		goto out;
	}
	if (ret != HAL_OK) {
		LOG_ERR("Failed to stop the pipeline");
		ret = -EIO;
		goto out;
	}

	/* Turn off the source nobody else is using the DCMIPP */
	if (dcmipp->enabled_pipe == 1) {
		ret = video_stream_stop(config->source_dev, VIDEO_BUF_TYPE_OUTPUT);
		if (ret < 0) {
			LOG_ERR("Failed to stop the source");
			ret = -EIO;
			goto out;
		}
	}

	/* Release the video buffer allocated when start streaming */
	if (pipe->next != NULL) {
		k_fifo_put(&pipe->fifo_in, pipe->next);
	}
	if (pipe->active != NULL) {
		k_fifo_put(&pipe->fifo_in, pipe->active);
	}

	pipe->state = STM32_DCMIPP_STOPPED;
	pipe->is_streaming = false;
	dcmipp->enabled_pipe--;

out:
	k_mutex_unlock(&pipe->lock);

	return ret;
}

static int stm32_dcmipp_set_stream(const struct device *dev, bool enable, enum video_buf_type type)
{
	return enable ? stm32_dcmipp_stream_enable(dev) : stm32_dcmipp_stream_disable(dev);
}

static int stm32_dcmipp_enqueue(const struct device *dev, struct video_buffer *vbuf)
{
	struct stm32_dcmipp_pipe_data *pipe = dev->data;
	struct stm32_dcmipp_data *dcmipp = pipe->dcmipp;

	k_mutex_lock(&pipe->lock, K_FOREVER);

	if (pipe->fmt.pitch * pipe->fmt.height > vbuf->size) {
		return -EINVAL;
	}

	if (pipe->state == STM32_DCMIPP_WAIT_FOR_BUFFER) {
		LOG_DBG("Restart CPTREQ after wait for buffer");
		pipe->next = vbuf;
		stm32_dcmipp_set_next_buffer_addr(pipe);
		if (pipe->id == DCMIPP_PIPE0) {
			SET_BIT(dcmipp->hdcmipp.Instance->P0FCTCR, DCMIPP_P0FCTCR_CPTREQ);
		}
#if defined(STM32_DCMIPP_HAS_PIXEL_PIPES)
		else if (pipe->id == DCMIPP_PIPE1) {
			SET_BIT(dcmipp->hdcmipp.Instance->P1FCTCR, DCMIPP_P1FCTCR_CPTREQ);
		} else if (pipe->id == DCMIPP_PIPE2) {
			SET_BIT(dcmipp->hdcmipp.Instance->P2FCTCR, DCMIPP_P2FCTCR_CPTREQ);
		}
#endif
		pipe->state = STM32_DCMIPP_RUNNING;
	} else {
		k_fifo_put(&pipe->fifo_in, vbuf);
	}

	k_mutex_unlock(&pipe->lock);

	return 0;
}

static int stm32_dcmipp_dequeue(const struct device *dev, struct video_buffer **vbuf,
				k_timeout_t timeout)
{
	struct stm32_dcmipp_pipe_data *pipe = dev->data;

	*vbuf = k_fifo_get(&pipe->fifo_out, timeout);
	if (*vbuf == NULL) {
		return -EAGAIN;
	}

	return 0;
}

/*
 * TODO: caps aren't yet handled hence give back straight the caps given by the
 * source.  Normally this should be the intersection of what the source produces
 * vs what the DCMIPP can input (for pipe0) and, for pipe 1 and 2, for a given
 * input format, generate caps based on capabilities, color conversion, decimation
 * etc
 */
static int stm32_dcmipp_get_caps(const struct device *dev, struct video_caps *caps)
{
	const struct stm32_dcmipp_config *config = dev->config;
	int ret;

	ret = video_get_caps(config->source_dev, caps);

	caps->min_vbuf_count = 1;
	caps->min_line_count = LINE_COUNT_HEIGHT;
	caps->max_line_count = LINE_COUNT_HEIGHT;

	return ret;
}

static int stm32_dcmipp_get_frmival(const struct device *dev, struct video_frmival *frmival)
{
	const struct stm32_dcmipp_config *config = dev->config;

	return video_get_frmival(config->source_dev, frmival);
}

static int stm32_dcmipp_set_frmival(const struct device *dev, struct video_frmival *frmival)
{
	const struct stm32_dcmipp_config *config = dev->config;

	return video_set_frmival(config->source_dev, frmival);
}

static int stm32_dcmipp_enum_frmival(const struct device *dev, struct video_frmival_enum *fie)
{
	const struct stm32_dcmipp_config *config = dev->config;

	return video_enum_frmival(config->source_dev, fie);
}

static inline int stm32_dcmipp_pipe0_pixel_align(uint32_t pixelformat)
{
	unsigned int bpp = video_bits_per_pixel(pixelformat);

	/*
	 * Pipe0 crop work in number of lines (vertically) but in 32bit words horizontally.
	 * So capabilities of crop depends on the format. As an example, if the format is
	 * 8bpp, then step will be 4 pixels, for 16bpp, step will be 2 pixels,
	 * for 32bpp, step will be 1 pixel, and for 24bpp step will be 4 pixels
	 */

	if (bpp == 8 || bpp == 24) {
		return 4;
	}
	if (bpp == 16) {
		return 2;
	}
	if (bpp == 32) {
		return 1;
	}

	/* Unknown bpp */
	return 0;
}

static int stm32_dcmipp_set_selection(const struct device *dev, struct video_selection *sel)
{
	struct stm32_dcmipp_pipe_data *pipe = dev->data;
	struct stm32_dcmipp_data *dcmipp = pipe->dcmipp;
	uint32_t frame_width = dcmipp->source_fmt.width;
	uint32_t frame_height = dcmipp->source_fmt.height;

	if (sel->type != VIDEO_BUF_TYPE_OUTPUT) {
		return -EINVAL;
	}

#if defined(STM32_DCMIPP_HAS_PIXEL_PIPES)
	if (pipe->id == DCMIPP_PIPE1 || pipe->id == DCMIPP_PIPE2) {
		frame_width /= dcmipp->isp_dec_hratio;
		frame_height /= dcmipp->isp_dec_vratio;
	}
#endif

	switch (sel->target) {
	case VIDEO_SEL_TGT_CROP:
		/* Reset to the whole frame if the requested rectangle isn't part of the frame */
		if (!IN_RANGE(sel->rect.top, 0, frame_height - 1) ||
		    !IN_RANGE(sel->rect.height, 1, frame_height - sel->rect.top) ||
		    !IN_RANGE(sel->rect.left, 0, frame_width - 1) ||
		    !IN_RANGE(sel->rect.width, 1, frame_width - sel->rect.left)) {
			sel->rect.top = 0;
			sel->rect.left = 0;
			sel->rect.width = frame_width;
			sel->rect.height = frame_height;
		}

		/*
		 * Adjust value to horizontal alignment constraints for PIPE0
		 * except if crop area is the full frame
		 */
		if (pipe->id == DCMIPP_PIPE0 &&
		    !(sel->rect.left == 0 || sel->rect.width == frame_width)) {
			int h_pixel_align = stm32_dcmipp_pipe0_pixel_align(pipe->fmt.pixelformat);

			if (h_pixel_align == 0) {
				LOG_ERR("Cannot figure out required pixel alignment");
				return -EIO;
			}

			sel->rect.left = ROUND_DOWN(sel->rect.left, h_pixel_align);
			sel->rect.width = ROUND_DOWN(sel->rect.width, h_pixel_align);
		}

		k_mutex_lock(&pipe->lock, K_FOREVER);
		pipe->crop = sel->rect;
		pipe->compose.width = sel->rect.width;
		pipe->compose.height = sel->rect.height;
		pipe->fmt.width = sel->rect.width;
		pipe->fmt.height = sel->rect.height;
		stm32_dcmipp_compute_fmt_pitch(pipe->id, &pipe->fmt);
		k_mutex_unlock(&pipe->lock);
		break;
	case VIDEO_SEL_TGT_COMPOSE:
		/* Compose not available on Pipe0 */
		if (pipe->id == DCMIPP_PIPE0) {
			sel->rect = pipe->crop;
			goto out;
		}

		if (sel->rect.left != 0) {
			sel->rect.left = 0;
		}
		if (sel->rect.top != 0) {
			sel->rect.top = 0;
		}

		if (!IN_RANGE(sel->rect.width,
			      pipe->crop.width / STM32_DCMIPP_MAX_PIPE_SCALE_FACTOR,
			      pipe->crop.width)) {
			sel->rect.width = pipe->crop.width / STM32_DCMIPP_MAX_PIPE_SCALE_FACTOR;
		}

		if (!IN_RANGE(sel->rect.height,
			      pipe->crop.height / STM32_DCMIPP_MAX_PIPE_SCALE_FACTOR,
			      pipe->crop.height)) {
			sel->rect.height = pipe->crop.height / STM32_DCMIPP_MAX_PIPE_SCALE_FACTOR;
		}

		k_mutex_lock(&pipe->lock, K_FOREVER);
		pipe->compose = sel->rect;
		pipe->fmt.width = sel->rect.width;
		pipe->fmt.height = sel->rect.height;
		stm32_dcmipp_compute_fmt_pitch(pipe->id, &pipe->fmt);
		k_mutex_unlock(&pipe->lock);
		break;
	default:
		return -EINVAL;
	};

out:
	return 0;
}

static int stm32_dcmipp_get_selection(const struct device *dev, struct video_selection *sel)
{
	struct stm32_dcmipp_pipe_data *pipe = dev->data;
	struct stm32_dcmipp_data *dcmipp = pipe->dcmipp;

	if (sel->type != VIDEO_BUF_TYPE_OUTPUT) {
		return -EINVAL;
	}

	switch (sel->target) {
	case VIDEO_SEL_TGT_CROP:
		sel->rect = pipe->crop;
		break;
	case VIDEO_SEL_TGT_COMPOSE:
		sel->rect = pipe->compose;
		break;
	case VIDEO_SEL_TGT_CROP_BOUND:
	case VIDEO_SEL_TGT_COMPOSE_BOUND:
		sel->rect.top = 0;
		sel->rect.left = 0;
		if (pipe->id == DCMIPP_PIPE0) {
			sel->rect.width = dcmipp->source_fmt.width;
			sel->rect.height = dcmipp->source_fmt.height;
		}
#if defined(STM32_DCMIPP_HAS_PIXEL_PIPES)
		else if (pipe->id == DCMIPP_PIPE1 || pipe->id == DCMIPP_PIPE2) {
			sel->rect.width = dcmipp->source_fmt.width / dcmipp->isp_dec_hratio;
			sel->rect.height = dcmipp->source_fmt.height / dcmipp->isp_dec_vratio;
		}
#endif
		break;
	case VIDEO_SEL_TGT_NATIVE_SIZE:
		sel->rect.top = 0;
		sel->rect.left = 0;
		sel->rect.width = dcmipp->source_fmt.width;
		sel->rect.height = dcmipp->source_fmt.height;
		break;
	default:
		return -EINVAL;
	};

	return 0;
}

static DEVICE_API(video, stm32_dcmipp_driver_api) = {
	.set_format = stm32_dcmipp_set_fmt,
	.get_format = stm32_dcmipp_get_fmt,
	.set_stream = stm32_dcmipp_set_stream,
	.enqueue = stm32_dcmipp_enqueue,
	.dequeue = stm32_dcmipp_dequeue,
	.get_caps = stm32_dcmipp_get_caps,
	.get_frmival = stm32_dcmipp_get_frmival,
	.set_frmival = stm32_dcmipp_set_frmival,
	.enum_frmival = stm32_dcmipp_enum_frmival,
	.set_selection = stm32_dcmipp_set_selection,
	.get_selection = stm32_dcmipp_get_selection,
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
	err = clock_control_configure(cc_node, (clock_control_subsys_t)&config->dcmipp_pclken_ker,
				      NULL);
	if (err < 0) {
		LOG_ERR("Failed to configure DCMIPP clock. Error %d", err);
		return err;
	}

	err = clock_control_on(cc_node, (clock_control_subsys_t *)&config->dcmipp_pclken);
	if (err < 0) {
		LOG_ERR("Failed to enable DCMIPP clock. Error %d", err);
		return err;
	}

#if defined(STM32_DCMIPP_HAS_CSI)
	/* Turn on CSI peripheral clock */
	err = clock_control_on(cc_node, (clock_control_subsys_t *)&config->csi_pclken);
	if (err < 0) {
		LOG_ERR("Failed to enable CSI clock. Error %d", err);
		return err;
	}
#endif

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

#if defined(STM32_DCMIPP_HAS_PIXEL_PIPES)
	dcmipp->isp_dec_hratio = 1;
	dcmipp->isp_dec_vratio = 1;
#endif

	/* Configure DT provided pins */
	if (cfg->bus_type == VIDEO_BUS_TYPE_PARALLEL) {
		err = pinctrl_apply_state(cfg->pctrl, PINCTRL_STATE_DEFAULT);
		if (err < 0) {
			LOG_ERR("pinctrl setup failed. Error %d.", err);
			return err;
		}
	}

	/* Enable DCMIPP / CSI clocks */
	err = stm32_dcmipp_enable_clock(dev);
	if (err < 0) {
		LOG_ERR("Clock enabling failed.");
		return err;
	}

	/* Reset DCMIPP & CSI */
	if (!device_is_ready(cfg->reset_dcmipp.dev)) {
		LOG_ERR("reset controller not ready");
		return -ENODEV;
	}
	reset_line_toggle_dt(&cfg->reset_dcmipp);
#if defined(STM32_DCMIPP_HAS_CSI)
	reset_line_toggle_dt(&cfg->reset_csi);
#endif

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

	/* Initialize format/crop/compose */
	pipe->fmt.type = VIDEO_BUF_TYPE_OUTPUT;
	pipe->fmt.width = dcmipp->source_fmt.width;
	pipe->fmt.height = dcmipp->source_fmt.height;
	pipe->fmt.pixelformat = dcmipp->source_fmt.pixelformat;
	pipe->crop.top = 0;
	pipe->crop.left = 0;
	pipe->crop.width = pipe->fmt.width;
	pipe->crop.height = pipe->fmt.height;
	pipe->compose = pipe->crop;

	/* Store the pipe data pointer into dcmipp data structure */
	dcmipp->pipe[pipe->id] = pipe;

	return 0;
}

static void stm32_dcmipp_isr(const struct device *dev)
{
	struct stm32_dcmipp_data *dcmipp = dev->data;

	HAL_DCMIPP_IRQHandler(&dcmipp->hdcmipp);
}

#define SOURCE_DEV(inst) DEVICE_DT_GET(DT_NODE_REMOTE_DEVICE(DT_INST_ENDPOINT_BY_ID(inst, 0, 0)))

#define DCMIPP_PIPE_INIT_DEFINE(node_id, inst)						\
	static struct stm32_dcmipp_pipe_data stm32_dcmipp_pipe_##node_id = {		\
		.id = DT_NODE_CHILD_IDX(node_id),					\
		.dcmipp = &stm32_dcmipp_data_##inst,					\
	};										\
											\
	DEVICE_DT_DEFINE(node_id, &stm32_dcmipp_pipe_init, NULL,			\
			 &stm32_dcmipp_pipe_##node_id,					\
			 &stm32_dcmipp_config_##inst,					\
			 POST_KERNEL, CONFIG_VIDEO_INIT_PRIORITY,			\
			 &stm32_dcmipp_driver_api);					\
											\
	VIDEO_DEVICE_DEFINE(dcmipp_##inst_pipe_##node_id, DEVICE_DT_GET(node_id), SOURCE_DEV(inst));

#if defined(STM32_DCMIPP_HAS_CSI)
#define STM32_DCMIPP_CSI_DT_PARAMS(inst)							\
		.csi_pclken =									\
			{.bus = DT_CLOCKS_CELL_BY_NAME(DT_DRV_INST(inst), csi, bus),		\
			 .enr = DT_CLOCKS_CELL_BY_NAME(DT_DRV_INST(inst), csi, bits)},		\
		.reset_csi = RESET_DT_SPEC_INST_GET_BY_IDX(inst, 1),				\
		.csi.nb_lanes = DT_PROP_LEN(DT_INST_ENDPOINT_BY_ID(inst, 0, 0), data_lanes),	\
		.csi.lanes[0] = DT_PROP_BY_IDX(DT_INST_ENDPOINT_BY_ID(inst, 0, 0),		\
					       data_lanes, 0),					\
		.csi.lanes[1] = COND_CODE_1(DT_PROP_LEN(DT_INST_ENDPOINT_BY_ID(inst, 0, 0),	\
							data_lanes),				\
					    (0),						\
					    (DT_PROP_BY_IDX(DT_INST_ENDPOINT_BY_ID(inst, 0, 0),	\
							    data_lanes, 1))),
#else
#define STM32_DCMIPP_CSI_DT_PARAMS(inst)
#endif

#if defined(STM32_DCMIPP_HAS_PIXEL_PIPES)
#define STM32_DCMIPP_PIPES(inst) \
	DT_FOREACH_CHILD_VARGS(DT_INST_CHILD(inst, pipes), DCMIPP_PIPE_INIT_DEFINE, inst)
#else
#define STM32_DCMIPP_PIPE_INIT(node_id, inst) DCMIPP_PIPE_INIT_DEFINE(node_id, inst)
#define STM32_DCMIPP_PIPES(inst) STM32_DCMIPP_PIPE_INIT(DT_INST_CHILD(inst, pipe), inst)
#endif

#define STM32_DCMIPP_INIT(inst)									\
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
		.source_fmt = {									\
			.pixelformat =								\
				VIDEO_FOURCC_FROM_STR(						\
						CONFIG_VIDEO_STM32_DCMIPP_SENSOR_PIXEL_FORMAT),	\
			.width = CONFIG_VIDEO_STM32_DCMIPP_SENSOR_WIDTH,			\
			.height = CONFIG_VIDEO_STM32_DCMIPP_SENSOR_HEIGHT,			\
		},										\
	};											\
												\
	PINCTRL_DT_INST_DEFINE(inst);								\
												\
	static const struct stm32_dcmipp_config stm32_dcmipp_config_##inst = {			\
		.dcmipp_pclken =								\
			{.bus = DT_CLOCKS_CELL_BY_NAME(DT_DRV_INST(inst), dcmipp, bus),		\
			 .enr = DT_CLOCKS_CELL_BY_NAME(DT_DRV_INST(inst), dcmipp, bits)},	\
		.dcmipp_pclken_ker =								\
			{.bus = DT_CLOCKS_CELL_BY_NAME(DT_DRV_INST(inst), dcmipp_ker, bus),	\
			 .enr = DT_CLOCKS_CELL_BY_NAME(DT_DRV_INST(inst), dcmipp_ker, bits)},	\
		.irq_config = stm32_dcmipp_irq_config_##inst,					\
		.pctrl = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),					\
		.source_dev = SOURCE_DEV(inst),							\
		.reset_dcmipp = RESET_DT_SPEC_INST_GET_BY_IDX(inst, 0),				\
		.bus_type = DT_PROP_OR(DT_INST_ENDPOINT_BY_ID(inst, 0, 0), bus_type,		\
				       VIDEO_BUS_TYPE_PARALLEL),				\
		STM32_DCMIPP_CSI_DT_PARAMS(inst)						\
		.parallel.vs_polarity = DT_PROP_OR(DT_INST_ENDPOINT_BY_ID(inst, 0, 0),		\
						    vsync_active, 0) ?				\
						    DCMIPP_VSPOLARITY_HIGH :			\
						    DCMIPP_VSPOLARITY_LOW,			\
		.parallel.hs_polarity = DT_PROP_OR(DT_INST_ENDPOINT_BY_ID(inst, 0, 0),	        \
						   hsync_active, 0) ?				\
						    DCMIPP_HSPOLARITY_HIGH :			\
						    DCMIPP_HSPOLARITY_LOW,			\
		.parallel.pck_polarity = DT_PROP_OR(DT_INST_ENDPOINT_BY_ID(inst, 0, 0),		\
						    pclk_sample, 0) ?				\
						    DCMIPP_PCKPOLARITY_RISING :			\
						    DCMIPP_PCKPOLARITY_FALLING,			\
	};											\
												\
	DEVICE_DT_INST_DEFINE(inst, &stm32_dcmipp_init,						\
		    NULL, &stm32_dcmipp_data_##inst,						\
		    &stm32_dcmipp_config_##inst,						\
		    POST_KERNEL, CONFIG_VIDEO_INIT_PRIORITY,					\
		    NULL);									\
												\
	STM32_DCMIPP_PIPES(inst)

DT_INST_FOREACH_STATUS_OKAY(STM32_DCMIPP_INIT)
