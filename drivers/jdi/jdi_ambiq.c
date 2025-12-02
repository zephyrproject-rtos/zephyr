/*
 * Copyright (c) 2025 Ambiq Micro Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ambiq_jdi

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/jdi.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

#include <am_mcu_apollo.h>
#include <nema_dc_regs.h>
#include <nema_dc_hal.h>
#include <nema_dc_jdi.h>
#include <nema_dc_intern.h>
#include <nema_dc.h>

#define LAYER_ACTIVE   1
#define LAYER_INACTIVE 0

LOG_MODULE_REGISTER(jdi_ambiq, CONFIG_JDI_LOG_LEVEL);

struct jdi_ambiq_config {
	const struct pinctrl_dev_config *pcfg;
	void (*irq_config_func)(const struct device *dev);
};

struct jdi_ambiq_data {
	uint32_t vck_gck_max_frequency;
	uint32_t hck_bck_max_frequency;

	nemadc_layer_t dc_layer;
	nemadc_initial_config_t dc_config;
};

static int jdi_ambiq_attach(const struct device *dev, const struct jdi_device *jdev)
{
	struct jdi_ambiq_data *data = dev->data;

	/* Validate input parameters */
	if (jdev == NULL) {
		LOG_ERR("Invalid JDI device pointer");
		return -EINVAL;
	}

	/* Configure input pixel format */
	switch (jdev->input_pixfmt) {
	case JDI_PIXFMT_RGBA5551:
		data->dc_layer.format = NEMADC_RGBA5551;
		break;

	case JDI_PIXFMT_ABGR8888:
		data->dc_layer.format = NEMADC_ABGR8888;
		break;

	case JDI_PIXFMT_BGR888:
		data->dc_layer.format = NEMADC_BGR24;
		break;

	case JDI_PIXFMT_RGB332:
		data->dc_layer.format = NEMADC_RGB332;
		break;

	case JDI_PIXFMT_RGB565:
		data->dc_layer.format = NEMADC_RGB565;
		break;

	case JDI_PIXFMT_BGRA8888:
		data->dc_layer.format = NEMADC_BGRA8888;
		break;

	case JDI_PIXFMT_L8:
		data->dc_layer.format = NEMADC_L8;
		break;

	case JDI_PIXFMT_BGRA4444:
		data->dc_layer.format = NEMADC_BGRA4444;
		break;

	case JDI_PIXFMT_RGB888:
		data->dc_layer.format = NEMADC_RGB24;
		break;

	case JDI_PIXFMT_ABGR4444:
		data->dc_layer.format = NEMADC_ABGR4444;
		break;

	case JDI_PIXFMT_RGBA8888:
		data->dc_layer.format = NEMADC_RGBA8888;
		break;

	case JDI_PIXFMT_ARGB8888:
		data->dc_layer.format = NEMADC_ARGB8888;
		break;

	case JDI_PIXFMT_BGRA5551:
		data->dc_layer.format = NEMADC_BGRA5551;
		break;

	case JDI_PIXFMT_ARGB1555:
		data->dc_layer.format = NEMADC_ARGB1555;
		break;

	case JDI_PIXFMT_RGBA4444:
		data->dc_layer.format = NEMADC_RGBA4444;
		break;

	case JDI_PIXFMT_BGR565:
		data->dc_layer.format = NEMADC_BGR565;
		break;

	case JDI_PIXFMT_AL88:
		data->dc_layer.format = NEMADC_AL88;
		break;

	case JDI_PIXFMT_ARGB4444:
		data->dc_layer.format = NEMADC_ARGB4444;
		break;

	case JDI_PIXFMT_AL44:
		data->dc_layer.format = NEMADC_AL44;
		break;

	case JDI_PIXFMT_RGBA2222:
		data->dc_layer.format = NEMADC_RGBA2222;
		break;

	case JDI_PIXFMT_BGRA2222:
		data->dc_layer.format = NEMADC_BGRA2222;
		break;

	case JDI_PIXFMT_ARGB2222:
		data->dc_layer.format = NEMADC_ARGB2222;
		break;

	case JDI_PIXFMT_ABGR2222:
		data->dc_layer.format = NEMADC_ABGR2222;
		break;

	case JDI_PIXFMT_A8:
		data->dc_layer.format = NEMADC_A8;
		break;

	case JDI_PIXFMT_ABGR1555:
		data->dc_layer.format = NEMADC_ABGR1555;
		break;

	default:
		LOG_ERR("Unsupported pixel format: %d", jdev->input_pixfmt);
		return -ENOTSUP;
	}

	data->dc_config.eInterface = DISP_INTERFACE_JDI;
	data->dc_config.ui16ResX = jdev->width;
	data->dc_config.ui16ResY = jdev->height;

	/* Convert unit from Hz to MHz */
	data->dc_config.fHCKBCKMaxFreq = (float)data->hck_bck_max_frequency / 1000000;
	data->dc_config.fVCKGCKFFMaxFreq = (float)data->vck_gck_max_frequency / 1000000;

	nemadc_configure(&data->dc_config);

	/* Initialize layer configuration with optimized defaults */
	data->dc_layer.resx = data->dc_config.ui16ResX;
	data->dc_layer.resy = data->dc_config.ui16ResY;
	data->dc_layer.sizex = data->dc_config.ui16ResX;
	data->dc_layer.sizey = data->dc_config.ui16ResY;
	data->dc_layer.startx = 0;
	data->dc_layer.starty = 0;
	data->dc_layer.stride = nemadc_stride_size(data->dc_layer.format, data->dc_config.ui16ResX);
	data->dc_layer.buscfg = 0;
	data->dc_layer.blendmode = NEMADC_BL_SRC;
	data->dc_layer.alpha = 0xFF;
	data->dc_layer.flipx_en = 0;
	data->dc_layer.flipy_en = 0;
	data->dc_layer.extra_bits = 0;

	return 0;
}

static ssize_t jdi_ambiq_transfer(const struct device *dev, const struct jdi_msg *msg)
{
	struct jdi_ambiq_data *data = dev->data;

	if (!msg->h) {
		LOG_ERR("Invalid parameters.");
		return -EINVAL;
	}

	if (msg->w != data->dc_config.ui16ResX) {
		data->dc_config.ui16ResX = msg->w;
		data->dc_layer.resx = data->dc_config.ui16ResX;
		data->dc_layer.sizex = data->dc_config.ui16ResX;
		data->dc_layer.stride =
			nemadc_stride_size(data->dc_layer.format, data->dc_config.ui16ResX);
	}

	/* Reset JDI used parameters before transfer frame */
	nemadc_reset_mip_parameters();

	if (msg->tx_len) {
		data->dc_layer.startx = msg->x;
		data->dc_layer.starty = msg->y;
		data->dc_layer.baseaddr_virt = (void *)msg->tx_buf;
		data->dc_layer.baseaddr_phys = (unsigned int)(data->dc_layer.baseaddr_virt);
		nemadc_mip_setup(LAYER_ACTIVE, &data->dc_layer, LAYER_INACTIVE, NULL,
				 LAYER_INACTIVE, NULL, LAYER_INACTIVE, NULL, 1, msg->y,
				 msg->y + msg->h);
		nemadc_transfer_frame_launch();
		nemadc_wait_vsync();
	}

	return 0;
}

/* JDI Driver API Structure */
static DEVICE_API(jdi, jdi_ambiq_driver_api) = {
	.attach = jdi_ambiq_attach,
	.transfer = jdi_ambiq_transfer,
};

static int jdi_ambiq_init(const struct device *dev)
{
	const struct jdi_ambiq_config *config = dev->config;
	int ret;

	LOG_DBG("JDI init");

	/* Apply default pinmux configuration */
	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("Failed to apply pinctrl state: %d", ret);
		return ret;
	}

	/* Enable display peripheral power */
	ret = am_hal_pwrctrl_periph_enable(AM_HAL_PWRCTRL_PERIPH_DISP);
	if (ret != AM_HAL_STATUS_SUCCESS) {
		LOG_ERR("Failed to enable display peripheral power: %d", ret);
		return -EIO;
	}

	/* Configure clock to 48MHz, the frequency is up to 192MHz */
	ret = am_hal_clkgen_control(AM_HAL_CLKGEN_CONTROL_DISPCLKSEL_HFRC48, NULL);
	if (ret != AM_HAL_STATUS_SUCCESS) {
		LOG_ERR("Failed to configure display clock: %d", ret);
		return -EIO;
	}

	ret = am_hal_clkgen_control(AM_HAL_CLKGEN_CONTROL_DCCLK_ENABLE, NULL);
	if (ret != AM_HAL_STATUS_SUCCESS) {
		LOG_ERR("Failed to enable DC clock: %d", ret);
		return -EIO;
	}

	/* Initialize NemaDC */
	ret = nemadc_init();
	if (ret != AM_HAL_STATUS_SUCCESS) {
		LOG_ERR("NemaDC initialization failed");
		return -EFAULT;
	}

	/* Enable global interrupts */
	am_hal_interrupt_master_enable();

	/* Configure interrupts */
	config->irq_config_func(dev);
	return 0;
}

/*
 * Ambiq DC interrupt service routine
 */
extern void am_disp_isr(void);

/* Device Definition Macros */
#define AMBIQ_JDI_DEVICE(n)                                                                        \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	static void disp_##n##_irq_config_func(const struct device *dev)                           \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), am_disp_isr,                \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}                                                                                          \
	static struct jdi_ambiq_data jdi_ambiq_data_##n = {                                        \
		.hck_bck_max_frequency = DT_INST_PROP_OR(n, hck_bck_max_freq, 758000),             \
		.vck_gck_max_frequency = DT_INST_PROP_OR(n, vck_gck_max_freq, 500000),             \
		.dc_config = {.ui32XRSTINTBDelay = DT_INST_PROP_OR(n, xrst_intb_delay, 1),         \
			      .ui32XRSTINTBWidth = DT_INST_PROP_OR(n, xrst_intb_width, 566),       \
			      .ui32VSTGSPDelay = DT_INST_PROP_OR(n, vst_gsp_delay, 73),            \
			      .ui32VSTGSPWidth = DT_INST_PROP_OR(n, vst_gsp_width, 576),           \
			      .ui32VCKGCKDelay = DT_INST_PROP_OR(n, vck_gck_delay, 217),           \
			      .ui32VCKGCKWidth = DT_INST_PROP_OR(n, vck_gck_width, 288),           \
			      .ui32VCKGCKClosingPulses =                                           \
				      DT_INST_PROP_OR(n, vck_gck_closing_pulses, 6),               \
			      .ui32HSTBSPDelay = DT_INST_PROP_OR(n, hst_bsp_delay, 2),             \
			      .ui32HSTBSPWidth = DT_INST_PROP_OR(n, hst_bsp_width, 4),             \
			      .ui32HCKBCKDataStart = DT_INST_PROP_OR(n, hck_bck_data_start, 1),    \
			      .ui32ENBGENDelay = DT_INST_PROP_OR(n, enb_gen_delay, 99),            \
			      .ui32ENBGENWidth = DT_INST_PROP_OR(n, enb_gen_width, 90)}};          \
	static const struct jdi_ambiq_config jdi_ambiq_config_##n = {                              \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		.irq_config_func = disp_##n##_irq_config_func,                                     \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, jdi_ambiq_init, NULL, &jdi_ambiq_data_##n, &jdi_ambiq_config_##n, \
			      POST_KERNEL, CONFIG_JDI_INIT_PRIORITY, &jdi_ambiq_driver_api);

DT_INST_FOREACH_STATUS_OKAY(AMBIQ_JDI_DEVICE)
