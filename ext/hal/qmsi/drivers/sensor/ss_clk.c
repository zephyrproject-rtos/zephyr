/*
 * Copyright (c) 2016, Intel Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of the Intel Corporation nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE INTEL CORPORATION OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "qm_common.h"
#include "ss_clk.h"

int ss_clk_gpio_enable(const qm_ss_gpio_t gpio)
{
	QM_CHECK(gpio < QM_SS_GPIO_NUM, -EINVAL);
	int addr =
	    (gpio == QM_SS_GPIO_0) ? QM_SS_GPIO_0_BASE : QM_SS_GPIO_1_BASE;
	__builtin_arc_sr(QM_SS_GPIO_LS_SYNC_CLK_EN |
			     QM_SS_GPIO_LS_SYNC_SYNC_LVL,
			 addr + QM_SS_GPIO_LS_SYNC);
	return 0;
}

int ss_clk_gpio_disable(const qm_ss_gpio_t gpio)
{
	QM_CHECK(gpio < QM_SS_GPIO_NUM, -EINVAL);
	int addr =
	    (gpio == QM_SS_GPIO_0) ? QM_SS_GPIO_0_BASE : QM_SS_GPIO_1_BASE;
	__builtin_arc_sr(0, addr + QM_SS_GPIO_LS_SYNC);
	return 0;
}

int ss_clk_spi_enable(const qm_ss_spi_t spi)
{
	QM_CHECK(spi < QM_SS_SPI_NUM, -EINVAL);
	int addr = (spi == QM_SS_SPI_0) ? QM_SS_SPI_0_BASE : QM_SS_SPI_1_BASE;
	QM_SS_REG_AUX_OR(addr + QM_SS_SPI_CTRL, QM_SS_SPI_CTRL_CLK_ENA);
	return 0;
}

int ss_clk_spi_disable(const qm_ss_spi_t spi)
{
	QM_CHECK(spi < QM_SS_SPI_NUM, -EINVAL);
	int addr = (spi == QM_SS_SPI_0) ? QM_SS_SPI_0_BASE : QM_SS_SPI_1_BASE;
	QM_SS_REG_AUX_NAND(addr + QM_SS_SPI_CTRL, QM_SS_SPI_CTRL_CLK_ENA);
	return 0;
}

int ss_clk_i2c_enable(const qm_ss_i2c_t i2c)
{
	QM_CHECK(i2c < QM_SS_I2C_NUM, -EINVAL);
	int addr = (i2c == QM_SS_I2C_0) ? QM_SS_I2C_0_BASE : QM_SS_I2C_1_BASE;
	QM_SS_REG_AUX_OR(addr + QM_SS_I2C_CON, QM_SS_I2C_CON_CLK_ENA);
	return 0;
}

int ss_clk_i2c_disable(const qm_ss_i2c_t i2c)
{
	QM_CHECK(i2c < QM_SS_I2C_NUM, -EINVAL);
	int addr = (i2c == QM_SS_I2C_0) ? QM_SS_I2C_0_BASE : QM_SS_I2C_1_BASE;
	QM_SS_REG_AUX_NAND(addr + QM_SS_I2C_CON, QM_SS_I2C_CON_CLK_ENA);
	return 0;
}

int ss_clk_adc_enable(void)
{
	/* Enable the ADC clock */
	QM_SS_REG_AUX_OR(QM_SS_ADC_BASE + QM_SS_ADC_CTRL,
			 QM_SS_ADC_CTRL_CLK_ENA);
	return 0;
}

int ss_clk_adc_disable(void)
{
	/* Disable the ADC clock */
	QM_SS_REG_AUX_NAND(QM_SS_ADC_BASE + QM_SS_ADC_CTRL,
			   QM_SS_ADC_CTRL_CLK_ENA);
	return 0;
}

int ss_clk_adc_set_div(const uint32_t div)
{
	uint32_t reg;

	/*
	 * Scale the max divisor with the system clock speed. Clock speeds less
	 * than 1 MHz will not work properly.
	 */
	QM_CHECK(div <= QM_SS_ADC_DIV_MAX * clk_sys_get_ticks_per_us(),
		 -EINVAL);

	/* Set the ADC divisor */
	reg = __builtin_arc_lr(QM_SS_ADC_BASE + QM_SS_ADC_DIVSEQSTAT);
	reg &= ~(QM_SS_ADC_DIVSEQSTAT_CLK_RATIO_MASK);
	__builtin_arc_sr(reg | div, QM_SS_ADC_BASE + QM_SS_ADC_DIVSEQSTAT);

	return 0;
}
