/*
 * Copyright (c) 2025 Renesas Electronics Co.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_ra_ceu

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/video.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
#include <soc.h>
#include <errno.h>

#include "video_device.h"
#include "r_ceu.h"

LOG_MODULE_REGISTER(renesas_ra_video_ceu, CONFIG_VIDEO_LOG_LEVEL);

/*
 * Hardware alignment constraints:
 * - Width must be a multiple of 8 pixels.
 * - Height must be a multiple of 4 lines.
 */
#define CEU_WIDTH_ALIGN  8U
#define CEU_HEIGHT_ALIGN 4U

/*
 * Default capture configuration:
 * - Resolution: 128x96
 * - Bytes per pixel: 2
 */
#define CEU_DEFAULT_WIDTH           128U
#define CEU_DEFAULT_HEIGHT          96U
#define CEU_DEFAULT_START_X         0U
#define CEU_DEFAULT_START_Y         0U
#define CEU_DEFAULT_BYTES_PER_PIXEL 2U

struct video_renesas_ra_ceu_config {
	void (*irq_config_func)(const struct device *dev);
	const struct device *clock_dev;
	const struct device *cam_xclk_dev;
	const struct device *source_dev;
	const struct pinctrl_dev_config *pincfg;
	const struct clock_control_ra_subsys_cfg clock_subsys;
};

struct video_renesas_ra_ceu_data {
	struct video_renesas_ra_ceu_config *config;
	struct st_ceu_instance_ctrl *fsp_ctrl;
	struct st_capture_cfg *fsp_cfg;
	struct st_ceu_extended_cfg *fsp_extend_cfg;
	struct video_buffer *vbuf;
	struct video_format fmt;
	struct k_fifo fifo_in;
	struct k_fifo fifo_out;
	bool is_streaming;
#ifdef CONFIG_POLL
	struct k_poll_signal *signal_out;
#endif
};

extern void ceu_isr(void);

static void video_renesas_ra_ceu_callback(capture_callback_args_t *p_args)
{
	const struct device *dev = p_args->p_context;
	struct video_renesas_ra_ceu_data *data = dev->data;
	fsp_err_t err;

	if (p_args->event & CEU_EVENT_FRAME_END) {
		if (data->vbuf) {
			data->vbuf->timestamp = k_uptime_get_32();
			k_fifo_put(&data->fifo_out, data->vbuf);
			data->vbuf = NULL;
		}

		data->vbuf = k_fifo_get(&data->fifo_in, K_NO_WAIT);
		if (data->vbuf == NULL) {
			return;
		}

		err = R_CEU_CaptureStart(&data->fsp_ctrl, data->vbuf->buffer);
		if (err != FSP_SUCCESS) {
			return;
		}
	}
}

static int video_renesas_ra_ceu_get_format(const struct device *dev, struct video_format *fmt)
{
	const struct video_renesas_ra_ceu_config *config = dev->config;
	int ret;

	ret = video_get_format(config->source_dev, fmt);
	if (ret < 0) {
		LOG_DBG("Failed to get video format from source device");
		return ret;
	}

	fmt->pitch = fmt->width * video_bits_per_pixel(fmt->pixelformat) / BITS_PER_BYTE;

	return 0;
}

static int video_renesas_ra_ceu_set_format(const struct device *dev, struct video_format *fmt)
{
	struct video_renesas_ra_ceu_data *data = dev->data;
	fsp_err_t err;
	int ret;

	fmt->pitch = fmt->width * video_bits_per_pixel(fmt->pixelformat) / BITS_PER_BYTE;

	ret = video_set_format(data->config->source_dev, fmt);
	if (ret < 0) {
		LOG_DBG("Failed to set format on source device");
		return ret;
	}

	if (fmt->width % CEU_WIDTH_ALIGN) {
		LOG_DBG("Width %d not a multiple of %d", fmt->width, CEU_WIDTH_ALIGN);
		return -ENOTSUP;
	}
	if (fmt->height % CEU_HEIGHT_ALIGN) {
		LOG_DBG("Height %d not a multiple of %d", fmt->height, CEU_HEIGHT_ALIGN);
		return -ENOTSUP;
	}

	if (data->fsp_ctrl->open) {
		R_CEU_Close(data->fsp_ctrl);
	}

	data->fsp_cfg->x_capture_pixels = fmt->width;
	data->fsp_cfg->y_capture_pixels = fmt->height;
	data->fsp_cfg->bytes_per_pixel = video_bits_per_pixel(fmt->pixelformat) / BITS_PER_BYTE;

	err = R_CEU_Open(data->fsp_ctrl, data->fsp_cfg);
	if (err != FSP_SUCCESS) {
		LOG_DBG("Failed to open CEU");
		return -EIO;
	}

	memcpy(&data->fmt, fmt, sizeof(struct video_format));

	return 0;
}

static int video_renesas_ra_ceu_get_caps(const struct device *dev, struct video_caps *caps)
{
	const struct video_renesas_ra_ceu_config *config = dev->config;

	caps->min_line_count = caps->max_line_count = LINE_COUNT_HEIGHT;

	return video_get_caps(config->source_dev, caps);
}

static int video_renesas_ra_ceu_set_stream(const struct device *dev, bool enable,
					   enum video_buf_type type)
{
	const struct video_renesas_ra_ceu_config *config = dev->config;
	struct video_renesas_ra_ceu_data *data = dev->data;
	fsp_err_t err;

	if (!enable) {
		if (video_stream_stop(config->source_dev, type)) {
			LOG_DBG("Failed to stop source device stream");
			return -EIO;
		}

		data->is_streaming = false;
		R_CEU->CAPSR = R_CEU_CAPSR_CPKIL_Msk;
	}

	if (data->is_streaming) {
		return -EBUSY;
	}

	data->vbuf = k_fifo_get(&data->fifo_in, K_NO_WAIT);
	if (data->vbuf == NULL) {
		LOG_DBG("No enqueued video buffers available to start streaming");
		return -EAGAIN;
	}

	err = R_CEU_CaptureStart(&data->fsp_ctrl, data->vbuf->buffer);
	if (err != FSP_SUCCESS) {
		LOG_DBG("Failed to start CEU capture");
		return -EIO;
	}

	if (video_stream_start(config->source_dev, type)) {
		LOG_DBG("Failed to start source device stream");
		return -EIO;
	}

	data->is_streaming = true;

	return 0;
}

static int video_renesas_ra_ceu_enqueue(const struct device *dev, struct video_buffer *vbuf)
{
	struct video_renesas_ra_ceu_data *data = dev->data;
	const uint32_t buffer_size = data->fmt.pitch * data->fmt.height;

	if (buffer_size > vbuf->size) {
		LOG_DBG("Enqueue buffer too small");
		return -EINVAL;
	}

	vbuf->bytesused = buffer_size;
	vbuf->line_offset = 0;

	k_fifo_put(&data->fifo_in, vbuf);

	return 0;
}

static int video_renesas_ra_ceu_dequeue(const struct device *dev, struct video_buffer **buf,
					k_timeout_t timeout)
{
	struct video_renesas_ra_ceu_data *data = dev->data;

	*buf = k_fifo_get(&data->fifo_out, timeout);
	if (*buf == NULL) {
		LOG_DBG("Dequeue timeout or no completed buffer available");
		return -EAGAIN;
	}

	return 0;
}

static int video_renesas_ra_ceu_get_frmival(const struct device *dev, struct video_frmival *frmival)
{
	const struct video_renesas_ra_ceu_config *config = dev->config;

	return video_get_frmival(config->source_dev, frmival);
}

static int video_renesas_ra_ceu_set_frmival(const struct device *dev, struct video_frmival *frmival)
{
	const struct video_renesas_ra_ceu_config *config = dev->config;

	return video_set_frmival(config->source_dev, frmival);
}

static int video_renesas_ra_ceu_enum_frmival(const struct device *dev,
					     struct video_frmival_enum *fie)
{
	const struct video_renesas_ra_ceu_config *config = dev->config;

	return video_enum_frmival(config->source_dev, fie);
}

static int video_renesas_ra_ceu_flush(const struct device *dev, bool cancel)
{
	struct video_renesas_ra_ceu_data *data = dev->data;
	struct video_buffer *vbuf = NULL;
	enum video_buf_type type = VIDEO_BUF_TYPE_OUTPUT;

	if (cancel) {
		if (data->is_streaming) {
			video_renesas_ra_ceu_set_stream(dev, false, type);
		}
		if (data->vbuf) {
			k_fifo_put(&data->fifo_out, data->vbuf);
			data->vbuf = NULL;
		}
		while ((vbuf = k_fifo_get(&data->fifo_in, K_NO_WAIT)) != NULL) {
			k_fifo_put(&data->fifo_out, vbuf);
#ifdef CONFIG_POLL
			if (data->signal_out) {
				k_poll_signal_raise(data->signal_out, VIDEO_BUF_ABORTED);
			}
#endif
		}
	} else {
		while (!k_fifo_is_empty(&data->fifo_in)) {
			k_sleep(K_MSEC(1));
		}
	}

	return 0;
}

#ifdef CONFIG_POLL
int video_renesas_ra_ceu_set_signal(const struct device *dev, struct k_poll_signal *sig)
{
	struct video_renesas_ra_ceu_data *data = dev->data;

	data->signal_out = sig;
	return 0;
}
#endif

static int video_renesas_ra_ceu_init(const struct device *dev)
{
	struct video_renesas_ra_ceu_data *data = dev->data;
	fsp_err_t err;
	int ret;

	ret = pinctrl_apply_state(data->config->pincfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_DBG("Failed to configure pinctrl");
		return ret;
	}

	if (!device_is_ready(data->config->clock_dev)) {
		LOG_DBG("Clock control device not ready");
		return -ENODEV;
	}

	ret = clock_control_on(data->config->clock_dev,
			       (clock_control_subsys_t)&data->config->clock_subsys);
	if (ret < 0) {
		LOG_DBG("Failed to enable clock control");
		return ret;
	}

	data->config->irq_config_func(dev);
	k_fifo_init(&data->fifo_in);
	k_fifo_init(&data->fifo_out);

	err = R_CEU_Open(data->fsp_ctrl, data->fsp_cfg);
	if (err != FSP_SUCCESS) {
		LOG_DBG("Failed to open CEU hardware");
		return -EIO;
	}

	return 0;
}

static DEVICE_API(video, video_renesas_ra_ceu_driver_api) = {
	.get_format = video_renesas_ra_ceu_get_format,
	.set_format = video_renesas_ra_ceu_set_format,
	.get_caps = video_renesas_ra_ceu_get_caps,
	.set_stream = video_renesas_ra_ceu_set_stream,
	.enqueue = video_renesas_ra_ceu_enqueue,
	.dequeue = video_renesas_ra_ceu_dequeue,
	.enum_frmival = video_renesas_ra_ceu_enum_frmival,
	.set_frmival = video_renesas_ra_ceu_set_frmival,
	.get_frmival = video_renesas_ra_ceu_get_frmival,
	.flush = video_renesas_ra_ceu_flush,
#ifdef CONFIG_POLL
	.set_signal = video_renesas_ra_ceu_set_signal,
#endif
};

#define EVENT_CEU_INT(inst) BSP_PRV_IELS_ENUM(CONCAT(EVENT_CEU, _CEUI))

#define EP_INST_NODE(inst) DT_INST_ENDPOINT_BY_ID(inst, 0, 0)

#define EP_INST_PROP_SEL(inst, prop, match_val, val_if_true, val_if_false)                         \
	(DT_PROP(EP_INST_NODE(inst), prop) == (match_val) ? (val_if_true) : (val_if_false))

#define SOURCE_DEV(inst) DEVICE_DT_GET(DT_NODE_REMOTE_DEVICE(EP_INST_NODE(inst)))

#define VIDEO_RENESAS_RA_CEU_INIT(inst)                                                            \
	static void video_renesas_ra_ceu_irq_config_func##inst(const struct device *dev)           \
	{                                                                                          \
		R_ICU->IELSR[DT_INST_IRQ_BY_NAME(inst, ceui, irq)] = EVENT_CEU_INT(inst);          \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(inst, ceui, irq),                                  \
			    DT_INST_IRQ_BY_NAME(inst, ceui, priority), ceu_isr, NULL, 0);          \
		irq_enable(DT_INST_IRQ_BY_NAME(inst, ceui, irq));                                  \
	}                                                                                          \
                                                                                                   \
	static int video_renesas_ra_ceu_cam_clock_init##inst(void)                                 \
	{                                                                                          \
		const struct device *dev = DEVICE_DT_INST_GET(inst);                               \
		const struct video_renesas_ra_ceu_config *config = dev->config;                    \
		int ret;                                                                           \
                                                                                                   \
		if (!device_is_ready(config->cam_xclk_dev)) {                                      \
			LOG_DBG("Camera clock control device not ready");                          \
			return -ENODEV;                                                            \
		}                                                                                  \
                                                                                                   \
		ret = clock_control_on(config->cam_xclk_dev, (clock_control_subsys_t)0);           \
		if (ret < 0) {                                                                     \
			LOG_DBG("Failed to enable camera clock control");                          \
			return ret;                                                                \
		}                                                                                  \
		return 0;                                                                          \
	}                                                                                          \
                                                                                                   \
	PINCTRL_DT_INST_DEFINE(inst);                                                              \
                                                                                                   \
	static struct video_renesas_ra_ceu_config video_renesas_ra_ceu_config##inst = {            \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR_BY_NAME(inst, pclk)),               \
		.cam_xclk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR_BY_NAME(inst, cam_xclk)),        \
		.source_dev = SOURCE_DEV(inst),                                                    \
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                                    \
		.clock_subsys =                                                                    \
			{                                                                          \
				.mstp = DT_INST_CLOCKS_CELL_BY_NAME(inst, pclk, mstp),             \
				.stop_bit = DT_INST_CLOCKS_CELL_BY_NAME(inst, pclk, stop_bit),     \
			},                                                                         \
		.irq_config_func = video_renesas_ra_ceu_irq_config_func##inst,                     \
	};                                                                                         \
                                                                                                   \
	static struct st_ceu_instance_ctrl video_renesas_ra_ceu_fsp_ctrl##inst = {                 \
		.p_context = DEVICE_DT_INST_GET(inst),                                             \
		.p_callback_memory = NULL,                                                         \
	};                                                                                         \
                                                                                                   \
	static struct st_ceu_extended_cfg video_renesas_ra_ceu_fsp_extend_cfg##inst = {            \
		.capture_format = CEU_CAPTURE_FORMAT_DATA_SYNCHRONOUS,                             \
		.data_bus_width = EP_INST_PROP_SEL(inst, bus_width, 8, 0, 1),                      \
		.edge_info =                                                                       \
			{                                                                          \
				.dsel = EP_INST_PROP_SEL(inst, pclk_sample, 1, 0, 1),              \
				.hdsel = EP_INST_PROP_SEL(inst, hsync_sample, 1, 0, 1),            \
				.vdsel = EP_INST_PROP_SEL(inst, vsync_sample, 1, 0, 1),            \
			},                                                                         \
		.hsync_polarity = EP_INST_PROP_SEL(inst, hsync_active, 1, 0, 1),                   \
		.vsync_polarity = EP_INST_PROP_SEL(inst, vsync_active, 1, 0, 1),                   \
		.byte_swapping =                                                                   \
			{                                                                          \
				.swap_8bit_units = DT_INST_PROP(inst, swap_8bits),                 \
				.swap_16bit_units = DT_INST_PROP(inst, swap_16bits),               \
				.swap_32bit_units = DT_INST_PROP(inst, swap_32bits),               \
			},                                                                         \
		.burst_mode = DT_INST_ENUM_IDX(inst, burst_transfer),                              \
		.ceu_ipl = DT_INST_IRQ_BY_NAME(inst, ceui, priority),                              \
		.ceu_irq = DT_INST_IRQ_BY_NAME(inst, ceui, irq),                                   \
		.interrupts_enabled = R_CEU_CEIER_CPEIE_Msk | R_CEU_CEIER_VDIE_Msk |               \
				      R_CEU_CEIER_CDTOFIE_Msk | R_CEU_CEIER_VBPIE_Msk |            \
				      R_CEU_CEIER_NHDIE_Msk | R_CEU_CEIER_NVDIE_Msk,               \
	};                                                                                         \
                                                                                                   \
	static struct st_capture_cfg video_renesas_ra_ceu_fsp_cfg##inst = {                        \
		.x_capture_pixels = CEU_DEFAULT_WIDTH,                                             \
		.y_capture_pixels = CEU_DEFAULT_HEIGHT,                                            \
		.x_capture_start_pixel = CEU_DEFAULT_START_X,                                      \
		.y_capture_start_pixel = CEU_DEFAULT_START_Y,                                      \
		.bytes_per_pixel = CEU_DEFAULT_BYTES_PER_PIXEL,                                    \
		.p_extend = &video_renesas_ra_ceu_fsp_extend_cfg##inst,                            \
		.p_callback = video_renesas_ra_ceu_callback,                                       \
		.p_context = DEVICE_DT_INST_GET(inst),                                             \
	};                                                                                         \
                                                                                                   \
	static struct video_renesas_ra_ceu_data video_renesas_ra_ceu_data##inst = {                \
		.config = &video_renesas_ra_ceu_config##inst,                                      \
		.fsp_ctrl = &video_renesas_ra_ceu_fsp_ctrl##inst,                                  \
		.fsp_cfg = &video_renesas_ra_ceu_fsp_cfg##inst,                                    \
		.fsp_extend_cfg = &video_renesas_ra_ceu_fsp_extend_cfg##inst,                      \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, &video_renesas_ra_ceu_init, NULL,                              \
			      &video_renesas_ra_ceu_data##inst,                                    \
			      &video_renesas_ra_ceu_config##inst, POST_KERNEL,                     \
			      CONFIG_VIDEO_INIT_PRIORITY, &video_renesas_ra_ceu_driver_api);       \
                                                                                                   \
	VIDEO_DEVICE_DEFINE(renesas_ra_ceu##inst, DEVICE_DT_INST_GET(inst), SOURCE_DEV(inst));     \
                                                                                                   \
	SYS_INIT(video_renesas_ra_ceu_cam_clock_init##inst, POST_KERNEL,                           \
		 CONFIG_CLOCK_CONTROL_PWM_INIT_PRIORITY);

DT_INST_FOREACH_STATUS_OKAY(VIDEO_RENESAS_RA_CEU_INIT)
