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
#include <zephyr/drivers/pwm.h>
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

BUILD_ASSERT(CONFIG_VIDEO_ESP32_I2S_CAM_INIT_PRIORITY < CONFIG_VIDEO_INIT_PRIORITY,
	     "XCLK must run before the image sensor is initialized");

/* One DMA descriptor per chunk, an RX EOF interrupt is raised for each chunk */
#define VIDEO_ESP32_I2S_CAM_CHUNK_SIZE  LLDESC_MAX_NUM_PER_DESC
#define VIDEO_ESP32_I2S_CAM_CHUNK_WORDS (VIDEO_ESP32_I2S_CAM_CHUNK_SIZE / sizeof(uint32_t))
#define VIDEO_ESP32_I2S_CAM_CHUNK_NUM   CONFIG_VIDEO_ESP32_I2S_CAM_DMA_CHUNKS

/* 32-bit single channel RX FIFO mode: one camera byte per word, in bits [23:16] */
#define VIDEO_ESP32_I2S_CAM_FIFO_MOD     3U
#define VIDEO_ESP32_I2S_CAM_SAMPLE_SHIFT 16U

#define VIDEO_ESP32_I2S_CAM_JPEG_MARKER 0xFFU
#define VIDEO_ESP32_I2S_CAM_JPEG_EOI    0xD9U

struct video_esp32_i2s_cam_config {
	i2s_dev_t *hw;
	const struct device *source_dev;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	const struct pinctrl_dev_config *pcfg;
	struct gpio_dt_spec vsync_gpio;
	struct pwm_dt_spec xclk;
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
	uint8_t chunk_idx;
	uint8_t prev_byte;
	bool frame_done;
	bool capturing;
	bool is_streaming;
	bool is_jpeg;
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
	uint8_t prev = data->prev_byte;
	uint32_t len;
	uint8_t *dst;

	if (vbuf == NULL || data->frame_done) {
		return;
	}

	len = MIN(VIDEO_ESP32_I2S_CAM_CHUNK_WORDS, data->frame_max - data->frame_len);
	dst = &vbuf->buffer[data->frame_len];

	for (uint32_t i = 0; i < len; i++) {
		uint8_t byte = (uint8_t)(src[i] >> VIDEO_ESP32_I2S_CAM_SAMPLE_SHIFT);

		dst[i] = byte;

		/* JPEG entropy data never contains an EOI marker, the first one ends the frame */
		if (data->is_jpeg && prev == VIDEO_ESP32_I2S_CAM_JPEG_MARKER &&
		    byte == VIDEO_ESP32_I2S_CAM_JPEG_EOI) {
			data->frame_len += i + 1U;
			data->frame_done = true;
			return;
		}

		prev = byte;
	}

	data->prev_byte = prev;
	data->frame_len += len;

	if (!data->is_jpeg && data->frame_len == data->frame_max) {
		data->frame_done = true;
	}
}

/* Copy all chunks completed by the DMA since the last call */
static void video_esp32_i2s_cam_drain(struct video_esp32_i2s_cam_data *data)
{
	const struct video_esp32_i2s_cam_config *cfg = data->dev->config;
	uint32_t eof_addr;
	uint32_t last;
	uint8_t idx;

	if ((i2s_ll_get_intr_status(cfg->hw) & I2S_LL_EVENT_RX_EOF) == 0U) {
		return;
	}

	i2s_ll_clear_intr_status(cfg->hw, I2S_LL_EVENT_RX_EOF);

	if (!data->capturing) {
		return;
	}

	/* The DMA may have completed several chunks before the interrupt was serviced */
	i2s_ll_rx_get_eof_des_addr(cfg->hw, &eof_addr);
	last = (eof_addr - (uint32_t)&data->dma_desc[0]) / sizeof(lldesc_t);
	if (last >= VIDEO_ESP32_I2S_CAM_CHUNK_NUM) {
		last = data->chunk_idx;
	}

	do {
		idx = data->chunk_idx;
		video_esp32_i2s_cam_copy(data, data->dma_buf[idx]);
		data->chunk_idx = (idx + 1U) % VIDEO_ESP32_I2S_CAM_CHUNK_NUM;
	} while (idx != last);
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
	data->frame_max = data->is_jpeg ? data->active_vbuf->size
					: MIN(data->active_vbuf->size, data->fmt.size);
	data->frame_done = false;
	data->prev_byte = 0U;
	data->chunk_idx = 0U;

	i2s_ll_rx_reset(hw);
	i2s_ll_rx_reset_fifo(hw);
	hw->lc_conf.ahbm_fifo_rst = 1;
	hw->lc_conf.ahbm_fifo_rst = 0;
	hw->lc_conf.ahbm_rst = 1;
	hw->lc_conf.ahbm_rst = 0;
	i2s_ll_clear_intr_status(hw, I2S_LL_EVENT_RX_EOF);

	i2s_ll_rx_set_eof_num(hw, VIDEO_ESP32_I2S_CAM_CHUNK_SIZE);
	i2s_ll_rx_start_link(hw, (uint32_t)&data->dma_desc[0]);
	i2s_ll_rx_start(hw);

	data->capturing = true;
}

static void video_esp32_i2s_cam_finish_frame(struct video_esp32_i2s_cam_data *data)
{
	struct video_buffer *vbuf = data->active_vbuf;

	/* The chunk being filled when VSYNC arrived holds the tail of the frame */
	video_esp32_i2s_cam_copy(data, data->dma_buf[data->chunk_idx]);

	if (!data->frame_done) {
		/* Keep the buffer for the next frame */
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

	data->fmt.type = type;
	ret = video_get_format(cfg->source_dev, &data->fmt);
	if (ret < 0) {
		return ret;
	}

	ret = video_estimate_fmt_size(&data->fmt);
	if (ret < 0) {
		return ret;
	}

	data->is_jpeg = data->fmt.pixelformat == VIDEO_PIX_FMT_JPEG;

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

	return video_estimate_fmt_size(fmt);
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

static int video_esp32_i2s_cam_set_frmival(const struct device *dev, struct video_frmival *frmival)
{
	const struct video_esp32_i2s_cam_config *cfg = dev->config;

	return video_set_frmival(cfg->source_dev, frmival);
}

static int video_esp32_i2s_cam_get_frmival(const struct device *dev, struct video_frmival *frmival)
{
	const struct video_esp32_i2s_cam_config *cfg = dev->config;

	return video_get_frmival(cfg->source_dev, frmival);
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

	/* Circular descriptor list, one descriptor per chunk */
	for (int i = 0; i < VIDEO_ESP32_I2S_CAM_CHUNK_NUM; i++) {
		lldesc_t *desc = &data->dma_desc[i];

		desc->size = VIDEO_ESP32_I2S_CAM_CHUNK_SIZE;
		desc->length = VIDEO_ESP32_I2S_CAM_CHUNK_SIZE;
		desc->owner = 1;
		desc->buf = (uint8_t *)data->dma_buf[i];
		desc->empty = (uint32_t)&data->dma_desc[(i + 1) % VIDEO_ESP32_I2S_CAM_CHUNK_NUM];
	}

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

	if (!device_is_ready(cfg->clock_dev) || !gpio_is_ready_dt(&cfg->vsync_gpio) ||
	    !pwm_is_ready_dt(&cfg->xclk)) {
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

	ret = gpio_add_callback_dt(&cfg->vsync_gpio, &data->vsync_cb);
	if (ret < 0) {
		return ret;
	}

	/* The image sensor needs XCLK before it can be probed over SCCB */
	ret = pwm_set_dt(&cfg->xclk, cfg->xclk.period, cfg->xclk.period / 2U);
	if (ret < 0) {
		LOG_ERR("Failed to start XCLK (%d)", ret);
		return ret;
	}

	return 0;
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
#ifdef CONFIG_POLL
	.set_signal = video_esp32_i2s_cam_set_signal,
#endif
};

#define SOURCE_DEV(n) DEVICE_DT_GET(DT_NODE_REMOTE_DEVICE(DT_INST_ENDPOINT_BY_ID(n, 0, 0)))

PINCTRL_DT_INST_DEFINE(0);

static const struct video_esp32_i2s_cam_config video_esp32_i2s_cam_config_0 = {
	.hw = (i2s_dev_t *)DT_INST_REG_ADDR(0),
	.source_dev = SOURCE_DEV(0),
	.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(0)),
	.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(0, offset),
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(0),
	.vsync_gpio = GPIO_DT_SPEC_INST_GET(0, vsync_gpios),
	.xclk = PWM_DT_SPEC_INST_GET(0),
	.irq_source = DT_INST_IRQ_BY_IDX(0, 0, irq),
	.irq_priority = DT_INST_IRQ_BY_IDX(0, 0, priority),
	.irq_flags = DT_INST_IRQ_BY_IDX(0, 0, flags),
};

static struct video_esp32_i2s_cam_data video_esp32_i2s_cam_data_0;

DEVICE_DT_INST_DEFINE(0, video_esp32_i2s_cam_init, NULL, &video_esp32_i2s_cam_data_0,
		      &video_esp32_i2s_cam_config_0, POST_KERNEL,
		      CONFIG_VIDEO_ESP32_I2S_CAM_INIT_PRIORITY, &video_esp32_i2s_cam_api);

VIDEO_DEVICE_DEFINE(esp32_i2s_cam, DEVICE_DT_INST_GET(0), SOURCE_DEV(0));
