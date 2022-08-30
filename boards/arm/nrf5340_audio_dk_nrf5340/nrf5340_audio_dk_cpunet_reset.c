/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>

#include <soc.h>
#include <zephyr/drivers/gpio.h>

#include "nrfx_gpiote.h"

LOG_MODULE_REGISTER(nrf5340_audio_dk_nrf5340_cpuapp, CONFIG_LOG_DEFAULT_LEVEL);

#if !defined(CONFIG_TRUSTED_EXECUTION_NONSECURE)

#if defined(CONFIG_BT_CTLR_DEBUG_PINS_CPUAPP)
#include <../subsys/bluetooth/controller/ll_sw/nordic/hal/nrf5/debug.h>
#else
#define DEBUG_SETUP()
#endif

#define SHARED_SPI DT_NODELABEL(spi4) /* SD card and HW codec share the SPI4 */

static int core_config(void)
{
	int ret;
	nrf_gpiote_latency_t latency;

	latency = nrfx_gpiote_latency_get();

	if (latency != NRF_GPIOTE_LATENCY_LOWPOWER) {
		LOG_DBG("Setting gpiote latency to low power");
		nrfx_gpiote_latency_set(NRF_GPIOTE_LATENCY_LOWPOWER);
	}

	/* USB port detection
	 * See nPM1100 datasheet for more information
	 */
	static const struct gpio_dt_spec pmic_iset =
		GPIO_DT_SPEC_GET(DT_NODELABEL(pmic_iset_out), gpios);

	if (!device_is_ready(pmic_iset.port)) {
		LOG_ERR("GPIO is not ready!");
		return -ENXIO;
	}

	ret = gpio_pin_configure_dt(&pmic_iset, GPIO_OUTPUT_LOW);
	if (ret) {
		return ret;
	}

	/* Set on-board DSP/HW codec as default */
	static const struct gpio_dt_spec hw_codec_sel =
		GPIO_DT_SPEC_GET(DT_NODELABEL(hw_codec_sel_out), gpios);

	if (!device_is_ready(hw_codec_sel.port)) {
		LOG_ERR("GPIO is not ready!");
		return -ENXIO;
	}

	ret = gpio_pin_configure_dt(&hw_codec_sel, GPIO_OUTPUT_LOW);
	if (ret) {
		return ret;
	}

	/* Pull the CS47L63 reset line to high (this pin is active low) */
	static const struct gpio_dt_spec hw_codec_reset =
		GPIO_DT_SPEC_GET(DT_NODELABEL(hw_codec_reset_out), gpios);

	if (!device_is_ready(hw_codec_reset.port)) {
		LOG_ERR("GPIO is not ready!");
		return -ENXIO;
	}

	ret = gpio_pin_configure_dt(&hw_codec_reset, GPIO_OUTPUT_HIGH);
	if (ret) {
		return ret;
	}

	/* Disable board revision readback as default */
	static const struct gpio_dt_spec board_id_en =
		GPIO_DT_SPEC_GET(DT_NODELABEL(board_id_en_out), gpios);

	if (!device_is_ready(board_id_en.port)) {
		LOG_ERR("GPIO is not ready!");
		return -ENXIO;
	}

	ret = gpio_pin_configure_dt(&board_id_en, GPIO_OUTPUT_LOW);
	if (ret) {
		return ret;
	}

	return 0;
}

static void remoteproc_mgr_config(void)
{
	/* Route Bluetooth Controller Debug Pins */
	DEBUG_SETUP();

	/* Retain nRF5340 Network MCU in Secure domain (bus
	 * accesses by Network MCU will have Secure attribute set).
	 */
	NRF_SPU->EXTDOMAIN[0].PERM = 1 << 4;
}
#endif /* !CONFIG_TRUSTED_EXECUTION_NONSECURE */

static int remoteproc_mgr_boot(const struct device *dev)
{
	int ret;

	ARG_UNUSED(dev);

	ret = core_config();
	if (ret) {
		return ret;
	}

#if !defined(CONFIG_TRUSTED_EXECUTION_NONSECURE)
	/* Secure domain may configure permissions for the Network MCU. */
	remoteproc_mgr_config();
#endif /* !CONFIG_TRUSTED_EXECUTION_NONSECURE */

#if !defined(CONFIG_TRUSTED_EXECUTION_SECURE)
	/*
	 * Building Zephyr with CONFIG_TRUSTED_EXECUTION_SECURE=y implies
	 * building also a Non-Secure image. The Non-Secure image will, in
	 * this case do the remainder of actions to properly configure and
	 * boot the Network MCU.
	 */

	/* Release the Network MCU, 'Release force off signal' */
	NRF_RESET->NETWORK.FORCEOFF = RESET_NETWORK_FORCEOFF_FORCEOFF_Release;

	LOG_DBG("Network MCU released.");
#endif /* !CONFIG_TRUSTED_EXECUTION_SECURE */

	return 0;
}

SYS_INIT(remoteproc_mgr_boot, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
