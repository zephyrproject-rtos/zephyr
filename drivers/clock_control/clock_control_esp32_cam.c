#define DT_DRV_COMPAT espressif_esp32_cam_clk

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(esp32_lcd_cam, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control.h>
#include <soc/lcd_cam_struct.h>
#include <soc/lcd_cam_reg.h>

#define ESP32_CAM_PLL_F160M     160000000UL
#define ESP32_CAM_PLL_F160M_SEL 3
#define ESP32_CAM_CLK_OFF_SEL   0

struct clock_control_esp32_cam_config {
	const struct pinctrl_dev_config *pcfg;
	const struct device *clk_dev;
	struct device *clk_subsys;
	uint32_t cam_clk;
	uint8_t clock_sel;
};

static int enable_pheripheral_clock(const struct device *dev)
{
	const struct clock_control_esp32_cam_config *cfg = dev->config;
	int ret = 0;

	/* Enable peripheral */
	if (!device_is_ready(cfg->clk_dev)) {
		return -ENODEV;
	}

	return clock_control_on(cfg->clk_dev, cfg->clk_subsys);
}

static int set_camera_clock(uint32_t cam_clk)
{
	int ret = 0;

	if (0 == cam_clk) {
		LCD_CAM.cam_ctrl.cam_clk_sel =
			ESP32_CAM_CLK_OFF_SEL;
		LOG_DBG("Disabled CAM_CLK");
		return -EINVAL;
	}

	if (ESP32_CAM_PLL_F160M % cam_clk) {
		LOG_WRN("MCLK is not a devider of 160MHz");
	}

	LCD_CAM.cam_ctrl.cam_clk_sel = ESP32_CAM_PLL_F160M_SEL;
	LCD_CAM.cam_ctrl.cam_clkm_div_num = ESP32_CAM_PLL_F160M / cam_clk;
	LCD_CAM.cam_ctrl.cam_clkm_div_b = 0;
	LCD_CAM.cam_ctrl.cam_clkm_div_a = 0;
	LOG_DBG("MCLK set to %ld", ESP32_CAM_PLL_F160M / LCD_CAM.cam_ctrl.cam_clkm_div_num);

	return ret;
}

static int clock_control_esp32_cam_init(const struct device *dev)
{
	const struct clock_control_esp32_cam_config *cfg = dev->config;
	int ret = 0;

	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("video pinctrl setup failed (%d)", ret);
		return ret;
	}

	ret = enable_pheripheral_clock(dev);
	if (ret < 0) {
		LOG_ERR("Failed to enable peripheral clock");
		return ret;
	}

	ret = set_camera_clock(cfg->cam_clk);
	if (ret < 0) {
		LOG_ERR("Failed to set camera clock");
		return ret;
	}

	LOG_DBG("cam clock initialized");

	return 0;
}

PINCTRL_DT_INST_DEFINE(0);

static const struct clock_control_esp32_cam_config clock_control_esp32_cam_config = {
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(0),
	.clk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(0)),
	.clk_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(0, offset),
	.cam_clk = DT_INST_PROP_OR(0, cam_clk, 0),
};

DEVICE_DT_INST_DEFINE(0, &clock_control_esp32_cam_init, NULL, NULL, &clock_control_esp32_cam_config,
		      PRE_KERNEL_1, CONFIG_CLOCK_CONTROL_INIT_PRIORITY, NULL);
