/*
 * Copyright (c) 2022 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/init.h>
#include <zephyr/zephyr.h>
#include <zephyr/device.h>
#include <nrfx_qspi.h>
#include <hal/nrf_clock.h>
#include <zephyr/drivers/pinctrl.h>

#define QSPI_STD_CMD_WRSR  0x01
#define QSPI_STD_CMD_WRDI  0x04
#define QSPI_STD_CMD_RSTEN 0x66
#define QSPI_STD_CMD_RST   0x99

#define QSPI_NODE DT_NODELABEL(qspi)
#define QSPI_PROP_AT(prop, idx) DT_PROP_BY_IDX(QSPI_NODE, prop, idx)

static inline int qspi_get_zephyr_ret_code(nrfx_err_t res)
{
	switch (res) {
	case NRFX_SUCCESS:
		return 0;
	case NRFX_ERROR_INVALID_PARAM:
	case NRFX_ERROR_INVALID_ADDR:
		return -EINVAL;
	case NRFX_ERROR_INVALID_STATE:
		return -ECANCELED;
	case NRFX_ERROR_BUSY:
	case NRFX_ERROR_TIMEOUT:
	default:
		return -EBUSY;
	}
}

static int qspi_ext_mem_init(const struct device *dev)
{
	int ret;

	PINCTRL_DT_DEFINE(QSPI_NODE);

	ARG_UNUSED(dev);

	nrf_clock_hfclk192m_div_set(NRF_CLOCK, NRF_CLOCK_HFCLK_DIV_1);

	static nrfx_qspi_config_t qspi_config = {
		.irq_priority = NRFX_QSPI_DEFAULT_CONFIG_IRQ_PRIORITY,
		.prot_if = {
			.readoc    = NRF_QSPI_READOC_READ4IO,
			.writeoc   = NRF_QSPI_WRITEOC_PP4IO,
			.addrmode  = NRF_QSPI_ADDRMODE_24BIT,
			.dpmconfig = false
		},
		.phy_if = {
			.sck_freq  = 1,
			.sck_delay = 0x05,
			.spi_mode  = NRF_QSPI_MODE_0,
			.dpmen     = false
		},
		.skip_gpio_cfg = true,
		.skip_psel_cfg = true,
	};

	ret = pinctrl_apply_state(PINCTRL_DT_DEV_CONFIG_GET(QSPI_NODE),
				  PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	nrfx_err_t res = nrfx_qspi_init(&qspi_config, NULL, NULL);

	ret = qspi_get_zephyr_ret_code(res);
	if (ret < 0) {
		return ret;
	}

	/* Enable XiP */
	nrf_qspi_xip_set(NRF_QSPI, true);

	uint8_t temporary = 0x40;
	nrf_qspi_cinstr_conf_t cinstr_cfg = {
		.opcode    = QSPI_STD_CMD_RSTEN,
		.length    = NRF_QSPI_CINSTR_LEN_1B,
		.io2_level = true,
		.io3_level = true,
		.wipwait   = true,
		.wren      = true
	};

	/* Send reset enable */
	nrfx_qspi_cinstr_xfer(&cinstr_cfg, NULL, NULL);

	/* Send reset command */
	cinstr_cfg.opcode = QSPI_STD_CMD_RST;
	nrfx_qspi_cinstr_xfer(&cinstr_cfg, NULL, NULL);

	/* Switch to qspi mode and high performance */
	cinstr_cfg.opcode = QSPI_STD_CMD_WRSR;
	cinstr_cfg.length = NRF_QSPI_CINSTR_LEN_2B;
	nrfx_qspi_cinstr_xfer(&cinstr_cfg, &temporary, NULL);

	/* Send write disable command */
	cinstr_cfg.opcode = QSPI_STD_CMD_WRDI;
	cinstr_cfg.wren   = false;
	cinstr_cfg.length = NRF_QSPI_CINSTR_LEN_1B;
	nrfx_qspi_cinstr_xfer(&cinstr_cfg, NULL, NULL);

	return 0;
}

SYS_INIT(qspi_ext_mem_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_OBJECTS);
