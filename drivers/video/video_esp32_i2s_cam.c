/*
 * SPDX-FileCopyrightText: 2026 Muhammad Waleed Badar
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT espressif_esp32_i2s_cam

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/interrupt_controller/intc_esp32.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/video.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/spinlock.h>

#include <esp_memory_utils.h>
#include <esp_rom_gpio.h>
#include <hal/i2s_ll.h>
#include <soc/gpio_pins.h>
#include <soc/gpio_sig_map.h>
#include <soc/lldesc.h>

#include "video_common.h"

LOG_MODULE_REGISTER(video_esp32_i2s_cam, CONFIG_VIDEO_LOG_LEVEL);

/* Camera mode shares the registers, clock and interrupt of the I2S0 controller node */
#define VIDEO_ESP32_I2S_CAM_I2S_NODE DT_INST_PARENT(0)

BUILD_ASSERT(!DT_NODE_HAS_STATUS_OKAY(VIDEO_ESP32_I2S_CAM_I2S_NODE),
	     "The I2S0 controller cannot be an audio controller and a camera interface");

/* One DMA descriptor per chunk, an RX EOF interrupt is raised for each chunk */
#define VIDEO_ESP32_I2S_CAM_CHUNK_WORDS (LLDESC_MAX_NUM_PER_DESC / sizeof(uint32_t))
#define VIDEO_ESP32_I2S_CAM_CHUNK_NUM   CONFIG_VIDEO_ESP32_I2S_CAM_DMA_CHUNKS

/* 32-bit single channel RX FIFO mode: one camera byte per word, in bits [23:16] */
#define VIDEO_ESP32_I2S_CAM_FIFO_MOD     3U
#define VIDEO_ESP32_I2S_CAM_SAMPLE_SHIFT 16U

/* Fastest pixel clock the controller samples without losing bytes in this mode */
#define VIDEO_ESP32_I2S_CAM_PCLK_MAX MHZ(10)

struct video_esp32_i2s_cam_config {
	i2s_dev_t *hw;
	const struct device *source_dev;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	const struct pinctrl_dev_config *pcfg;
	struct gpio_dt_spec vsync_gpio;
	int irq_source;
	int irq_priority;
	int irq_flags;
};

struct video_esp32_i2s_cam_data {
	const struct device *dev;
	struct k_spinlock lock;
	struct gpio_callback vsync_cb;
	struct video_format fmt;
	struct video_buffer *active_vbuf;
	struct k_fifo fifo_in;
	struct k_fifo fifo_out;
	uint32_t frame_len;
	uint32_t frame_max;
	uint32_t chunk_words;
	uint8_t chunk_idx;
	bool frame_done;
	bool capturing;
	bool is_streaming;
#ifdef CONFIG_POLL
	struct k_poll_signal *signal_out;
#endif
	lldesc_t dma_desc[VIDEO_ESP32_I2S_CAM_CHUNK_NUM];
	uint32_t dma_buf[VIDEO_ESP32_I2S_CAM_CHUNK_NUM][VIDEO_ESP32_I2S_CAM_CHUNK_WORDS];
};

static void video_esp32_i2s_cam_raise_signal(struct video_esp32_i2s_cam_data *data, int result)
{
#ifdef CONFIG_POLL
	if (data->signal_out != NULL) {
		k_poll_signal_raise(data->signal_out, result);
	}
#endif
}

/* Extract the camera bytes of one DMA chunk into the active video buffer */
static void video_esp32_i2s_cam_copy(struct video_esp32_i2s_cam_data *data, const uint32_t *src)
{
	struct video_buffer *vbuf = data->active_vbuf;
	uint32_t len;
	uint8_t *dst;

	if (vbuf == NULL || data->frame_done) {
		return;
	}

	len = MIN(data->chunk_words, data->frame_max - data->frame_len);
	dst = &vbuf->buffer[data->frame_len];

	for (uint32_t i = 0; i < len; i++) {
		dst[i] = (uint8_t)(src[i] >> VIDEO_ESP32_I2S_CAM_SAMPLE_SHIFT);
	}

	data->frame_len += len;
	data->frame_done = (data->frame_len == data->frame_max);
}

/* Copy all chunks completed by the DMA since the last call */
static void video_esp32_i2s_cam_drain(struct video_esp32_i2s_cam_data *data)
{
	const struct video_esp32_i2s_cam_config *cfg = data->dev->config;
	uint32_t eof_addr;
	uint32_t last;
	uint32_t end;

	if ((i2s_ll_get_intr_status(cfg->hw) & I2S_LL_EVENT_RX_EOF) == 0U) {
		return;
	}

	i2s_ll_clear_intr_status(cfg->hw, I2S_LL_EVENT_RX_EOF);

	if (!data->capturing) {
		return;
	}

	/*
	 * The DMA may have completed several chunks before the interrupt was
	 * serviced. A chunk completing after the flag was cleared is copied now
	 * and raises the flag again, so a pass may find nothing new.
	 */
	i2s_ll_rx_get_eof_des_addr(cfg->hw, &eof_addr);
	last = (eof_addr - (uint32_t)&data->dma_desc[0]) / sizeof(lldesc_t);
	if (last >= VIDEO_ESP32_I2S_CAM_CHUNK_NUM) {
		return;
	}

	end = (last + 1U) % VIDEO_ESP32_I2S_CAM_CHUNK_NUM;
	while (data->chunk_idx != end) {
		video_esp32_i2s_cam_copy(data, data->dma_buf[data->chunk_idx]);
		data->chunk_idx = (data->chunk_idx + 1U) % VIDEO_ESP32_I2S_CAM_CHUNK_NUM;
	}
}

static void video_esp32_i2s_cam_stop_dma(const struct video_esp32_i2s_cam_config *cfg)
{
	i2s_ll_rx_stop(cfg->hw);
	i2s_ll_rx_stop_link(cfg->hw);
}

static void video_esp32_i2s_cam_start_frame(struct video_esp32_i2s_cam_data *data)
{
	const struct video_esp32_i2s_cam_config *cfg = data->dev->config;
	i2s_dev_t *hw = cfg->hw;

	if (data->active_vbuf == NULL) {
		data->active_vbuf = k_fifo_get(&data->fifo_in, K_NO_WAIT);
		if (data->active_vbuf == NULL) {
			LOG_DBG("Frame dropped, no buffer available");
			data->capturing = false;
			return;
		}
	}

	data->frame_len = 0U;
	data->frame_max = MIN(data->active_vbuf->size, data->fmt.size);
	data->frame_done = false;
	data->chunk_idx = 0U;

	i2s_ll_rx_reset(hw);
	i2s_ll_rx_reset_fifo(hw);
	hw->lc_conf.ahbm_fifo_rst = 1;
	hw->lc_conf.ahbm_fifo_rst = 0;
	hw->lc_conf.ahbm_rst = 1;
	hw->lc_conf.ahbm_rst = 0;
	i2s_ll_clear_intr_status(hw, I2S_LL_EVENT_RX_EOF);

	i2s_ll_rx_set_eof_num(hw, data->chunk_words * sizeof(uint32_t));
	i2s_ll_rx_start_link(hw, (uint32_t)&data->dma_desc[0]);
	i2s_ll_rx_start(hw);

	data->capturing = true;
}

static void video_esp32_i2s_cam_finish_frame(struct video_esp32_i2s_cam_data *data)
{
	struct video_buffer *vbuf = data->active_vbuf;

	/* The chunks divide the frame, so a frame that ends inside a chunk is short */
	if (vbuf == NULL || !data->frame_done) {
		/* Flushed, or incomplete: keep the buffer for the next frame */
		LOG_DBG("Incomplete frame dropped (%u bytes)", data->frame_len);
		return;
	}

	vbuf->bytesused = data->frame_len;
	vbuf->line_offset = 0U;
	vbuf->timestamp = k_uptime_get_32();
	k_fifo_put(&data->fifo_out, vbuf);
	data->active_vbuf = NULL;

	video_esp32_i2s_cam_raise_signal(data, VIDEO_BUF_DONE);
}

/* Largest chunk that divides the frame, so that its last byte completes a chunk */
static uint32_t video_esp32_i2s_cam_chunk_words(uint32_t frame_size)
{
	uint32_t words = VIDEO_ESP32_I2S_CAM_CHUNK_WORDS;

	while (frame_size % words != 0U) {
		words--;
	}

	return words;
}

/* Frames are delimited by their size, which a compressed format does not have */
static int video_esp32_i2s_cam_fmt_size(struct video_format *fmt)
{
	int ret;

	ret = video_estimate_fmt_size(fmt);
	if (ret < 0) {
		return ret;
	}

	return fmt->pitch == 0U ? -ENOTSUP : 0;
}

/* Have the sensor use the fastest of its link frequencies that the controller can sample */
static int video_esp32_i2s_cam_select_link_freq(const struct device *dev)
{
	const struct video_esp32_i2s_cam_config *cfg = dev->config;
	struct video_ctrl_query cq = {.dev = cfg->source_dev, .id = VIDEO_CID_LINK_FREQ};
	struct video_control ctrl = {.id = VIDEO_CID_LINK_FREQ, .val = -1};

	/* A sensor that offers no choice sets its pixel clock itself */
	if (video_query_ctrl(&cq) < 0 || cq.int_menu == NULL ||
	    (cq.flags & VIDEO_CTRL_FLAG_READ_ONLY) != 0U) {
		return 0;
	}

	for (int32_t i = cq.range.min; i <= cq.range.max; i++) {
		if (cq.int_menu[i] <= VIDEO_ESP32_I2S_CAM_PCLK_MAX &&
		    (ctrl.val < 0 || cq.int_menu[i] > cq.int_menu[ctrl.val])) {
			ctrl.val = i;
		}
	}

	if (ctrl.val < 0) {
		LOG_ERR("The sensor has no link frequency of %u Hz or less",
			VIDEO_ESP32_I2S_CAM_PCLK_MAX);
		return -EINVAL;
	}

	return video_set_ctrl(cfg->source_dev, &ctrl);
}

/* A VSYNC edge ends the current frame and starts capturing the next one */
static void video_esp32_i2s_cam_vsync_handler(const struct device *port, struct gpio_callback *cb,
					      gpio_port_pins_t pins)
{
	struct video_esp32_i2s_cam_data *data =
		CONTAINER_OF(cb, struct video_esp32_i2s_cam_data, vsync_cb);
	const struct video_esp32_i2s_cam_config *cfg = data->dev->config;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	ARG_UNUSED(port);
	ARG_UNUSED(pins);

	if (data->is_streaming) {
		if (data->capturing) {
			video_esp32_i2s_cam_stop_dma(cfg);
			video_esp32_i2s_cam_drain(data);
			video_esp32_i2s_cam_finish_frame(data);
		}

		video_esp32_i2s_cam_start_frame(data);
	}

	k_spin_unlock(&data->lock, key);
}

static void video_esp32_i2s_cam_isr(void *arg)
{
	const struct device *dev = arg;
	struct video_esp32_i2s_cam_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	video_esp32_i2s_cam_drain(data);

	k_spin_unlock(&data->lock, key);
}

static int video_esp32_i2s_cam_set_stream(const struct device *dev, bool enable,
					  enum video_buf_type type)
{
	const struct video_esp32_i2s_cam_config *cfg = dev->config;
	struct video_esp32_i2s_cam_data *data = dev->data;
	k_spinlock_key_t key;
	int ret;

	if (!enable) {
		ret = gpio_pin_interrupt_configure_dt(&cfg->vsync_gpio, GPIO_INT_DISABLE);
		if (ret < 0) {
			return ret;
		}

		key = k_spin_lock(&data->lock);
		video_esp32_i2s_cam_stop_dma(cfg);
		data->is_streaming = false;
		data->capturing = false;
		k_spin_unlock(&data->lock, key);

		return video_stream_stop(cfg->source_dev, type);
	}

	if (data->is_streaming) {
		return -EBUSY;
	}

	ret = video_esp32_i2s_cam_select_link_freq(dev);
	if (ret < 0) {
		return ret;
	}

	data->fmt.type = type;
	ret = video_get_format(cfg->source_dev, &data->fmt);
	if (ret < 0) {
		return ret;
	}

	ret = video_esp32_i2s_cam_fmt_size(&data->fmt);
	if (ret < 0) {
		return ret;
	}

	data->chunk_words = video_esp32_i2s_cam_chunk_words(data->fmt.size);
	for (int i = 0; i < VIDEO_ESP32_I2S_CAM_CHUNK_NUM; i++) {
		data->dma_desc[i].size = data->chunk_words * sizeof(uint32_t);
		data->dma_desc[i].length = data->chunk_words * sizeof(uint32_t);
	}

	ret = video_stream_start(cfg->source_dev, type);
	if (ret < 0) {
		return ret;
	}

	data->is_streaming = true;

	ret = gpio_pin_interrupt_configure_dt(&cfg->vsync_gpio, GPIO_INT_EDGE_TO_INACTIVE);
	if (ret < 0) {
		data->is_streaming = false;
		(void)video_stream_stop(cfg->source_dev, type);
		return ret;
	}

	return 0;
}

static int video_esp32_i2s_cam_get_caps(const struct device *dev, struct video_caps *caps)
{
	const struct video_esp32_i2s_cam_config *cfg = dev->config;

	/* One buffer is filled while the other is processed by the application */
	caps->min_vbuf_count = 2;

	/* The DMA only writes the internal chunks, video buffers are filled by the CPU */
	caps->buf_align = 0;

	return video_get_caps(cfg->source_dev, caps);
}

static int video_esp32_i2s_cam_get_fmt(const struct device *dev, struct video_format *fmt)
{
	const struct video_esp32_i2s_cam_config *cfg = dev->config;
	int ret;

	ret = video_get_format(cfg->source_dev, fmt);
	if (ret < 0) {
		return ret;
	}

	return video_estimate_fmt_size(fmt);
}

static int video_esp32_i2s_cam_set_fmt(const struct device *dev, struct video_format *fmt)
{
	const struct video_esp32_i2s_cam_config *cfg = dev->config;
	struct video_esp32_i2s_cam_data *data = dev->data;
	int ret;

	if (data->is_streaming) {
		return -EBUSY;
	}

	ret = video_set_format(cfg->source_dev, fmt);
	if (ret < 0) {
		return ret;
	}

	return video_esp32_i2s_cam_fmt_size(fmt);
}

static int video_esp32_i2s_cam_enqueue(const struct device *dev, struct video_buffer *vbuf)
{
	struct video_esp32_i2s_cam_data *data = dev->data;

	k_fifo_put(&data->fifo_in, vbuf);

	return 0;
}

static int video_esp32_i2s_cam_dequeue(const struct device *dev, struct video_buffer **vbuf,
				       k_timeout_t timeout)
{
	struct video_esp32_i2s_cam_data *data = dev->data;

	*vbuf = k_fifo_get(&data->fifo_out, timeout);
	if (*vbuf == NULL) {
		return -EAGAIN;
	}

	return 0;
}

static int video_esp32_i2s_cam_flush(const struct device *dev, bool cancel)
{
	struct video_esp32_i2s_cam_data *data = dev->data;
	struct video_buffer *vbuf;
	k_spinlock_key_t key;

	if (!cancel) {
		while (!k_fifo_is_empty(&data->fifo_in)) {
			k_sleep(K_MSEC(1));
		}

		return 0;
	}

	key = k_spin_lock(&data->lock);
	vbuf = data->active_vbuf;
	data->active_vbuf = NULL;
	k_spin_unlock(&data->lock, key);

	if (vbuf != NULL) {
		k_fifo_put(&data->fifo_out, vbuf);
		video_esp32_i2s_cam_raise_signal(data, VIDEO_BUF_ABORTED);
	}

	while ((vbuf = k_fifo_get(&data->fifo_in, K_NO_WAIT)) != NULL) {
		k_fifo_put(&data->fifo_out, vbuf);
		video_esp32_i2s_cam_raise_signal(data, VIDEO_BUF_ABORTED);
	}

	return 0;
}

/* The frame intervals of the sensor are bounded by the link frequency it is given */
static int video_esp32_i2s_cam_set_frmival(const struct device *dev, struct video_frmival *frmival)
{
	const struct video_esp32_i2s_cam_config *cfg = dev->config;
	int ret;

	ret = video_esp32_i2s_cam_select_link_freq(dev);
	if (ret < 0) {
		return ret;
	}

	return video_set_frmival(cfg->source_dev, frmival);
}

static int video_esp32_i2s_cam_get_frmival(const struct device *dev, struct video_frmival *frmival)
{
	const struct video_esp32_i2s_cam_config *cfg = dev->config;
	int ret;

	ret = video_esp32_i2s_cam_select_link_freq(dev);
	if (ret < 0) {
		return ret;
	}

	return video_get_frmival(cfg->source_dev, frmival);
}

static int video_esp32_i2s_cam_set_selection(const struct device *dev,
					     struct video_selection *sel)
{
	const struct video_esp32_i2s_cam_config *cfg = dev->config;
	struct video_esp32_i2s_cam_data *data = dev->data;

	if (data->is_streaming) {
		return -EBUSY;
	}

	return video_set_selection(cfg->source_dev, sel);
}

static int video_esp32_i2s_cam_get_selection(const struct device *dev,
					     struct video_selection *sel)
{
	const struct video_esp32_i2s_cam_config *cfg = dev->config;

	return video_get_selection(cfg->source_dev, sel);
}

static int video_esp32_i2s_cam_enum_frmival(const struct device *dev,
					    struct video_frmival_enum *fie)
{
	const struct video_esp32_i2s_cam_config *cfg = dev->config;
	int ret;

	ret = video_esp32_i2s_cam_select_link_freq(dev);
	if (ret < 0) {
		return ret;
	}

	return video_enum_frmival(cfg->source_dev, fie);
}

#ifdef CONFIG_POLL
static int video_esp32_i2s_cam_set_signal(const struct device *dev, struct k_poll_signal *sig)
{
	struct video_esp32_i2s_cam_data *data = dev->data;

	data->signal_out = sig;

	return 0;
}
#endif

static void video_esp32_i2s_cam_hw_init(const struct device *dev)
{
	const struct video_esp32_i2s_cam_config *cfg = dev->config;
	struct video_esp32_i2s_cam_data *data = dev->data;
	i2s_dev_t *hw = cfg->hw;

	/* Circular descriptor list, one descriptor per chunk, sized when streaming starts */
	for (int i = 0; i < VIDEO_ESP32_I2S_CAM_CHUNK_NUM; i++) {
		lldesc_t *desc = &data->dma_desc[i];

		desc->owner = 1;
		desc->buf = (uint8_t *)data->dma_buf[i];
		desc->empty = (uint32_t)&data->dma_desc[(i + 1) % VIDEO_ESP32_I2S_CAM_CHUNK_NUM];
	}

	/* The module clock enabled through clock control is gated again by the controller */
	i2s_ll_enable_core_clock(hw, true);
	i2s_ll_rx_reset(hw);
	i2s_ll_rx_reset_fifo(hw);

	/* Camera slave mode: PCLK on WS, data sampled while VSYNC, HSYNC and HENABLE are high */
	i2s_ll_rx_set_slave_mod(hw, true);
	i2s_ll_rx_enable_right_first(hw, false);
	i2s_ll_rx_enable_msb_right(hw, false);
	i2s_ll_rx_enable_msb_shift(hw, false);
	hw->conf.rx_mono = 0;
	hw->conf.rx_short_sync = 0;
	i2s_ll_enable_lcd(hw, true);
	i2s_ll_enable_camera(hw, true);
	i2s_ll_set_raw_mclk_div(hw, 2, 0, 0);

	i2s_ll_enable_dma(hw, true);
	i2s_ll_dma_enable_owner_check(hw, false);
	hw->fifo_conf.rx_fifo_mod = VIDEO_ESP32_I2S_CAM_FIFO_MOD;
	i2s_ll_rx_force_enable_fifo_mod(hw, true);
	hw->conf_chan.rx_chan_mod = 1;
	hw->sample_rate_conf.rx_bits_mod = 0;
	hw->timing.val = 0;
	hw->timing.rx_dsync_sw = 1;

	/* Frames are delimited by restarting the DMA on VSYNC, lines are gated by HREF */
	esp_rom_gpio_connect_in_signal(GPIO_MATRIX_CONST_ONE_INPUT, I2S0I_V_SYNC_IDX, false);
	esp_rom_gpio_connect_in_signal(GPIO_MATRIX_CONST_ONE_INPUT, I2S0I_H_SYNC_IDX, false);

	i2s_ll_clear_intr_status(hw, I2S_LL_EVENT_RX_EOF);
	i2s_ll_rx_enable_intr(hw);
}

static int video_esp32_i2s_cam_init(const struct device *dev)
{
	const struct video_esp32_i2s_cam_config *cfg = dev->config;
	struct video_esp32_i2s_cam_data *data = dev->data;
	int ret;

	data->dev = dev;
	k_fifo_init(&data->fifo_in);
	k_fifo_init(&data->fifo_out);

	if (!esp_ptr_dma_capable(data->dma_buf) || !esp_ptr_dma_capable(data->dma_desc)) {
		LOG_ERR("DMA buffers are not in DMA capable memory");
		return -ENOMEM;
	}

	if (!device_is_ready(cfg->clock_dev) || !gpio_is_ready_dt(&cfg->vsync_gpio)) {
		LOG_ERR("Dependencies not ready");
		return -ENODEV;
	}

	ret = clock_control_on(cfg->clock_dev, cfg->clock_subsys);
	if (ret < 0) {
		LOG_ERR("Failed to enable I2S clock (%d)", ret);
		return ret;
	}

	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("Failed to apply pinctrl state (%d)", ret);
		return ret;
	}

	video_esp32_i2s_cam_hw_init(dev);

	ret = esp_intr_alloc(cfg->irq_source,
			     ESP_PRIO_TO_FLAGS(cfg->irq_priority) |
				     ESP_INT_FLAGS_CHECK(cfg->irq_flags),
			     video_esp32_i2s_cam_isr, (void *)dev, NULL);
	if (ret != 0) {
		LOG_ERR("Failed to allocate interrupt (%d)", ret);
		return ret;
	}

	ret = gpio_pin_configure_dt(&cfg->vsync_gpio, GPIO_INPUT);
	if (ret < 0) {
		return ret;
	}

	gpio_init_callback(&data->vsync_cb, video_esp32_i2s_cam_vsync_handler,
			   BIT(cfg->vsync_gpio.pin));

	return gpio_add_callback_dt(&cfg->vsync_gpio, &data->vsync_cb);
}

static DEVICE_API(video, video_esp32_i2s_cam_api) = {
	.set_format = video_esp32_i2s_cam_set_fmt,
	.get_format = video_esp32_i2s_cam_get_fmt,
	.set_stream = video_esp32_i2s_cam_set_stream,
	.get_caps = video_esp32_i2s_cam_get_caps,
	.enqueue = video_esp32_i2s_cam_enqueue,
	.dequeue = video_esp32_i2s_cam_dequeue,
	.flush = video_esp32_i2s_cam_flush,
	.set_frmival = video_esp32_i2s_cam_set_frmival,
	.get_frmival = video_esp32_i2s_cam_get_frmival,
	.enum_frmival = video_esp32_i2s_cam_enum_frmival,
	.set_selection = video_esp32_i2s_cam_set_selection,
	.get_selection = video_esp32_i2s_cam_get_selection,
#ifdef CONFIG_POLL
	.set_signal = video_esp32_i2s_cam_set_signal,
#endif
};

#define SOURCE_DEV(n) DEVICE_DT_GET(DT_NODE_REMOTE_DEVICE(DT_INST_ENDPOINT_BY_ID(n, 0, 0)))

PINCTRL_DT_INST_DEFINE(0);

static const struct video_esp32_i2s_cam_config video_esp32_i2s_cam_config_0 = {
	.hw = (i2s_dev_t *)DT_REG_ADDR(VIDEO_ESP32_I2S_CAM_I2S_NODE),
	.source_dev = SOURCE_DEV(0),
	.clock_dev = DEVICE_DT_GET(DT_CLOCKS_CTLR(VIDEO_ESP32_I2S_CAM_I2S_NODE)),
	.clock_subsys =
		(clock_control_subsys_t)DT_CLOCKS_CELL(VIDEO_ESP32_I2S_CAM_I2S_NODE, offset),
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(0),
	.vsync_gpio = GPIO_DT_SPEC_INST_GET(0, vsync_gpios),
	.irq_source = DT_IRQ_BY_NAME(VIDEO_ESP32_I2S_CAM_I2S_NODE, rx, irq),
	.irq_priority = DT_IRQ_BY_NAME(VIDEO_ESP32_I2S_CAM_I2S_NODE, rx, priority),
	.irq_flags = DT_IRQ_BY_NAME(VIDEO_ESP32_I2S_CAM_I2S_NODE, rx, flags),
};

static struct video_esp32_i2s_cam_data video_esp32_i2s_cam_data_0;

DEVICE_DT_INST_DEFINE(0, video_esp32_i2s_cam_init, NULL, &video_esp32_i2s_cam_data_0,
		      &video_esp32_i2s_cam_config_0, POST_KERNEL,
		      CONFIG_VIDEO_INIT_PRIORITY, &video_esp32_i2s_cam_api);

VIDEO_DEVICE_DEFINE(esp32_i2s_cam, DEVICE_DT_INST_GET(0), SOURCE_DEV(0));
