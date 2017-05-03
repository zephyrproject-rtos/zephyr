/**
 *
 * \file
 *
 * \brief This module contains NMC1000 bus wrapper APIs implementation.
 *
 * Copyright (c) 2015 Atmel Corporation. All rights reserved.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. The name of Atmel may not be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * EXPRESSLY AND SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * \asf_license_stop
 *
 */

#include <stdio.h>
#include <stdint.h>

#include <board.h>
#include <device.h>
#include <gpio.h>
#include <spi.h>
#include <logging/sys_log.h>

#include "bsp/include/nm_bsp.h"
#include "common/include/nm_common.h"
#include "bus_wrapper/include/nm_bus_wrapper.h"
#include "conf_winc.h"

#define NM_BUS_MAX_TRX_SZ       256


struct mm_bus_capabilities nm_bus_capabilities = {
	NM_BUS_MAX_TRX_SZ
};

#ifdef CONF_WINC_USE_I2C

#define SLAVE_ADDRESS 0x60

/** Number of times to try to send packet if failed. */
#define I2C_TIMEOUT 100

static s8_t nm_i2c_write(u8_t *b, u16_t sz)
{
	/* Not implemented */
}

static s8_t nm_i2c_read(u8_t *rb, u16_t sz)
{
	/* Not implemented */
}

static s8_t nm_i2c_write_special(u8_t *wb1, u16_t sz1,
				 u8_t *wb2, u16_t sz2)
{
	/* Not implemented */
}
#endif

#ifdef CONF_WINC_USE_SPI

static s8_t spi_rw(u8_t *mosi, u8_t *miso, u16_t size)
{
	int ret;

	spi_slave_select(winc1500.spi_dev, winc1500.spi_slave);

	ret = spi_transceive(winc1500.spi_dev,
			     mosi, size,
			     miso, miso ? size : 0);
	if (ret) {
		SYS_LOG_ERR("spi_rw spi_transceive fail %d\n", ret);
		return M2M_ERR_BUS_FAIL;
	}

	return M2M_SUCCESS;
}

#endif

/*
 *	@fn		nm_bus_init
 *	@brief	Initialize the bus wrapper
 *	@return	0 in case of success and -1 in case of failure
 */
s8_t nm_bus_init(void *pvinit)
{
	s8_t result = 0;

#ifdef CONF_WINC_USE_I2C
	/* Not implemented */
#elif defined CONF_WINC_USE_SPI
	struct spi_config spi_config;
	int ret;

	/* configure GPIOs */
	winc1500.gpios = winc1500_configure_gpios();

	/* setup SPI device */
	winc1500.spi_dev = device_get_binding(CONFIG_WINC1500_SPI_DRV_NAME);
	if (!winc1500.spi_dev) {
		SYS_LOG_ERR("spi device binding\n");
		return -1;
	}

	spi_config.config = SPI_WORD(8) | SPI_TRANSFER_MSB;
	spi_config.max_sys_freq = CONFIG_WINC1500_SPI_FREQ;
	ret = spi_configure(winc1500.spi_dev, &spi_config);
	if (ret) {
		SYS_LOG_ERR("spi_configure fail %d\n", ret);
		return -1;
	}
	winc1500.spi_slave = CONFIG_WINC1500_SPI_SLAVE;
	SYS_LOG_INF("spi_configure OK\n");

	winc1500_configure_intgpios();

	nm_bsp_reset();
	nm_bsp_sleep(1);

	nm_bsp_interrupt_ctrl(1);

	SYS_LOG_INF("nm_bus_init:NOTICE:DONE\n");
#endif
	return result;
}

/*
 *	@fn		nm_bus_ioctl
 *	@brief	send/receive from the bus
 *	@param[IN]	cmd
 *					IOCTL command for the operation
 *	@param[IN]	parameter
 *					Arbitrary parameter depenging on IOCTL
 *	@return	0 in case of success and -1 in case of failure
 *	@note	For SPI only, it's important to be able to
 *	        send/receive at the same time
 */
s8_t nm_bus_ioctl(u8_t cmd, void *parameter)
{
	sint8 ret = 0;

	switch (cmd) {
#ifdef CONF_WINC_USE_I2C
	case NM_BUS_IOCTL_R: {
		struct nm_i2c_default *param = (struct nm_i2c_default *)parameter;

		ret = nm_i2c_read(param->buffer, param->size);
	}
	break;
	case NM_BUS_IOCTL_W: {
		struct nm_i2c_default *param = (struct nm_i2c_default *)parameter;

		ret = nm_i2c_write(param->buffer, param->size);
	}
	break;
	case NM_BUS_IOCTL_W_SPECIAL: {
		struct nm_i2c_special *param = (struct nm_i2c_special *)parameter;

		ret = nm_i2c_write_special(param->buffer1, param->size1,
					   param->buffer2, param->size2);
	}
	break;
#elif defined CONF_WINC_USE_SPI
	case NM_BUS_IOCTL_RW: {
		struct nm_spi_rw *param = (struct nm_spi_rw *)parameter;

		ret = spi_rw(param->in_buffer, param->out_buffer, param->size);
	}
	break;
#endif
	default:
		ret = -1;
		M2M_ERR("nm_bus_ioctl:ERROR:invalide ioclt cmd\n");
		break;
	}

	return ret;
}

/*
 *	@fn		nm_bus_deinit
 *	@brief	De-initialize the bus wrapper
 */
s8_t nm_bus_deinit(void)
{
	return M2M_SUCCESS;
}

/*
 *	@fn			nm_bus_reinit
 *	@brief		re-initialize the bus wrapper
 *	@param [in]	void *config
 *					re-init configuration data
 *	@return		0 in case of success and -1 in case of failure
 *	@author		Dina El Sissy
 *	@date		19 Sept 2012
 *	@version	1.0
 */
s8_t nm_bus_reinit(void *config)
{
	return 0;
}

