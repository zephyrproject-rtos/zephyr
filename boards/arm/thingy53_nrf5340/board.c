/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <hal/nrf_gpio.h>
#include <nrfx.h>
#include <device.h>
#include <drivers/gpio.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(thingy53_board_init);

#define NRF21540FEM_CTRL_NODE		DT_NODELABEL(nrf21540fem_ctrl)
#define NRF21540FEM_CTRL_PDN_GPIO_NODE	DT_GPIO_CTLR(NRF21540FEM_CTRL_NODE, pdn_gpios)
#define NRF21540FEM_CTRL_PDN_PIN	DT_GPIO_PIN(NRF21540FEM_CTRL_NODE, pdn_gpios)
#define NRF21540FEM_CTRL_PDN_FLAGS	DT_GPIO_FLAGS(NRF21540FEM_CTRL_NODE, pdn_gpios)

#define ADXL362_NODE			DT_NODELABEL(adxl362)
#define ADXL362_GPIO_NODE		DT_SPI_DEV_CS_GPIOS_CTLR(ADXL362_NODE)
#define ADXL362_CS			DT_SPI_DEV_CS_GPIOS_PIN(ADXL362_NODE)
#define ADXL362_FLAGS			DT_SPI_DEV_CS_GPIOS_FLAGS(ADXL362_NODE)

#define BMI270_NODE			DT_NODELABEL(bmi270)
#define BMI270_GPIO_NODE		DT_SPI_DEV_CS_GPIOS_CTLR(BMI270_NODE)
#define BMI270_CS			DT_SPI_DEV_CS_GPIOS_PIN(BMI270_NODE)
#define BMI270_FLAGS			DT_SPI_DEV_CS_GPIOS_FLAGS(BMI270_NODE)

#define NRF21540FEM_NODE		DT_NODELABEL(nrf21540fem)
#define NRF21540FEM_GPIO_NODE		DT_SPI_DEV_CS_GPIOS_CTLR(NRF21540FEM_NODE)
#define NRF21540FEM_CS			DT_SPI_DEV_CS_GPIOS_PIN(NRF21540FEM_NODE)
#define NRF21540FEM_FLAGS		DT_SPI_DEV_CS_GPIOS_FLAGS(NRF21540FEM_NODE)

/* Initialization chain of Thingy:53 board requires some delays before on board sensors
 * could be accessed after power up. In particular bme680 and bmm150 sensors require,
 * respectively 2ms and 1ms power on delay. In order to avoid delays sum, common delay is
 * introduced in the board start up file. Below asserts ensure correct initialization order:
 * on board regulators, board init (this), sensors init.
 */
BUILD_ASSERT(CONFIG_THINGY53_INIT_PRIORITY > CONFIG_REGULATOR_FIXED_INIT_PRIORITY,
	"CONFIG_THINGY53_INIT_PRIORITY must be higher than CONFIG_REGULATOR_FIXED_INIT_PRIORITY");
#if defined(CONFIG_SENSOR)
BUILD_ASSERT(CONFIG_THINGY53_INIT_PRIORITY < CONFIG_SENSOR_INIT_PRIORITY,
	"CONFIG_THINGY53_INIT_PRIORITY must be less than CONFIG_SENSOR_INIT_PRIORITY");
#endif

static void enable_cpunet(void)
{
#if !defined(CONFIG_TRUSTED_EXECUTION_NONSECURE)
	/* Give nRF21540fem control pins to NetworkMCU */
	nrf_gpio_pin_mcu_select(NRF_DT_GPIOS_TO_PSEL(NRF21540FEM_CTRL_NODE, tx_en_gpios),
				GPIO_PIN_CNF_MCUSEL_NetworkMCU); /* TX_EN */
	nrf_gpio_pin_mcu_select(NRF_DT_GPIOS_TO_PSEL(NRF21540FEM_CTRL_NODE, pdn_gpios),
				GPIO_PIN_CNF_MCUSEL_NetworkMCU); /* PDN */
	nrf_gpio_pin_mcu_select(NRF_DT_GPIOS_TO_PSEL(NRF21540FEM_CTRL_NODE, rx_en_gpios),
				GPIO_PIN_CNF_MCUSEL_NetworkMCU); /* RX_EN */
	nrf_gpio_pin_mcu_select(NRF_DT_GPIOS_TO_PSEL(NRF21540FEM_CTRL_NODE, mode_gpios),
				GPIO_PIN_CNF_MCUSEL_NetworkMCU); /* MODE */

	/* Retain nRF5340 Network MCU in Secure domain (bus
	 * accesses by Network MCU will have Secure attribute set).
	 */
	NRF_SPU->EXTDOMAIN[0].PERM = 1 << 4;
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
}

static int setup(const struct device *dev)
{
	ARG_UNUSED(dev);
#if !defined(CONFIG_TRUSTED_EXECUTION_SECURE)

	const struct device *gpio;
	int err;

	gpio = DEVICE_DT_GET(NRF21540FEM_CTRL_PDN_GPIO_NODE);
	if (!device_is_ready(gpio)) {
		LOG_ERR("%s device not ready", gpio->name);
		return -ENODEV;
	}
	err = gpio_pin_configure(gpio, NRF21540FEM_CTRL_PDN_PIN,
				 NRF21540FEM_CTRL_PDN_FLAGS |
				 IS_ENABLED(CONFIG_THINGY53_MISO_WORKAROUND) ?
				 GPIO_OUTPUT_ACTIVE : GPIO_OUTPUT_INACTIVE);
	if (err < 0) {
		LOG_ERR("Failed to configure NRF21540FEM PDN Pin");
		return err;
	}

	gpio = DEVICE_DT_GET(ADXL362_GPIO_NODE);
	if (!device_is_ready(gpio)) {
		LOG_ERR("%s device not ready", gpio->name);
		return -ENODEV;
	}
	err = gpio_pin_configure(gpio, ADXL362_CS, ADXL362_FLAGS | GPIO_OUTPUT_INACTIVE);
	if (err < 0) {
		LOG_ERR("Failed to configure ADXL362 CS Pin");
		return err;
	}

	gpio = DEVICE_DT_GET(BMI270_GPIO_NODE);
	if (!device_is_ready(gpio)) {
		LOG_ERR("%s device not ready", gpio->name);
		return -ENODEV;
	}
	err = gpio_pin_configure(gpio, BMI270_CS, BMI270_FLAGS | GPIO_OUTPUT_INACTIVE);
	if (err < 0) {
		LOG_ERR("Failed to configure BMI270 CS Pin");
		return err;
	}

	gpio = DEVICE_DT_GET(NRF21540FEM_GPIO_NODE);
	if (!device_is_ready(gpio)) {
		LOG_ERR("%s device not ready", gpio->name);
		return -ENODEV;
	}
	err = gpio_pin_configure(gpio, NRF21540FEM_CS, NRF21540FEM_FLAGS | GPIO_OUTPUT_INACTIVE);
	if (err < 0) {
		LOG_ERR("Failed to configure NRF21540FEM CS Pin");
		return err;
	}

	if (IS_ENABLED(CONFIG_SENSOR)) {
		/* Initialization chain of Thingy:53 board requires some delays before on board
		 * sensors could be accessed after power up. In particular bme680 and bmm150
		 * sensors require, 2ms and 1ms power on delay respectively. In order not to sum
		 * delays, common delay is introduced in the board start up file. This code is
		 * executed after sensors are powered up and before their initialization.
		 * It's ensured by build asserts at the beggining of this file.
		 */
		k_msleep(2);
	}

#endif /* !CONFIG_TRUSTED_EXECUTION_SECURE */

	if (IS_ENABLED(CONFIG_BOARD_ENABLE_CPUNET)) {
		enable_cpunet();
	}

	return 0;
}

SYS_INIT(setup, POST_KERNEL, CONFIG_THINGY53_INIT_PRIORITY);
