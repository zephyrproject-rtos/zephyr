/*
 * Copyright (c) 2022 Grant Ramsay <grant.ramsay@hotmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT espressif_esp32_mdio

#include <soc.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/mdio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <esp_mac.h>
#include <hal/emac_hal.h>
#include <hal/emac_ll.h>
#include <soc/rtc.h>
#include <clk_ctrl_os.h>

LOG_MODULE_REGISTER(mdio_esp32, CONFIG_MDIO_LOG_LEVEL);

#define PHY_OPERATION_TIMEOUT_US 1000

struct mdio_esp32_dev_data {
	struct k_sem sem;
	emac_hal_context_t hal;
};

struct mdio_esp32_dev_config {
	const struct pinctrl_dev_config *pcfg;
};

static int mdio_transfer(const struct device *dev, uint8_t prtad, uint8_t regad,
			 bool write, uint16_t data_in, uint16_t *data_out)
{
	struct mdio_esp32_dev_data *const dev_data = dev->data;

	k_sem_take(&dev_data->sem, K_FOREVER);

	if (emac_ll_is_mii_busy(dev_data->hal.mac_regs)) {
		LOG_ERR("phy busy");
		k_sem_give(&dev_data->sem);
		return -EBUSY;
	}

	if (write) {
		emac_ll_set_phy_data(dev_data->hal.mac_regs, data_in);
	}
	emac_hal_set_phy_cmd(&dev_data->hal, prtad, regad, write);

	/* Poll until operation complete */
	bool success = false;

	for (uint32_t t_us = 0; t_us < PHY_OPERATION_TIMEOUT_US; t_us += 100) {
		k_sleep(K_USEC(100));
		if (!emac_ll_is_mii_busy(dev_data->hal.mac_regs)) {
			success = true;
			break;
		}
	}
	if (!success) {
		LOG_ERR("phy timeout");
		k_sem_give(&dev_data->sem);
		return -ETIMEDOUT;
	}

	if (!write && data_out != NULL) {
		*data_out = emac_ll_get_phy_data(dev_data->hal.mac_regs);
	}

	k_sem_give(&dev_data->sem);

	return 0;
}

static int mdio_esp32_read(const struct device *dev, uint8_t prtad, uint8_t regad,
			 uint16_t *data)
{
	return mdio_transfer(dev, prtad, regad, false, 0, data);

}

static int mdio_esp32_write(const struct device *dev, uint8_t prtad,
			  uint8_t regad, uint16_t data)
{
	return mdio_transfer(dev, prtad, regad, true, data, NULL);
}

#if DT_INST_NODE_HAS_PROP(0, ref_clk_output_gpios)
static int emac_config_apll_clock(void)
{
	uint32_t expt_freq = MHZ(50);
	uint32_t real_freq = 0;
	esp_err_t ret = periph_rtc_apll_freq_set(expt_freq, &real_freq);

	if (ret == ESP_ERR_INVALID_ARG) {
		LOG_ERR("Set APLL clock coefficients failed");
		return -EIO;
	}

	if (ret == ESP_ERR_INVALID_STATE) {
		LOG_INF("APLL is occupied already, it is working at %d Hz", real_freq);
	}

	/* If the difference of real APLL frequency
	 * is not within 50 ppm, i.e. 2500 Hz,
	 * the APLL is unavailable
	 */
	if (abs((int)real_freq - (int)expt_freq) > 2500) {
		LOG_ERR("The APLL is working at an unusable frequency");
		return -EIO;
	}

	return 0;
}
#endif

static int mdio_esp32_initialize(const struct device *dev)
{
	const struct mdio_esp32_dev_config *const cfg = dev->config;
	struct mdio_esp32_dev_data *const dev_data = dev->data;
	int res;

	k_sem_init(&dev_data->sem, 1, 1);

	res = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (res != 0) {
		goto err;
	}

	const struct device *clock_dev =
		DEVICE_DT_GET(DT_CLOCKS_CTLR(DT_NODELABEL(mdio)));
	clock_control_subsys_t clock_subsys =
		(clock_control_subsys_t)DT_CLOCKS_CELL(DT_NODELABEL(mdio), offset);

	/* clock is shared, so do not bail out if already enabled */
	res = clock_control_on(clock_dev, clock_subsys);
	if (res < 0 && res != -EALREADY) {
		goto err;
	}

	/* Only the mac registers are required for MDIO */
	dev_data->hal.mac_regs = &EMAC_MAC;

#if DT_INST_NODE_HAS_PROP(0, ref_clk_output_gpios)
	emac_hal_init(&dev_data->hal, NULL, NULL, NULL);
	emac_hal_iomux_init_rmii();
	BUILD_ASSERT(DT_INST_GPIO_PIN(0, ref_clk_output_gpios) == 0 ||
	  DT_INST_GPIO_PIN(0, ref_clk_output_gpios) == 16 ||
		DT_INST_GPIO_PIN(0, ref_clk_output_gpios) == 17,
		"Only GPIO0/16/17 are allowed as a GPIO REF_CLK source!");
	int ref_clk_gpio = DT_INST_GPIO_PIN(0, ref_clk_output_gpios);

	emac_hal_iomux_rmii_clk_output(ref_clk_gpio);
	emac_ll_clock_enable_rmii_output(dev_data->hal.ext_regs);
	periph_rtc_apll_acquire();
	res = emac_config_apll_clock();
	if (res != 0) {
		goto err;
	}
	rtc_clk_apll_enable(true);
#endif

	/* Init MDIO clock */
	emac_hal_set_csr_clock_range(&dev_data->hal, esp_clk_apb_freq());

	return 0;

err:
	return res;
}

static DEVICE_API(mdio, mdio_esp32_driver_api) = {
	.read = mdio_esp32_read,
	.write = mdio_esp32_write,
};

#define MDIO_ESP32_CONFIG(n)						\
static const struct mdio_esp32_dev_config mdio_esp32_dev_config_##n = {	\
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),			\
};

#define MDIO_ESP32_DEVICE(n)						\
	PINCTRL_DT_INST_DEFINE(n);					\
	MDIO_ESP32_CONFIG(n);						\
	static struct mdio_esp32_dev_data mdio_esp32_dev_data##n;	\
	DEVICE_DT_INST_DEFINE(n,					\
			      &mdio_esp32_initialize,			\
			      NULL,					\
			      &mdio_esp32_dev_data##n,			\
			      &mdio_esp32_dev_config_##n, POST_KERNEL,	\
			      CONFIG_MDIO_INIT_PRIORITY,		\
			      &mdio_esp32_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MDIO_ESP32_DEVICE)
