/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT espressif_esp32_csi

#include <zephyr/cache.h>
#include <zephyr/device.h>
#include <zephyr/drivers/interrupt_controller/intc_esp32.h>
#include <zephyr/drivers/video.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/video/video.h>

#include <esp_intr_alloc.h>
#include <hal/clk_gate_ll.h>
#include <hal/dw_gdma_ll.h>
#include <esp_dw_gdma.h>
#include <hal/hal_utils.h>
#include <hal/isp_hal.h>
#include <hal/isp_ll.h>
#include <hal/mipi_csi_brg_ll.h>
#include <hal/mipi_csi_hal.h>
#include <hal/mipi_csi_ll.h>
#include <soc/clk_tree_defs.h>
#include <soc/reg_base.h>

#include "video_common.h"

LOG_MODULE_REGISTER(video_esp32_csi, CONFIG_VIDEO_LOG_LEVEL);

#define VIDEO_ESP32_CSI_DMA_STOP_TIMEOUT_US 1000
#define VIDEO_ESP32_CSI_ISP_CLK_SRC_HZ      MHZ(240)
#define VIDEO_ESP32_CSI_ISP_CLK_DIV_MIN     2
#define VIDEO_ESP32_CSI_ISP_CLK_DIV_MAX     256
#define VIDEO_ESP32_CSI_BRG_BURST_LEN       512
#define VIDEO_ESP32_CSI_BRG_DATA_TYPE_MIN   0x12
#define VIDEO_ESP32_CSI_BRG_DATA_TYPE_MAX   0x2f
#define VIDEO_ESP32_CSI_MAX_CAPS            8
#define VIDEO_ESP32_CSI_WB_GAIN_UNITY       256
#define VIDEO_ESP32_CSI_WB_GAIN_MAX         4095
#define VIDEO_ESP32_CSI_WB_GAIN_RED_DEF     384
#define VIDEO_ESP32_CSI_WB_GAIN_BLUE_DEF    410

struct video_esp32_csi_ctrls {
	struct video_ctrl red_balance;
	struct video_ctrl blue_balance;
};

struct video_esp32_csi_config {
	const struct device *source_dev;
	int irq_source;
	int irq_priority;
	int irq_flags;
	uint8_t dma_channel;
	uint8_t data_lanes;
};

struct video_esp32_csi_data {
	struct video_esp32_csi_ctrls ctrls;
	mipi_csi_hal_context_t csi_hal;
	isp_hal_context_t isp_hal;
	struct video_format fmt;
	struct video_format src_fmt;
	struct video_format_cap caps[VIDEO_ESP32_CSI_MAX_CAPS + 1];
	struct video_buffer *active_vbuf;
	struct k_fifo fifo_in;
	struct k_fifo fifo_out;
	intr_handle_t dma_intr;
	uint32_t frame_drops;
	bool is_streaming;
#ifdef CONFIG_POLL
	struct k_poll_signal *signal_out;
#endif
};

struct video_esp32_csi_bayer {
	uint32_t pixelformat;
	isp_color_t color;
	color_raw_element_order_t order;
};

static const struct video_esp32_csi_bayer video_esp32_csi_bayer_fmts[] = {
	{VIDEO_PIX_FMT_SBGGR8, ISP_COLOR_RAW8, COLOR_RAW_ELEMENT_ORDER_BGGR},
	{VIDEO_PIX_FMT_SGBRG8, ISP_COLOR_RAW8, COLOR_RAW_ELEMENT_ORDER_GBRG},
	{VIDEO_PIX_FMT_SGRBG8, ISP_COLOR_RAW8, COLOR_RAW_ELEMENT_ORDER_GRBG},
	{VIDEO_PIX_FMT_SRGGB8, ISP_COLOR_RAW8, COLOR_RAW_ELEMENT_ORDER_RGGB},
	{VIDEO_PIX_FMT_SBGGR10P, ISP_COLOR_RAW10, COLOR_RAW_ELEMENT_ORDER_BGGR},
	{VIDEO_PIX_FMT_SGBRG10P, ISP_COLOR_RAW10, COLOR_RAW_ELEMENT_ORDER_GBRG},
	{VIDEO_PIX_FMT_SGRBG10P, ISP_COLOR_RAW10, COLOR_RAW_ELEMENT_ORDER_GRBG},
	{VIDEO_PIX_FMT_SRGGB10P, ISP_COLOR_RAW10, COLOR_RAW_ELEMENT_ORDER_RGGB},
};

static const struct video_esp32_csi_bayer *video_esp32_csi_find_bayer(uint32_t pixelformat)
{
	ARRAY_FOR_EACH_PTR(video_esp32_csi_bayer_fmts, bayer) {
		if (bayer->pixelformat == pixelformat) {
			return bayer;
		}
	}

	return NULL;
}

static void video_esp32_csi_dma_arm(const struct device *dev, struct video_buffer *vbuf)
{
	const struct video_esp32_csi_config *cfg = dev->config;
	dw_gdma_dev_t *dma = DW_GDMA_LL_GET_HW(0);
	uint8_t ch = cfg->dma_channel;
	uint32_t dst = (uint32_t)vbuf->buffer;

	dw_gdma_ll_channel_set_src_addr(dma, ch, MIPI_CSI_BRG_MEM_BASE);
	dw_gdma_ll_channel_set_dst_addr(dma, ch, dst);
	dw_gdma_ll_channel_set_trans_block_size(dma, ch, vbuf->bytesused / 8);
	dw_gdma_ll_channel_set_src_master_port(dma, ch, MIPI_CSI_BRG_MEM_BASE);
	dw_gdma_ll_channel_set_dst_master_port(dma, ch, dst);
	dw_gdma_ll_channel_enable(dma, ch, true);
}

static void video_esp32_csi_dma_stop(const struct device *dev)
{
	const struct video_esp32_csi_config *cfg = dev->config;
	dw_gdma_dev_t *dma = DW_GDMA_LL_GET_HW(0);
	uint8_t ch = cfg->dma_channel;

	dw_gdma_ll_channel_enable(dma, ch, false);
	if (!WAIT_FOR((dma->chen0.val & BIT(ch)) == 0U, VIDEO_ESP32_CSI_DMA_STOP_TIMEOUT_US,
		      k_busy_wait(1))) {
		dw_gdma_ll_channel_abort(dma, ch);
	}
}

static void video_esp32_csi_dma_isr(void *arg)
{
	const struct device *dev = arg;
	const struct video_esp32_csi_config *cfg = dev->config;
	struct video_esp32_csi_data *data = dev->data;
	dw_gdma_dev_t *dma = DW_GDMA_LL_GET_HW(0);
	uint8_t ch = cfg->dma_channel;
	struct video_buffer *next;
	uint32_t status;

	status = dw_gdma_ll_channel_get_intr_status(dma, ch);
	dw_gdma_ll_channel_clear_intr(dma, ch, status);

	if ((status & DW_GDMA_LL_CHANNEL_EVENT_DMA_TFR_DONE) == 0U || !data->is_streaming) {
		return;
	}

	next = k_fifo_get(&data->fifo_in, K_NO_WAIT);
	if (next == NULL) {
		data->frame_drops++;
		video_esp32_csi_dma_arm(dev, data->active_vbuf);
		return;
	}

	data->active_vbuf->timestamp = k_uptime_get_32();
	k_fifo_put(&data->fifo_out, data->active_vbuf);
	data->active_vbuf = next;
	video_esp32_csi_dma_arm(dev, next);

#ifdef CONFIG_POLL
	if (data->signal_out != NULL) {
		k_poll_signal_raise(data->signal_out, VIDEO_BUF_DONE);
	}
#endif
}

static int video_esp32_csi_dma_init(const struct device *dev)
{
	const struct video_esp32_csi_config *cfg = dev->config;
	struct video_esp32_csi_data *data = dev->data;
	dw_gdma_dev_t *dma = DW_GDMA_LL_GET_HW(0);
	uint8_t ch = cfg->dma_channel;
	int err;

	err = esp_dw_gdma_init();
	if (err != 0) {
		LOG_ERR("GDMA bring-up failed (%d)", err);
		return err;
	}

	dw_gdma_ll_channel_enable(dma, ch, false);

	dw_gdma_ll_channel_set_trans_flow(dma, ch, DW_GDMA_ROLE_PERIPH_CSI, DW_GDMA_ROLE_MEM,
					  DW_GDMA_FLOW_CTRL_SRC);

	dw_gdma_ll_channel_set_src_multi_block_type(dma, ch, DW_GDMA_BLOCK_TRANSFER_CONTIGUOUS);
	dw_gdma_ll_channel_set_dst_multi_block_type(dma, ch, DW_GDMA_BLOCK_TRANSFER_CONTIGUOUS);

	dw_gdma_ll_channel_set_src_handshake_interface(dma, ch, DW_GDMA_HANDSHAKE_HW);
	dw_gdma_ll_channel_set_dst_handshake_interface(dma, ch, DW_GDMA_HANDSHAKE_HW);
	dw_gdma_ll_channel_set_src_handshake_periph(dma, ch, DW_GDMA_ROLE_PERIPH_CSI);

	dw_gdma_ll_channel_set_priority(dma, ch, 1);
	dw_gdma_ll_channel_set_src_outstanding_limit(dma, ch, 5);
	dw_gdma_ll_channel_set_dst_outstanding_limit(dma, ch, 5);
	dw_gdma_ll_channel_set_src_periph_status_addr(dma, ch, MIPI_CSI_BRG_MEM_BASE);

	dw_gdma_ll_channel_set_src_burst_mode(dma, ch, DW_GDMA_BURST_MODE_FIXED);
	dw_gdma_ll_channel_set_dst_burst_mode(dma, ch, DW_GDMA_BURST_MODE_INCREMENT);
	dw_gdma_ll_channel_set_src_trans_width(dma, ch, DW_GDMA_TRANS_WIDTH_64);
	dw_gdma_ll_channel_set_dst_trans_width(dma, ch, DW_GDMA_TRANS_WIDTH_64);
	dw_gdma_ll_channel_set_src_burst_items(dma, ch, DW_GDMA_BURST_ITEMS_512);
	dw_gdma_ll_channel_set_dst_burst_items(dma, ch, DW_GDMA_BURST_ITEMS_512);
	dw_gdma_ll_channel_set_src_burst_len(dma, ch, 16);
	dw_gdma_ll_channel_set_dst_burst_len(dma, ch, 16);

	dw_gdma_ll_channel_enable_intr_generation(dma, ch, UINT32_MAX, true);
	dw_gdma_ll_channel_enable_intr_propagation(dma, ch, DW_GDMA_LL_CHANNEL_EVENT_DMA_TFR_DONE,
						   true);

	err = esp_intr_alloc_intrstatus(
		cfg->irq_source,
		ESP_INTR_FLAG_SHARED | ESP_PRIO_TO_FLAGS(cfg->irq_priority) |
			ESP_INT_FLAGS_CHECK(cfg->irq_flags),
		(uint32_t)dw_gdma_ll_get_intr_status_reg(dma), DW_GDMA_LL_CHANNEL_EVENT_MASK(ch),
		video_esp32_csi_dma_isr, (void *)dev, &data->dma_intr);
	if (err != 0) {
		LOG_ERR("Failed to allocate DMA interrupt (%d)", err);
		return -EIO;
	}

	return 0;
}

/* Start the bridge, host and ISP from reset with their clocks running */
static void video_esp32_csi_module_reset(void)
{
	unsigned int key = irq_lock();

	clk_gate_ll_ref_20m_clk_en(true);
	clk_gate_ll_ref_240m_clk_en(true);

	mipi_csi_ll_enable_brg_module_clock(0, true);
	mipi_csi_ll_reset_brg_module_clock(0);
	mipi_csi_brg_ll_enable_clock(MIPI_CSI_BRG_LL_GET_HW(0), true);

	mipi_csi_ll_enable_host_bus_clock(0, true);
	mipi_csi_ll_reset_host_clock(0);

	mipi_csi_ll_set_phy_clock_source(0, MIPI_CSI_PHY_CLK_SRC_DEFAULT);
	mipi_csi_ll_enable_phy_config_clock(0, true);

	isp_ll_enable_module_clock(true);
	isp_ll_reset_module_clock();
	isp_ll_select_clk_source(ISP_CLK_SRC_PLL240);

	irq_unlock(key);
}

/* The ISP consumes one pixel per clock, so its clock must not fall below the source pixel rate */
static void video_esp32_csi_set_isp_clock(const struct device *dev)
{
	const struct video_esp32_csi_config *cfg = dev->config;
	struct video_control ctrl = {.id = VIDEO_CID_PIXEL_RATE};
	hal_utils_clk_div_t isp_div = {.integer = VIDEO_ESP32_CSI_ISP_CLK_DIV_MIN};
	unsigned int key;

	if (video_get_ctrl(cfg->source_dev, &ctrl) == 0 && ctrl.val64 > 0) {
		isp_div.integer =
			CLAMP(VIDEO_ESP32_CSI_ISP_CLK_SRC_HZ / ctrl.val64,
			      VIDEO_ESP32_CSI_ISP_CLK_DIV_MIN, VIDEO_ESP32_CSI_ISP_CLK_DIV_MAX);
	}

	key = irq_lock();
	isp_ll_set_clock_div(&isp_div);
	irq_unlock(key);
}

static void video_esp32_csi_apply_wb_gain(const struct device *dev)
{
	struct video_esp32_csi_data *data = dev->data;
	isp_wbg_gain_t gain = {
		.gain_r = data->ctrls.red_balance.val,
		.gain_g = VIDEO_ESP32_CSI_WB_GAIN_UNITY,
		.gain_b = data->ctrls.blue_balance.val,
	};

	isp_ll_awb_set_wb_gain(data->isp_hal.hw, gain);
	isp_ll_shadow_update_wbg(data->isp_hal.hw, true);
}

static int video_esp32_csi_hw_config(const struct device *dev)
{
	const struct video_esp32_csi_config *cfg = dev->config;
	struct video_esp32_csi_data *data = dev->data;
	mipi_csi_hal_config_t csi_config = {
		.lanes_num = cfg->data_lanes,
		.frame_width = data->fmt.width,
		.frame_height = data->fmt.height,
	};
	const struct video_esp32_csi_bayer *bayer;
	isp_dev_t *isp;
	int64_t link_freq;
	uint8_t bpp;

	bayer = video_esp32_csi_find_bayer(data->src_fmt.pixelformat);
	if (bayer == NULL) {
		return -ENOTSUP;
	}

	bpp = video_bits_per_pixel(data->src_fmt.pixelformat);
	link_freq = video_get_csi_link_freq(cfg->source_dev, bpp, cfg->data_lanes);
	if (link_freq < 0) {
		LOG_ERR("Failed to get the link frequency of the source (%lld)", link_freq);
		return (int)link_freq;
	}

	csi_config.lane_bit_rate_mbps = (int)(link_freq * 2 / MHZ(1));

	video_esp32_csi_module_reset();

	LOG_DBG("%ux%u, %u lanes at %d Mbps", data->fmt.width, data->fmt.height, cfg->data_lanes,
		csi_config.lane_bit_rate_mbps);

	mipi_csi_hal_init(&data->csi_hal, &csi_config);
	mipi_csi_brg_ll_set_intput_data_h_pixel_num(data->csi_hal.bridge_dev, data->fmt.width);
	mipi_csi_brg_ll_set_intput_data_v_row_num(data->csi_hal.bridge_dev, data->fmt.height);
	mipi_csi_brg_ll_set_burst_len(data->csi_hal.bridge_dev, VIDEO_ESP32_CSI_BRG_BURST_LEN);
	mipi_csi_brg_ll_set_data_type_min(data->csi_hal.bridge_dev,
					  VIDEO_ESP32_CSI_BRG_DATA_TYPE_MIN);
	mipi_csi_brg_ll_set_data_type_max(data->csi_hal.bridge_dev,
					  VIDEO_ESP32_CSI_BRG_DATA_TYPE_MAX);
	mipi_csi_brg_ll_enable_color_conversion(data->csi_hal.bridge_dev, true);
	mipi_csi_brg_ll_set_color_mode_bypass(data->csi_hal.bridge_dev, true);

	video_esp32_csi_set_isp_clock(dev);
	isp_hal_init(&data->isp_hal, 0);
	isp = data->isp_hal.hw;

	if (!isp_ll_set_input_data_color_format(isp, bayer->color) ||
	    !isp_ll_set_output_data_color_format(isp, ISP_COLOR_RGB565)) {
		return -ENOTSUP;
	}

	isp_ll_clk_enable(isp, true);
	isp_ll_set_input_data_source(isp, ISP_INPUT_DATA_SOURCE_CSI);
	isp_ll_enable_line_start_packet_exist(isp, false);
	isp_ll_enable_line_end_packet_exist(isp, false);
	isp_ll_set_intput_data_h_pixel_num(isp, data->fmt.width);
	isp_ll_set_intput_data_v_row_num(isp, data->fmt.height);
	isp_ll_set_bayer_mode(isp, bayer->order);
	isp_ll_shadow_set_mode(isp, ISP_SHADOW_MODE_UPDATE_ONLY_NEXT_VSYNC);

	isp_ll_awb_set_wb_gain_clk_ctrl_mode(isp, ISP_LL_PIPELINE_CLK_CTRL_AUTO);
	isp_ll_awb_enable_wb_gain(isp, true);
	video_esp32_csi_apply_wb_gain(dev);

	return 0;
}

static int video_esp32_csi_set_stream(const struct device *dev, bool enable,
				      enum video_buf_type type)
{
	const struct video_esp32_csi_config *cfg = dev->config;
	struct video_esp32_csi_data *data = dev->data;
	int ret;

	if (!enable) {
		if (!data->is_streaming) {
			return 0;
		}

		ret = video_stream_stop(cfg->source_dev, type);
		if (ret < 0) {
			LOG_ERR("Failed to stop the source device");
		}

		data->is_streaming = false;
		mipi_csi_brg_ll_enable(data->csi_hal.bridge_dev, false);
		video_esp32_csi_dma_stop(dev);
		isp_ll_enable(data->isp_hal.hw, false);

		if (data->active_vbuf != NULL) {
			k_fifo_put(&data->fifo_in, data->active_vbuf);
			data->active_vbuf = NULL;
		}

		if (data->frame_drops > 0U) {
			LOG_DBG("%u frames dropped", data->frame_drops);
		}

		return ret;
	}

	if (data->is_streaming) {
		return -EBUSY;
	}

	data->active_vbuf = k_fifo_get(&data->fifo_in, K_NO_WAIT);
	if (data->active_vbuf == NULL) {
		LOG_ERR("No enqueued video buffers available");
		return -EAGAIN;
	}

	ret = video_esp32_csi_hw_config(dev);
	if (ret < 0) {
		k_fifo_put(&data->fifo_in, data->active_vbuf);
		data->active_vbuf = NULL;
		return ret;
	}

	data->frame_drops = 0;
	data->is_streaming = true;

	isp_ll_enable(data->isp_hal.hw, true);
	video_esp32_csi_dma_arm(dev, data->active_vbuf);
	mipi_csi_brg_ll_enable(data->csi_hal.bridge_dev, true);

	ret = video_stream_start(cfg->source_dev, type);
	if (ret < 0) {
		LOG_ERR("Failed to start the source device");
		video_esp32_csi_set_stream(dev, false, type);
		return ret;
	}

	return 0;
}

static int video_esp32_csi_get_caps(const struct device *dev, struct video_caps *caps)
{
	struct video_esp32_csi_data *data = dev->data;

	caps->min_vbuf_count = 2;
	caps->format_caps = data->caps;

	return 0;
}

/* Pick the first Bayer format of the source that provides the requested frame size */
static int video_esp32_csi_find_src_fmt(const struct device *dev, const struct video_format *fmt,
					struct video_format *src_fmt)
{
	const struct video_esp32_csi_config *cfg = dev->config;
	struct video_caps src_caps = {.type = VIDEO_BUF_TYPE_OUTPUT};
	int ret;

	ret = video_get_caps(cfg->source_dev, &src_caps);
	if (ret < 0) {
		return ret;
	}

	for (size_t i = 0; src_caps.format_caps[i].pixelformat != 0U; i++) {
		const struct video_format_cap *cap = &src_caps.format_caps[i];

		if (video_esp32_csi_find_bayer(cap->pixelformat) == NULL ||
		    !IN_RANGE(fmt->width, cap->width_min, cap->width_max) ||
		    !IN_RANGE(fmt->height, cap->height_min, cap->height_max)) {
			continue;
		}

		src_fmt->type = VIDEO_BUF_TYPE_OUTPUT;
		src_fmt->pixelformat = cap->pixelformat;
		src_fmt->width = fmt->width;
		src_fmt->height = fmt->height;

		return 0;
	}

	LOG_ERR("No source format for %ux%u", fmt->width, fmt->height);

	return -ENOTSUP;
}

static int video_esp32_csi_set_fmt(const struct device *dev, struct video_format *fmt)
{
	const struct video_esp32_csi_config *cfg = dev->config;
	struct video_esp32_csi_data *data = dev->data;
	struct video_format src_fmt;
	int ret;

	if (data->is_streaming || !k_fifo_is_empty(&data->fifo_in)) {
		return -EBUSY;
	}

	if (fmt->pixelformat != VIDEO_PIX_FMT_RGB565) {
		LOG_ERR("Format '%s' not supported", VIDEO_FOURCC_TO_STR(fmt->pixelformat));
		return -ENOTSUP;
	}

	ret = video_esp32_csi_find_src_fmt(dev, fmt, &src_fmt);
	if (ret < 0) {
		return ret;
	}

	ret = video_set_format(cfg->source_dev, &src_fmt);
	if (ret < 0) {
		LOG_ERR("Failed to apply format to the source");
		return ret;
	}

	ret = video_estimate_fmt_size(fmt);
	if (ret < 0) {
		return ret;
	}

	data->src_fmt = src_fmt;
	data->fmt = *fmt;

	return 0;
}

static int video_esp32_csi_get_fmt(const struct device *dev, struct video_format *fmt)
{
	struct video_esp32_csi_data *data = dev->data;

	*fmt = data->fmt;

	return 0;
}

static int video_esp32_csi_enqueue(const struct device *dev, struct video_buffer *vbuf)
{
	struct video_esp32_csi_data *data = dev->data;

	if (vbuf->size < data->fmt.size ||
	    ((uintptr_t)vbuf->buffer % CONFIG_ESP32_CACHE_L2_LINE_SIZE) != 0U ||
	    (data->fmt.size % CONFIG_ESP32_CACHE_L2_LINE_SIZE) != 0U) {
		return -EINVAL;
	}

	vbuf->bytesused = data->fmt.size;
	vbuf->line_offset = 0;

	sys_cache_data_flush_and_invd_range(vbuf->buffer, vbuf->bytesused);
	k_fifo_put(&data->fifo_in, vbuf);

	return 0;
}

static int video_esp32_csi_dequeue(const struct device *dev, struct video_buffer **vbuf,
				   k_timeout_t timeout)
{
	struct video_esp32_csi_data *data = dev->data;

	*vbuf = k_fifo_get(&data->fifo_out, timeout);
	if (*vbuf == NULL) {
		return -EAGAIN;
	}

	sys_cache_data_invd_range((*vbuf)->buffer, (*vbuf)->bytesused);

	return 0;
}

static int video_esp32_csi_flush(const struct device *dev, bool cancel)
{
	struct video_esp32_csi_data *data = dev->data;
	struct video_buffer *vbuf;

	if (!cancel) {
		while (!k_fifo_is_empty(&data->fifo_in)) {
			k_sleep(K_MSEC(1));
		}
		return 0;
	}

	while ((vbuf = k_fifo_get(&data->fifo_in, K_NO_WAIT)) != NULL) {
		k_fifo_put(&data->fifo_out, vbuf);
#ifdef CONFIG_POLL
		if (data->signal_out != NULL) {
			k_poll_signal_raise(data->signal_out, VIDEO_BUF_ABORTED);
		}
#endif
	}

	return 0;
}

#ifdef CONFIG_POLL
static int video_esp32_csi_set_signal(const struct device *dev, struct k_poll_signal *sig)
{
	struct video_esp32_csi_data *data = dev->data;

	data->signal_out = sig;

	return 0;
}
#endif

static int video_esp32_csi_set_frmival(const struct device *dev, struct video_frmival *frmival)
{
	const struct video_esp32_csi_config *cfg = dev->config;

	return video_set_frmival(cfg->source_dev, frmival);
}

static int video_esp32_csi_get_frmival(const struct device *dev, struct video_frmival *frmival)
{
	const struct video_esp32_csi_config *cfg = dev->config;

	return video_get_frmival(cfg->source_dev, frmival);
}

static int video_esp32_csi_enum_frmival(const struct device *dev, struct video_frmival_enum *fie)
{
	const struct video_esp32_csi_config *cfg = dev->config;
	const struct video_format *fmt = fie->format;
	struct video_format src_fmt;
	int ret;

	ret = video_esp32_csi_find_src_fmt(dev, fmt, &src_fmt);
	if (ret < 0) {
		return ret;
	}

	fie->format = &src_fmt;
	ret = video_enum_frmival(cfg->source_dev, fie);
	fie->format = fmt;

	return ret;
}

static int video_esp32_csi_set_ctrl(const struct device *dev, uint32_t id)
{
	struct video_esp32_csi_data *data = dev->data;

	switch (id) {
	case VIDEO_CID_RED_BALANCE:
	case VIDEO_CID_BLUE_BALANCE:
		if (data->is_streaming) {
			video_esp32_csi_apply_wb_gain(dev);
		}
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int video_esp32_csi_init_ctrls(const struct device *dev)
{
	struct video_esp32_csi_data *data = dev->data;
	int ret;

	ret = video_init_ctrl(&data->ctrls.red_balance, dev, VIDEO_CID_RED_BALANCE,
			      (struct video_ctrl_range){.min = 0,
							.max = VIDEO_ESP32_CSI_WB_GAIN_MAX,
							.step = 1,
							.def = VIDEO_ESP32_CSI_WB_GAIN_RED_DEF});
	if (ret < 0) {
		return ret;
	}

	return video_init_ctrl(&data->ctrls.blue_balance, dev, VIDEO_CID_BLUE_BALANCE,
			       (struct video_ctrl_range){.min = 0,
							 .max = VIDEO_ESP32_CSI_WB_GAIN_MAX,
							 .step = 1,
							 .def = VIDEO_ESP32_CSI_WB_GAIN_BLUE_DEF});
}

static int video_esp32_csi_init_caps(const struct device *dev)
{
	const struct video_esp32_csi_config *cfg = dev->config;
	struct video_esp32_csi_data *data = dev->data;
	struct video_caps src_caps = {.type = VIDEO_BUF_TYPE_OUTPUT};
	size_t n = 0;
	size_t j;
	int ret;

	ret = video_get_caps(cfg->source_dev, &src_caps);
	if (ret < 0) {
		return ret;
	}

	for (size_t i = 0; src_caps.format_caps[i].pixelformat != 0U; i++) {
		const struct video_format_cap *cap = &src_caps.format_caps[i];

		if (video_esp32_csi_find_bayer(cap->pixelformat) == NULL) {
			continue;
		}

		if (n == VIDEO_ESP32_CSI_MAX_CAPS) {
			break;
		}

		/* Every Bayer variant of one frame size maps to the same RGB565 cap */
		for (j = 0; j < n; j++) {
			if (data->caps[j].width_min == cap->width_min &&
			    data->caps[j].width_max == cap->width_max &&
			    data->caps[j].height_min == cap->height_min &&
			    data->caps[j].height_max == cap->height_max) {
				break;
			}
		}

		if (j < n) {
			continue;
		}

		data->caps[n] = *cap;
		data->caps[n].pixelformat = VIDEO_PIX_FMT_RGB565;
		n++;
	}

	if (n == 0U) {
		LOG_ERR("Source provides no supported format");
		return -ENOTSUP;
	}

	return 0;
}

static int video_esp32_csi_init(const struct device *dev)
{
	const struct video_esp32_csi_config *cfg = dev->config;
	struct video_esp32_csi_data *data = dev->data;
	struct video_format fmt = {.type = VIDEO_BUF_TYPE_OUTPUT};
	int ret;

	if (!device_is_ready(cfg->source_dev)) {
		LOG_ERR("Source device %s is not ready", cfg->source_dev->name);
		return -ENODEV;
	}

	k_fifo_init(&data->fifo_in);
	k_fifo_init(&data->fifo_out);

	ret = video_esp32_csi_init_caps(dev);
	if (ret < 0) {
		return ret;
	}

	ret = video_esp32_csi_init_ctrls(dev);
	if (ret < 0) {
		return ret;
	}

	ret = video_esp32_csi_dma_init(dev);
	if (ret < 0) {
		return ret;
	}

	fmt.pixelformat = data->caps[0].pixelformat;
	fmt.width = data->caps[0].width_min;
	fmt.height = data->caps[0].height_min;

	return video_esp32_csi_set_fmt(dev, &fmt);
}

static DEVICE_API(video, video_esp32_csi_driver_api) = {
	.set_format = video_esp32_csi_set_fmt,
	.get_format = video_esp32_csi_get_fmt,
	.set_stream = video_esp32_csi_set_stream,
	.get_caps = video_esp32_csi_get_caps,
	.enqueue = video_esp32_csi_enqueue,
	.dequeue = video_esp32_csi_dequeue,
	.flush = video_esp32_csi_flush,
	.set_ctrl = video_esp32_csi_set_ctrl,
	.set_frmival = video_esp32_csi_set_frmival,
	.get_frmival = video_esp32_csi_get_frmival,
	.enum_frmival = video_esp32_csi_enum_frmival,
#ifdef CONFIG_POLL
	.set_signal = video_esp32_csi_set_signal,
#endif
};

#define VIDEO_ESP32_CSI_EP(n)     DT_INST_ENDPOINT_BY_ID(n, 0, 0)
#define VIDEO_ESP32_CSI_SOURCE(n) DEVICE_DT_GET(DT_NODE_REMOTE_DEVICE(VIDEO_ESP32_CSI_EP(n)))

#define VIDEO_ESP32_CSI_INIT(n)                                                                    \
	static const struct video_esp32_csi_config video_esp32_csi_config_##n = {                  \
		.source_dev = VIDEO_ESP32_CSI_SOURCE(n),                                           \
		.irq_source = DT_INST_IRQ_BY_IDX(n, 0, irq),                                       \
		.irq_priority = DT_INST_IRQ_BY_IDX(n, 0, priority),                                \
		.irq_flags = DT_INST_IRQ_BY_IDX(n, 0, flags),                                      \
		.dma_channel = DT_INST_PROP(n, dma_channel),                                       \
		.data_lanes = DT_PROP_LEN(VIDEO_ESP32_CSI_EP(n), data_lanes),                      \
	};                                                                                         \
                                                                                                   \
	static struct video_esp32_csi_data video_esp32_csi_data_##n;                               \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, video_esp32_csi_init, NULL, &video_esp32_csi_data_##n,            \
			      &video_esp32_csi_config_##n, POST_KERNEL,                            \
			      CONFIG_VIDEO_ESP32_CSI_INIT_PRIORITY, &video_esp32_csi_driver_api);  \
                                                                                                   \
	VIDEO_DEVICE_DEFINE(esp32_csi_##n, DEVICE_DT_INST_GET(n), VIDEO_ESP32_CSI_SOURCE(n));

DT_INST_FOREACH_STATUS_OKAY(VIDEO_ESP32_CSI_INIT)
