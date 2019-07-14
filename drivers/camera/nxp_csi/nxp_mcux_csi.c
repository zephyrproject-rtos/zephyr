/*
 * Copyright (c) 2019, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <drivers/camera_drv.h>
#include <drivers/image_sensor.h>
#include "fsl_common.h"
#include <drivers/clock_control.h>
#include <fsl_cache.h>

#define LOG_LEVEL LOG_LEVEL_ERR
#include <logging/log.h>
LOG_MODULE_REGISTER(NXP_MCUX_CSI);

#define NXP_CSI_DBG

#undef LOG_ERR
#define LOG_ERR printk

#ifdef NXP_CSI_DBG
#undef LOG_INF
#define LOG_INF printk
#endif

#define CSI_CSICR1_INT_EN_MASK 0xFFFF0000U
#define CSI_CSICR3_INT_EN_MASK 0x000000FFU
#define CSI_CSICR18_INT_EN_MASK 0x0000FF00U

#define CSI_FB_DEFAULT_WIDTH 480
#define CSI_FB_DEFAULT_HEIGHT 272

#define CSI_FB_DEFAULT_PIXEL_FORMAT PIXEL_FORMAT_RGB_565

#define CSI_FB_MAX_NUM 8

struct priv_csi_config {
	CSI_Type *base;
	int irq_num;
	u32_t polarity;
	bool sensor_vsync;
	/* In CCIR656 progressive mode,
	 * set true to use external VSYNC signal, set false
	 * to use internal VSYNC signal decoded from SOF.
	 */
};

enum priv_csi_polarity_flags {
	CSI_HSYNC_LOW = 0U,
	/* HSYNC is active low. */
	CSI_HSYNC_HIGH = CSI_CSICR1_HSYNC_POL_MASK,
	/* HSYNC is active high. */
	CSI_RISING_LATCH = CSI_CSICR1_REDGE_MASK,
	/* Pixel data latched at rising edge of pixel clock. */
	CSI_FALLING_LATCH = 0U,
	/* Pixel data latched at falling edge of pixel clock. */
	CSI_VSYNC_HIGH = 0U,
	/* VSYNC is active high. */
	CSI_VSYNC_LOW = CSI_CSICR1_SOF_POL_MASK,
	/* VSYNC is active low. */
};

enum priv_csi_fifo {
	CSI_RXFIFO = (1U << 0U),
	/* RXFIFO. */
	CSI_STATFIFO = (1U << 1U),
	/* STAT FIFO. */
	CSI_ALLFIFO = (CSI_RXFIFO | CSI_STATFIFO)
};

enum mcux_csi_status {
	MCUX_CSI_INIT,
	MCUX_CSI_POWER,
	MCUX_CSI_READY,
	MCUX_CSI_RUNNING,
	MCUX_CSI_PAUSE
};

struct mcux_csi_fb {
	void *fb[CSI_FB_MAX_NUM];
	u8_t fb_from[CSI_FB_MAX_NUM];
	struct k_mutex sw_hmutex;
	struct k_mutex sw_tmutex;
	u8_t sw_head;
	u8_t sw_tail;
	u8_t hw_head;
	u8_t hw_tail;
};

struct mcux_csi_priv {
	struct mcux_csi_fb csi_fb;
	struct priv_csi_config hw_cfg;
	struct device *clk_dev;
	clock_control_subsys_t clock_sys;
	u32_t mclk;
	enum mcux_csi_status status;
};

static void mcux_csi_config_irq(struct camera_driver_data *data);

static inline void csi_irq_configure(CSI_Type *base, u32_t mask)
{
	base->CSICR1 |= (mask & CSI_CSICR1_INT_EN_MASK);
	base->CSICR3 |= (mask & CSI_CSICR3_INT_EN_MASK);
	base->CSICR18 |= ((mask & CSI_CSICR18_INT_EN_MASK) >> 6U);
}

static inline void csi_hw_fifo_dma_enable(CSI_Type *base,
	enum priv_csi_fifo fifo, bool enable)
{
	u32_t cr3 = 0U;

	if ((u32_t)fifo & (u32_t)CSI_RXFIFO) {
		cr3 |= CSI_CSICR3_DMA_REQ_EN_RFF_MASK;
	}

	if ((u32_t)fifo & (u32_t)CSI_STATFIFO) {
		cr3 |= CSI_CSICR3_DMA_REQ_EN_SFF_MASK;
	}

	if (enable) {
		base->CSICR3 |= cr3;
	} else {
		base->CSICR3 &= ~cr3;
	}
}

static int csi_start(struct camera_driver_data *data)
{
	struct mcux_csi_priv *priv = camera_data_priv(data);
	u32_t cr3 = 0U;
	CSI_Type *base = priv->hw_cfg.base;

	base->CSICR18 =
		(base->CSICR18 & ~CSI_CSICR18_MASK_OPTION_MASK) |
		CSI_CSICR18_MASK_OPTION(3) |
		CSI_CSICR18_BASEADDR_SWITCH_SEL_MASK |
		CSI_CSICR18_BASEADDR_SWITCH_EN_MASK;

	if (data->mode == CAMERA_CAPTURE_MODE) {
		base->CSIDMASA_FB1 = (u32_t)priv->csi_fb.fb[0];
		base->CSIDMASA_FB2 = (u32_t)priv->csi_fb.fb[0];
	} else {
		base->CSIDMASA_FB1 = (u32_t)priv->csi_fb.fb[0];
		priv->csi_fb.hw_tail++;
		if (priv->csi_fb.hw_tail !=
			priv->csi_fb.sw_tail) {
			base->CSIDMASA_FB2 = (u32_t)priv->csi_fb.fb[1];
			priv->csi_fb.hw_tail++;
		} else {
			base->CSIDMASA_FB2 = (u32_t)priv->csi_fb.fb[0];
		}
	}

	/* After reflash DMA, the CSI saves frame to frame buffer 0. */
	cr3 |= CSI_CSICR3_DMA_REFLASH_RFF_MASK;
	base->CSICR3 |= cr3;
	while (base->CSICR3 & cr3) {
		;
	}

	/*Enable isr*/
	if (data->mode == CAMERA_CAPTURE_MODE) {
		csi_irq_configure(base, CSI_CSICR1_FB1_DMA_DONE_INTEN_MASK);
	} else {
		csi_irq_configure(base, CSI_CSICR1_FB1_DMA_DONE_INTEN_MASK |
			CSI_CSICR1_FB2_DMA_DONE_INTEN_MASK);
	}

	irq_enable(priv->hw_cfg.irq_num);

	/*Start capture*/
	csi_hw_fifo_dma_enable(base, CSI_RXFIFO, true);

	base->CSICR18 |= CSI_CSICR18_CSI_ENABLE_MASK;

	return 0;
}

static inline void csi_hw_stop(CSI_Type *base)
{
	base->CSICR18 &= ~CSI_CSICR18_CSI_ENABLE_MASK;
	csi_hw_fifo_dma_enable(base, CSI_RXFIFO, false);
}

static int mcux_csi_start(struct device *cam_dev,
		enum camera_mode mode, void **bufs,
		u8_t buf_num,
		camera_capture_cb cb)
{
	struct camera_driver_data *data = cam_dev->driver_data;
	struct mcux_csi_priv *priv = camera_data_priv(data);
	u8_t i;
	int ret;

	if (buf_num < 1 || buf_num > (CSI_FB_MAX_NUM - 1)) {
		return -EINVAL;
	}

	for (i = 0; i < buf_num; i++) {
		priv->csi_fb.fb[i] = bufs[i];
	}
	priv->csi_fb.sw_head = 0;
	priv->csi_fb.sw_tail = buf_num;
	priv->csi_fb.hw_head = 0;
	priv->csi_fb.hw_tail = 0;
	data->mode = mode;
	data->customer_cb = cb;

	ret = csi_start(data);
	if (!ret) {
		priv->status = MCUX_CSI_RUNNING;
	}

	return ret;
}

static int mcux_csi_resume(struct device *cam_dev)
{
	struct camera_driver_data *data = cam_dev->driver_data;
	struct mcux_csi_priv *priv = camera_data_priv(data);
	int ret;

	ret = csi_start(data);
	if (!ret) {
		priv->status = MCUX_CSI_RUNNING;
	}

	return ret;
}

static int mcux_csi_acquire_fb(struct device *dev,
	void **fb, s32_t timeout)
{
	struct camera_driver_data *data = dev->driver_data;
	struct mcux_csi_priv *priv = camera_data_priv(data);

	if (data->mode == CAMERA_CAPTURE_MODE) {
		*fb = priv->csi_fb.fb[priv->csi_fb.sw_head];
		return 0;
	}

	k_mutex_lock(&priv->csi_fb.sw_hmutex, timeout);

acquire_again:
	if (priv->csi_fb.sw_head ==
		priv->csi_fb.hw_head) {
		if (timeout == K_NO_WAIT) {
			*fb = 0;
			k_mutex_unlock(&priv->csi_fb.sw_hmutex);
			return -ENOBUFS;
		}

		k_sleep(1);
		timeout--;
		goto acquire_again;
	}

	*fb = priv->csi_fb.fb[priv->csi_fb.sw_head];

	if (priv->csi_fb.sw_head ==
		(CSI_FB_MAX_NUM - 1)) {
		priv->csi_fb.sw_head = 0;
	} else {
		priv->csi_fb.sw_head++;
	}

	k_mutex_unlock(&priv->csi_fb.sw_hmutex);

	return 0;
}

static int mcux_csi_release_fb(struct device *dev, void *fb)
{
	struct camera_driver_data *data = dev->driver_data;
	struct mcux_csi_priv *priv = camera_data_priv(data);

	k_mutex_lock(&priv->csi_fb.sw_tmutex, K_FOREVER);
	if ((priv->csi_fb.sw_tail + 1) ==
		priv->csi_fb.sw_head ||
		(priv->csi_fb.sw_tail ==
		(CSI_FB_MAX_NUM - 1) &&
		priv->csi_fb.sw_head == 0)) {
		k_mutex_unlock(&priv->csi_fb.sw_tmutex);
		return -ENOSPC;
	}

	priv->csi_fb.fb[priv->csi_fb.sw_tail] = fb;

	if (priv->csi_fb.sw_tail ==
		(CSI_FB_MAX_NUM - 1)) {
		priv->csi_fb.sw_tail = 0;
	} else {
		priv->csi_fb.sw_tail++;
	}

	k_mutex_unlock(&priv->csi_fb.sw_tmutex);

	if (priv->status == MCUX_CSI_PAUSE) {
		mcux_csi_resume(dev);
	}

	return 0;
}

static void mcux_csi_isr(void *arg)
{
	struct device *dev = (struct device *)arg;
	struct camera_driver_data *data = dev->driver_data;
	struct mcux_csi_priv *priv = camera_data_priv(data);
	CSI_Type *base = priv->hw_cfg.base;
	u32_t csisr = base->CSISR;
	void *fb;

	/* Clear the error flags. */
	base->CSISR = csisr;

	if (data->mode == CAMERA_PREVIEW_MODE) {
		if (csisr & CSI_CSISR_DMA_TSF_DONE_FB1_MASK) {
			fb = (void *)base->CSIDMASA_FB1;
			priv->csi_fb.fb_from[priv->csi_fb.hw_head] = 1;
			if (priv->csi_fb.hw_head <
				(CSI_FB_MAX_NUM - 1)) {
				priv->csi_fb.hw_head++;
			} else {
				priv->csi_fb.hw_head = 0;
			}

			base->CSIDMASA_FB1 =
				(u32_t)priv->csi_fb.fb[priv->csi_fb.hw_tail];

			if (priv->csi_fb.hw_tail !=
				priv->csi_fb.sw_tail) {
				if (priv->csi_fb.hw_tail <
					(CSI_FB_MAX_NUM - 1)) {
					priv->csi_fb.hw_tail++;
				} else {
					priv->csi_fb.hw_tail = 0;
				}
			} else {
				csi_hw_stop(base);
				priv->status = MCUX_CSI_PAUSE;
				LOG_ERR("FB1 stop\r\n");
			}
			if (data->customer_cb) {
				data->customer_cb(fb,
					data->fb_attr.width,
					data->fb_attr.height,
					data->fb_attr.bpp);
			}
		}

		if (csisr & CSI_CSISR_DMA_TSF_DONE_FB2_MASK) {
			fb = (void *)base->CSIDMASA_FB2;
			priv->csi_fb.fb_from[priv->csi_fb.hw_head] = 2;
			if (priv->csi_fb.hw_head <
				(CSI_FB_MAX_NUM - 1)) {
				priv->csi_fb.hw_head++;
			} else {
				priv->csi_fb.hw_head = 0;
			}

			base->CSIDMASA_FB2 =
				(u32_t)priv->csi_fb.fb[priv->csi_fb.hw_tail];

			if (priv->csi_fb.hw_tail !=
				priv->csi_fb.sw_tail) {
				if (priv->csi_fb.hw_tail <
					(CSI_FB_MAX_NUM - 1)) {
					priv->csi_fb.hw_tail++;
				} else {
					priv->csi_fb.hw_tail = 0;
				}
			} else {
				csi_hw_stop(base);
				priv->status = MCUX_CSI_PAUSE;
				LOG_ERR("FB2 stop\r\n");
			}
			if (data->customer_cb) {
				data->customer_cb(fb,
					data->fb_attr.width,
					data->fb_attr.height,
					data->fb_attr.bpp);
			}
		}

		return;
	}

	fb = (void *)base->CSIDMASA_FB1;
	/*Stop CSI*/
	base->CSICR18 &= ~CSI_CSICR18_CSI_ENABLE_MASK;
	base->CSICR3 &= ~CSI_CSICR3_DMA_REQ_EN_RFF_MASK;

	priv->status = MCUX_CSI_PAUSE;

	if (data->customer_cb) {
		data->customer_cb(fb,
			data->fb_attr.width,
			data->fb_attr.height,
			data->fb_attr.bpp);
	}
}

static void csi_hw_clear_fifo(CSI_Type *base, enum priv_csi_fifo fifo)
{
	u32_t cr1;
	u32_t mask = 0U;

	/* The FIFO could only be cleared when CSICR1[FCC] = 0,
	 * so first clear the FCC.
	 */
	cr1 = base->CSICR1;
	base->CSICR1 = (cr1 & ~CSI_CSICR1_FCC_MASK);

	if ((u32_t)fifo & (u32_t)CSI_RXFIFO) {
		mask |= CSI_CSICR1_CLR_RXFIFO_MASK;
	}

	if ((u32_t)fifo & (u32_t)CSI_STATFIFO) {
		mask |= CSI_CSICR1_CLR_STATFIFO_MASK;
	}

	base->CSICR1 = (cr1 & ~CSI_CSICR1_FCC_MASK) | mask;

	/* Wait clear completed. */
	while (base->CSICR1 & mask) {
		;
	}

	/* Recover the FCC. */
	base->CSICR1 = cr1;
}

static void csi_hw_reflash_fifo(CSI_Type *base, enum priv_csi_fifo fifo)
{
	u32_t cr3 = 0U;

	if ((u32_t)fifo & (u32_t)CSI_RXFIFO) {
		cr3 |= CSI_CSICR3_DMA_REFLASH_RFF_MASK;
	}

	if ((u32_t)fifo & (u32_t)CSI_STATFIFO) {
		cr3 |= CSI_CSICR3_DMA_REFLASH_SFF_MASK;
	}

	base->CSICR3 |= cr3;

	/* Wait clear completed. */
	while (base->CSICR3 & cr3) {
		;
	}
}

static void csi_hw_reset(CSI_Type *base)
{
	u32_t csisr;

	/* Disable transfer first. */
	csi_hw_stop(base);

	/* Disable DMA request. */
	base->CSICR3 = 0U;

	/* Reset the fame count. */
	base->CSICR3 |= CSI_CSICR3_FRMCNT_RST_MASK;
	while (base->CSICR3 & CSI_CSICR3_FRMCNT_RST_MASK) {
		;
	}

	/* Clear the RX FIFO. */
	csi_hw_clear_fifo(base, CSI_ALLFIFO);

	/* Reflash DMA. */
	csi_hw_reflash_fifo(base, CSI_ALLFIFO);

	/* Clear the status. */
	csisr = base->CSISR;
	base->CSISR = csisr;

	/* Set the control registers to default value. */
	base->CSICR1 = CSI_CSICR1_HSYNC_POL_MASK | CSI_CSICR1_EXT_VSYNC_MASK;
	base->CSICR2 = 0U;
	base->CSICR3 = 0U;

	base->CSICR18 = CSI_CSICR18_AHB_HPROT(0x0DU);
	base->CSIFBUF_PARA = 0U;
	base->CSIIMAG_PARA = 0U;
}

static int csi_hw_data_config(struct camera_driver_data *data)
{
	struct mcux_csi_priv *priv = camera_data_priv(data);
	u32_t reg;
	u32_t width_bytes;
	CSI_Type *base = priv->hw_cfg.base;

	width_bytes = data->fb_attr.width *
		data->fb_attr.bpp;

	csi_hw_reset(base);

	/*!< HSYNC, VSYNC, and PIXCLK signals are used. */
	reg = ((u32_t)CSI_CSICR1_GCLK_MODE(1U)) |
			priv->hw_cfg.polarity |
			CSI_CSICR1_FCC_MASK;

	if (priv->hw_cfg.sensor_vsync) {
		reg |= CSI_CSICR1_EXT_VSYNC_MASK;
	}

	base->CSICR1 = reg;

	/* Image parameter. */
	base->CSIIMAG_PARA =
		((u32_t)(width_bytes) <<
		CSI_CSIIMAG_PARA_IMAGE_WIDTH_SHIFT) |
		((u32_t)data->fb_attr.height <<
		CSI_CSIIMAG_PARA_IMAGE_HEIGHT_SHIFT);

	/* The CSI frame buffer bus is 8-byte width. */
	base->CSIFBUF_PARA = 0;

	/* Enable auto ECC. */
	base->CSICR3 |= CSI_CSICR3_ECC_AUTO_EN_MASK;

	if (!(width_bytes % (8 * 16))) {
		base->CSICR2 = CSI_CSICR2_DMA_BURST_TYPE_RFF(3U);
		base->CSICR3 =
			(base->CSICR3 & ~CSI_CSICR3_RxFF_LEVEL_MASK) |
			((2U << CSI_CSICR3_RxFF_LEVEL_SHIFT));
	} else if (!(width_bytes % (8 * 8))) {
		base->CSICR2 = CSI_CSICR2_DMA_BURST_TYPE_RFF(2U);
		base->CSICR3 =
			(base->CSICR3 & ~CSI_CSICR3_RxFF_LEVEL_MASK) |
			((1U << CSI_CSICR3_RxFF_LEVEL_SHIFT));
	} else {
		base->CSICR2 = CSI_CSICR2_DMA_BURST_TYPE_RFF(1U);
		base->CSICR3 =
			(base->CSICR3 & ~CSI_CSICR3_RxFF_LEVEL_MASK) |
			((0U << CSI_CSICR3_RxFF_LEVEL_SHIFT));
	}

	/*Reflash DMA*/
	base->CSICR3 |= CSI_CSICR3_DMA_REFLASH_RFF_MASK;

	/* Wait clear completed. */
	while (base->CSICR3 & CSI_CSICR3_DMA_REFLASH_RFF_MASK) {
		;
	}

	return 0;
}

static int mcux_csi_sensor_cfg(struct camera_driver_data *data)
{
	struct device *img_dev = data->sensor_dev;
	int ret;

	ret = img_sensor_set_framesize(img_dev,
		data->fb_attr.width,
		data->fb_attr.height);
	if (ret) {
		return ret;
	}

	ret = img_sensor_set_pixformat(img_dev,
		data->fb_attr.pixformat);
	if (ret) {
		return ret;
	}

	ret = img_sensor_configure(img_dev);

	return ret;
}

static int mcux_csi_config(struct device *cam_dev,
	struct camera_fb_cfg *fb_cfg)
{
	struct camera_driver_data *data = cam_dev->driver_data;
	struct mcux_csi_priv *priv = camera_data_priv(data);
	int ret;

	if (priv->status != MCUX_CSI_POWER) {
		LOG_ERR("CSI configuration on the fly not implemented\r\n");

		return -EACCES;
	}

	if (fb_cfg->cfg_mode == CAMERA_USER_CFG) {
		if (!(fb_cfg->fb_attr.pixformat &
			data->cap.pixformat_support)) {
			LOG_ERR("CSI pixel format 0x%08x not supported!\r\n",
				fb_cfg->fb_attr.pixformat);

			return -ENOTSUP;
		}

		if (fb_cfg->fb_attr.width >
			data->cap.width_max ||
			fb_cfg->fb_attr.height >
			data->cap.height_max) {
			LOG_ERR("CSI frame size exceeds!\r\n");

			return -ENOTSUP;
		}

		if (fb_cfg->fb_attr.pixformat !=
			PIXEL_FORMAT_RGB_565) {
			LOG_ERR("CSI other than RGB565 not implemented\r\n");

			return -ENOTSUP;
		}
	}

	camera_dev_configure(cam_dev, fb_cfg);

	ret = mcux_csi_sensor_cfg(data);
	if (ret) {
		return ret;
	}

	ret = csi_hw_data_config(data);
	if (ret) {
		return ret;
	}

	mcux_csi_config_irq(data);

	return 0;
}

static int  mcux_csi_power(struct device *cam_dev,
		bool power)
{
	struct camera_driver_data *data = cam_dev->driver_data;
	struct mcux_csi_priv *priv = camera_data_priv(data);
	struct device *img_dev = data->sensor_dev;
	int ret;
	struct img_sensor_capability sensor_cap;

	if (!img_dev) {
		LOG_ERR("CSI power, but CMOS sensor Not present!\r\n");

		return -ENODEV;
	}

	if (power) {
		if (priv->status != MCUX_CSI_INIT) {
			return 0;
		}

		CLOCK_EnableClock(kCLOCK_Csi);
		imxrt_csi_mclk_enable(true);
		k_sleep(1);

		csi_hw_reset(priv->hw_cfg.base);

		ret = z_impl_img_sensor_reset(img_dev);
		if (ret) {
			LOG_ERR("CMOS sensor reset failed with error: %d\r\n",
				ret);

			return ret;
		}

		z_impl_img_sensor_get_cap(img_dev, &sensor_cap);
		if (ret) {
			LOG_ERR("CMOS sensor get capability failed"
				" with error: %d\r\n",
				ret);

			return ret;
		}

		data->cap.pixformat_support =
			data->cap.pixformat_support &
			sensor_cap.pixformat_support;
		data->cap.width_max =
			sensor_cap.width_max;
		data->cap.height_max =
			sensor_cap.height_max;

		priv->status = MCUX_CSI_POWER;

		return 0;
	}

	CLOCK_DisableClock(kCLOCK_Csi);
	imxrt_csi_mclk_enable(false);

	priv->status = MCUX_CSI_INIT;

	return 0;
}

static int mcux_csi_reset(struct device *cam_dev)
{
	mcux_csi_power(cam_dev, false);

	k_sleep(1);
	mcux_csi_power(cam_dev, true);

	return 0;
}

static const struct camera_driver_api mcux_camera_api = {
	.camera_power_cb = mcux_csi_power,
	.camera_reset_cb = mcux_csi_reset,
	.camera_get_cap_cb = camera_dev_get_cap,
	.camera_configure_cb = mcux_csi_config,
	.camera_start_cb = mcux_csi_start,
	.camera_acquire_fb_cb = mcux_csi_acquire_fb,
	.camera_release_fb_cb = mcux_csi_release_fb,
};

static int mcux_csi_init(struct device *cam_dev)
{
	struct camera_driver_data *data;
	struct mcux_csi_priv *priv;
	struct device *img_dev;
	enum camera_id id;

#ifdef DT_INST_0_NXP_IMX_CSI_LABEL
#if (DT_INST_0_NXP_IMX_CSI_LABEL == CAMERA_PRIMARY_LABEL)
	id = CAMERA_PRIMARY_ID;
#elif (DT_INST_0_NXP_IMX_CSI_LABEL == CAMERA_SECONDARY_LABEL)
	id = CAMERA_SECONDARY_ID;
#else
#warning CSI LABEL should be "primary" or "secondary".
	id = CAMERA_PRIMARY_ID;
#endif
#else
	id = CAMERA_PRIMARY_ID;
#endif

	data = camera_drv_data_alloc(sizeof(struct mcux_csi_priv), id, true);
	cam_dev->driver_data = data;
	priv = camera_data_priv(data);

	priv->hw_cfg.base = (CSI_Type *)DT_INST_0_NXP_IMX_CSI_BASE_ADDRESS;
	priv->hw_cfg.irq_num = (int)DT_INST_0_NXP_IMX_CSI_IRQ_0;
	priv->hw_cfg.polarity = (CSI_HSYNC_HIGH |
						CSI_RISING_LATCH);
	priv->hw_cfg.sensor_vsync = true;

	k_mutex_init(&priv->csi_fb.sw_hmutex);
	k_mutex_init(&priv->csi_fb.sw_tmutex);

	data->cap.fb_alignment = 64;
	data->cap.pixformat_support =
		PIXEL_FORMAT_RGB_565 |
		PIXEL_FORMAT_RGB_888;

	data->fb_attr.width = CSI_FB_DEFAULT_WIDTH;
	data->fb_attr.height = CSI_FB_DEFAULT_HEIGHT;
	data->fb_attr.pixformat = CSI_FB_DEFAULT_PIXEL_FORMAT;
	if (data->fb_attr.pixformat ==
		PIXEL_FORMAT_RGB_888) {
		data->fb_attr.bpp = 3;
	} else if (data->fb_attr.pixformat ==
		PIXEL_FORMAT_ARGB_8888) {
		data->fb_attr.bpp = 4;
	} else if (data->fb_attr.pixformat ==
		PIXEL_FORMAT_RGB_565) {
		data->fb_attr.bpp = 2;
	} else {
		LOG_ERR("CSI does not support this pixel format %d\r\n",
			data->fb_attr.pixformat);
		return -EINVAL;
	}
	priv->status = MCUX_CSI_INIT;

	priv->clk_dev = device_get_binding(
				DT_INST_0_NXP_IMX_CSI_CLOCK_CONTROLLER);
	priv->clock_sys =
		(clock_control_subsys_t)DT_INST_0_NXP_IMX_CSI_CLOCK_NAME;

	CLOCK_SetDiv(kCLOCK_CsiDiv, 0);
	CLOCK_SetMux(kCLOCK_CsiMux, 0);
	CLOCK_EnableClock(kCLOCK_Csi);
	imxrt_csi_mclk_enable(true);

	if (clock_control_get_rate(priv->clk_dev, priv->clock_sys,
		&priv->mclk)) {
		return -EINVAL;
	}

	img_dev = img_sensor_scan(data->id);
	if (!img_dev) {
		LOG_ERR("CSI init No CMOS sensor present!\r\n");

		return -ENODEV;
	}

	data->sensor_dev = img_dev;

	/*Power off for power saving
	 * Until user powers on.
	 */
	CLOCK_DisableClock(kCLOCK_Csi);
	imxrt_csi_mclk_enable(false);

	return camera_dev_register(cam_dev);
}

DEVICE_AND_API_INIT(mcux_csi, "MCUX_CSI",
		&mcux_csi_init,
		0, 0,
		POST_KERNEL, CONFIG_CAMERA_INIT_PRIO,
		&mcux_camera_api);

static void mcux_csi_config_irq(struct camera_driver_data *data)
{
	IRQ_CONNECT(DT_INST_0_NXP_IMX_CSI_IRQ_0,
		0, mcux_csi_isr, DEVICE_GET(mcux_csi), 0);

	irq_enable(DT_INST_0_NXP_IMX_CSI_IRQ_0);
}
