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
 * - Width: 128–2560 pixels, must be a multiple of 8
 * - Height: 96–1920 lines, must be a multiple of 4
 */
#define VIDEO_RA_CEU_WIDTH_MIN    128U
#define VIDEO_RA_CEU_WIDTH_MAX    2560U
#define VIDEO_RA_CEU_WIDTH_ALIGN  8U
#define VIDEO_RA_CEU_HEIGHT_MIN   96U
#define VIDEO_RA_CEU_HEIGHT_MAX   1920U
#define VIDEO_RA_CEU_HEIGHT_ALIGN 4U

/*
 * Default capture configuration:
 * - Resolution: 128x96
 * - Bytes per pixel: 2
 */
#define VIDEO_RA_CEU_DEFAULT_WIDTH           VIDEO_RA_CEU_WIDTH_MIN
#define VIDEO_RA_CEU_DEFAULT_HEIGHT          VIDEO_RA_CEU_HEIGHT_MIN
#define VIDEO_RA_CEU_DEFAULT_START_X         0U
#define VIDEO_RA_CEU_DEFAULT_START_Y         0U
#define VIDEO_RA_CEU_DEFAULT_BYTES_PER_PIXEL 2U

struct video_renesas_ra_ceu_config {
	void (*irq_config_func)(const struct device *dev);
	const struct device *clock_dev;
	const struct device *cam_xclk_dev;
	const struct device *source_dev;
	const struct pinctrl_dev_config *pincfg;
	const struct clock_control_ra_subsys_cfg clock_subsys;
};

struct video_renesas_ra_ceu_data {
	struct st_ceu_instance_ctrl *fsp_ctrl;
	struct st_capture_cfg *fsp_cfg;
	struct st_ceu_extended_cfg *fsp_extend_cfg;
	struct video_format fmt;
	struct k_fifo fifo_in;
	struct k_fifo fifo_out;
	atomic_ptr_t vbuf;
	atomic_t streaming;
#ifdef CONFIG_POLL
	struct k_poll_signal *signal;
#endif
};

extern void ceu_isr(void);

static void video_renesas_ra_ceu_callback(capture_callback_args_t *p_args)
{
	const struct device *dev = p_args->p_context;
	struct video_renesas_ra_ceu_data *data = dev->data;
	struct video_buffer *curr_vbuf;
	struct video_buffer *next_vbuf;
	fsp_err_t err;

	if (p_args->event & CEU_EVENT_FRAME_END) {
		curr_vbuf = atomic_ptr_get(&data->vbuf);
		if (curr_vbuf && atomic_ptr_cas(&data->vbuf, curr_vbuf, NULL)) {
			curr_vbuf->timestamp = k_uptime_get_32();
			k_fifo_put(&data->fifo_out, curr_vbuf);
		}

		next_vbuf = k_fifo_get(&data->fifo_in, K_NO_WAIT);
		if (next_vbuf == NULL) {
			return;
		}

		if (!atomic_ptr_cas(&data->vbuf, NULL, next_vbuf)) {
			k_fifo_put(&data->fifo_in, next_vbuf);
			return;
		}

		err = R_CEU_CaptureStart(data->fsp_ctrl, next_vbuf->buffer);
		if (err != FSP_SUCCESS) {
			atomic_ptr_clear(&data->vbuf);
			k_fifo_put(&data->fifo_out, next_vbuf);
			return;
		}
	}
}

static void video_renesas_ra_ceu_capture_stop(void)
{
	R_CEU->CAPSR = R_CEU_CAPSR_CPKIL_Msk;
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
	const struct video_renesas_ra_ceu_config *config = dev->config;
	struct video_renesas_ra_ceu_data *data = dev->data;
	fsp_err_t err;
	int ret;

	if (fmt->width > VIDEO_RA_CEU_WIDTH_MAX || fmt->width < VIDEO_RA_CEU_WIDTH_MIN) {
		LOG_DBG("Width %d out of supported range %d-%d", fmt->width, VIDEO_RA_CEU_WIDTH_MIN,
			VIDEO_RA_CEU_WIDTH_MAX);
		return -ENOTSUP;
	}
	if (fmt->width % VIDEO_RA_CEU_WIDTH_ALIGN != 0) {
		LOG_DBG("Width %d not a multiple of %d", fmt->width, VIDEO_RA_CEU_WIDTH_ALIGN);
		return -ENOTSUP;
	}

	if (fmt->height > VIDEO_RA_CEU_HEIGHT_MAX || fmt->height < VIDEO_RA_CEU_HEIGHT_MIN) {
		LOG_DBG("Height %d out of supported range %d-%d", fmt->height,
			VIDEO_RA_CEU_HEIGHT_MIN, VIDEO_RA_CEU_HEIGHT_MAX);
		return -ENOTSUP;
	}
	if (fmt->height % VIDEO_RA_CEU_HEIGHT_ALIGN != 0) {
		LOG_DBG("Height %d not a multiple of %d", fmt->height, VIDEO_RA_CEU_HEIGHT_ALIGN);
		return -ENOTSUP;
	}

	ret = video_set_format(config->source_dev, fmt);
	if (ret < 0) {
		LOG_DBG("Failed to set format on source device");
		return ret;
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

	fmt->pitch = fmt->width * video_bits_per_pixel(fmt->pixelformat) / BITS_PER_BYTE;

	memcpy(&data->fmt, fmt, sizeof(struct video_format));

	return 0;
}

static int video_renesas_ra_ceu_get_caps(const struct device *dev, struct video_caps *caps)
{
	const struct video_renesas_ra_ceu_config *config = dev->config;

	caps->min_vbuf_count = 1;
	caps->min_line_count = LINE_COUNT_HEIGHT;
	caps->max_line_count = LINE_COUNT_HEIGHT;

	return video_get_caps(config->source_dev, caps);
}

static int video_renesas_ra_ceu_set_stream(const struct device *dev, bool enable,
					   enum video_buf_type type)
{
	const struct video_renesas_ra_ceu_config *config = dev->config;
	struct video_renesas_ra_ceu_data *data = dev->data;
	struct video_buffer *next_vbuf;
	fsp_err_t err;
	int ret;

	if (!enable) {
		ret = video_stream_stop(config->source_dev, type);
		if (ret < 0) {
			LOG_DBG("Failed to stop source device stream");
			return -EIO;
		}

		video_renesas_ra_ceu_capture_stop();
		atomic_clear(&data->streaming);

		return 0;
	}

	if (atomic_get(&data->streaming)) {
		return -EBUSY;
	}

	next_vbuf = k_fifo_get(&data->fifo_in, K_NO_WAIT);
	if (next_vbuf == NULL) {
		LOG_DBG("No enqueued video buffers available to start streaming");
		return -EAGAIN;
	}

	if (!atomic_ptr_cas(&data->vbuf, NULL, next_vbuf)) {
		k_fifo_put(&data->fifo_in, next_vbuf);
		return 0;
	}

	err = R_CEU_CaptureStart(data->fsp_ctrl, next_vbuf->buffer);
	if (err != FSP_SUCCESS) {
		LOG_DBG("Failed to start CEU capture");
		atomic_ptr_clear(&data->vbuf);
		k_fifo_put(&data->fifo_out, next_vbuf);
		return -EIO;
	}

	ret = video_stream_start(config->source_dev, type);
	if (ret < 0) {
		LOG_DBG("Failed to start source device stream");
		atomic_ptr_clear(&data->vbuf);
		video_renesas_ra_ceu_capture_stop();
		return -EIO;
	}

	atomic_set(&data->streaming, 1);

	return 0;
}

static int video_renesas_ra_ceu_enqueue(const struct device *dev, struct video_buffer *vbuf)
{
	struct video_renesas_ra_ceu_data *data = dev->data;
	struct video_buffer *next_vbuf;
	const uint32_t buffer_size = data->fmt.pitch * data->fmt.height;
	fsp_err_t err;

	if (buffer_size > vbuf->size) {
		LOG_DBG("Enqueue buffer too small");
		return -EINVAL;
	}

	vbuf->bytesused = buffer_size;
	vbuf->line_offset = 0;

	k_fifo_put(&data->fifo_in, vbuf);

	if (!atomic_get(&data->streaming)) {
		return 0;
	}

	if (atomic_ptr_get(&data->vbuf)) {
		return 0;
	}

	next_vbuf = k_fifo_get(&data->fifo_in, K_NO_WAIT);
	if (!next_vbuf) {
		LOG_DBG("No enqueued video buffers available to restart streaming");
		return -EAGAIN;
	}

	if (!atomic_ptr_cas(&data->vbuf, NULL, next_vbuf)) {
		k_fifo_put(&data->fifo_in, next_vbuf);
		return 0;
	}

	err = R_CEU_CaptureStart(data->fsp_ctrl, next_vbuf->buffer);
	if (err != FSP_SUCCESS) {
		LOG_DBG("Failed to start CEU capture");
		atomic_ptr_clear(&data->vbuf);
		k_fifo_put(&data->fifo_out, next_vbuf);
		return -EIO;
	}

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
	struct video_buffer *vbuf;

	if (cancel) {
		if (atomic_cas(&data->streaming, 1, 0)) {
			video_renesas_ra_ceu_set_stream(dev, false, VIDEO_BUF_TYPE_OUTPUT);
		}

		vbuf = atomic_ptr_get(&data->vbuf);
		if (vbuf && atomic_ptr_cas(&data->vbuf, vbuf, NULL)) {
			k_fifo_put(&data->fifo_out, vbuf);
		}

		while ((vbuf = k_fifo_get(&data->fifo_in, K_NO_WAIT)) != NULL) {
			k_fifo_put(&data->fifo_out, vbuf);
		}

#ifdef CONFIG_POLL
		if (data->signal) {
			k_poll_signal_raise(data->signal, VIDEO_BUF_ABORTED);
		}
#endif

	} else {
		while (!k_fifo_is_empty(&data->fifo_in)) {
			k_sleep(K_MSEC(1));
		}
	}

	return 0;
}

#ifdef CONFIG_POLL
static int video_renesas_ra_ceu_set_signal(const struct device *dev, struct k_poll_signal *sig)
{
	struct video_renesas_ra_ceu_data *data = dev->data;

	data->signal = sig;
	return 0;
}
#endif

static int video_renesas_ra_ceu_init(const struct device *dev)
{
	const struct video_renesas_ra_ceu_config *config = dev->config;
	struct video_renesas_ra_ceu_data *data = dev->data;
	fsp_err_t err;
	int ret;

	ret = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_DBG("Failed to configure pinctrl");
		return ret;
	}

	if (!device_is_ready(config->clock_dev)) {
		LOG_DBG("Clock control device not ready");
		return -ENODEV;
	}

	ret = clock_control_on(config->clock_dev, (clock_control_subsys_t)&config->clock_subsys);
	if (ret < 0) {
		LOG_DBG("Failed to enable clock control");
		return ret;
	}

	config->irq_config_func(dev);

	err = R_CEU_Open(data->fsp_ctrl, data->fsp_cfg);
	if (err != FSP_SUCCESS) {
		LOG_DBG("Failed to open CEU hardware");
		return -EIO;
	}

	atomic_clear(&data->streaming);
	atomic_ptr_clear(&data->vbuf);
	k_fifo_init(&data->fifo_in);
	k_fifo_init(&data->fifo_out);

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
	static const struct video_renesas_ra_ceu_config video_renesas_ra_ceu_config##inst = {      \
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
		.p_context = (void *)DEVICE_DT_INST_GET(inst),                                     \
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
		.x_capture_pixels = VIDEO_RA_CEU_DEFAULT_WIDTH,                                    \
		.y_capture_pixels = VIDEO_RA_CEU_DEFAULT_HEIGHT,                                   \
		.x_capture_start_pixel = VIDEO_RA_CEU_DEFAULT_START_X,                             \
		.y_capture_start_pixel = VIDEO_RA_CEU_DEFAULT_START_Y,                             \
		.bytes_per_pixel = VIDEO_RA_CEU_DEFAULT_BYTES_PER_PIXEL,                           \
		.p_extend = &video_renesas_ra_ceu_fsp_extend_cfg##inst,                            \
		.p_callback = video_renesas_ra_ceu_callback,                                       \
		.p_context = (void *)DEVICE_DT_INST_GET(inst),                                     \
	};                                                                                         \
                                                                                                   \
	static struct video_renesas_ra_ceu_data video_renesas_ra_ceu_data##inst = {                \
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
