/*
 * Copyright (c) 2022 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/init.h>
#include <zephyr/drivers/pinctrl.h>
#include <nrfx_qspi.h>
#include <hal/nrf_clock.h>

#define QSPI_STD_CMD_WRSR  0x01
#define QSPI_STD_CMD_RSTEN 0x66
#define QSPI_STD_CMD_RST   0x99

#define QSPI_NODE DT_NODELABEL(qspi)

static int qspi_ext_mem_init(void)
{

	static const nrfx_qspi_config_t qspi_config = {
		.prot_if = {
			.readoc    = NRF_QSPI_READOC_READ4IO,
			.writeoc   = NRF_QSPI_WRITEOC_PP4IO,
			.addrmode  = NRF_QSPI_ADDRMODE_24BIT,
		},
		.phy_if = {
			/* Frequency = PCLK192M / 2 * (sck_freq + 1) -> 32 MHz.
			 * 33 MHz is the maximum for WRSR, even in the High
			 * Performance mode.
			 */
			.sck_freq  = 2,
			.sck_delay = 0x05,
			.spi_mode  = NRF_QSPI_MODE_0,
		},
		.skip_gpio_cfg = true,
		.skip_psel_cfg = true,
	};
	static const nrf_qspi_phy_conf_t qspi_phy_48mhz = {
		/* After sending WRSR, use 48 MHz (96 MHz cannot be used,
		 * as 80 MHz is the maximum for the MX25R6435F chip).
		 */
		.sck_freq  = 1,
		.sck_delay = 0x05,
		.spi_mode  = NRF_QSPI_MODE_0,
	};
	nrf_qspi_cinstr_conf_t cinstr_cfg = {
		.opcode    = QSPI_STD_CMD_RSTEN,
		.length    = NRF_QSPI_CINSTR_LEN_1B,
		.io2_level = true,
		.io3_level = true,
		.wipwait   = true,
	};
	static const uint8_t flash_chip_cfg[] = {
		/* QE (Quad Enable) bit = 1 */
		BIT(6),
		0x00,
		/* L/H Switch bit = 1 -> High Performance mode */
		BIT(1),
	};
	nrfx_err_t err;
	int ret;

	PINCTRL_DT_DEFINE(QSPI_NODE);

	ret = pinctrl_apply_state(PINCTRL_DT_DEV_CONFIG_GET(QSPI_NODE),
				  PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	err = nrfx_qspi_init(&qspi_config, NULL, NULL);
	if (err != NRFX_SUCCESS) {
		return -EIO;
	}

	nrf_clock_hfclk192m_div_set(NRF_CLOCK, NRF_CLOCK_HFCLK_DIV_1);

	/* Send reset enable */
	nrfx_qspi_cinstr_xfer(&cinstr_cfg, NULL, NULL);

	/* Send reset command */
	cinstr_cfg.opcode = QSPI_STD_CMD_RST;
	nrfx_qspi_cinstr_xfer(&cinstr_cfg, NULL, NULL);

	/* Switch to Quad I/O and High Performance mode */
	cinstr_cfg.opcode = QSPI_STD_CMD_WRSR;
	cinstr_cfg.wren   = true;
	cinstr_cfg.length = NRF_QSPI_CINSTR_LEN_4B;
	nrfx_qspi_cinstr_xfer(&cinstr_cfg, &flash_chip_cfg, NULL);

	nrf_qspi_ifconfig1_set(NRF_QSPI, &qspi_phy_48mhz);

	/* Enable XiP */
	nrf_qspi_xip_set(NRF_QSPI, true);

	return 0;
}

SYS_INIT(qspi_ext_mem_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_OBJECTS);
