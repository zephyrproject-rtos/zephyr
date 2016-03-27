/*
 * Copyright (c) 2015, Intel Corporation
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

#include "qm_gpio.h"

#ifndef UNIT_TEST
#if (QUARK_SE)
qm_gpio_reg_t *qm_gpio[QM_GPIO_NUM] = {(qm_gpio_reg_t *)QM_GPIO_BASE,
				       (qm_gpio_reg_t *)QM_AON_GPIO_BASE};
#elif(QUARK_D2000)
qm_gpio_reg_t *qm_gpio[QM_GPIO_NUM] = {(qm_gpio_reg_t *)QM_GPIO_BASE};
#endif
#endif

static void (*callback[QM_GPIO_NUM])(uint32_t);

static void gpio_isr(const qm_gpio_t gpio)
{
	uint32_t int_status = QM_GPIO[gpio]->gpio_intstatus;

	if (callback[gpio]) {
		(*callback[gpio])(int_status);
	}

	/* This will clear all pending interrupts flags in status */
	QM_GPIO[gpio]->gpio_porta_eoi = int_status;
	/* Read back EOI register to avoid a spurious interrupt due to EOI
	 * propagation delay */
	QM_GPIO[gpio]->gpio_porta_eoi;
}

void qm_gpio_isr_0(void)
{
	gpio_isr(QM_GPIO_0);
	QM_ISR_EOI(QM_IRQ_GPIO_0_VECTOR);
}

#if (HAS_AON_GPIO)
void qm_aon_gpio_isr_0(void)
{
	gpio_isr(QM_AON_GPIO_0);
	QM_ISR_EOI(QM_IRQ_AONGPIO_0_VECTOR);
}
#endif

qm_rc_t qm_gpio_set_config(const qm_gpio_t gpio,
			   const qm_gpio_port_config_t *const cfg)
{
	QM_CHECK(gpio < QM_GPIO_NUM, QM_RC_EINVAL);
	QM_CHECK(cfg != NULL, QM_RC_EINVAL);

	qm_gpio_reg_t *const controller = QM_GPIO[gpio];

	uint32_t mask = controller->gpio_intmask;
	controller->gpio_intmask = 0xffffffff;

	controller->gpio_swporta_ddr = cfg->direction;
	controller->gpio_inttype_level = cfg->int_type;
	controller->gpio_int_polarity = cfg->int_polarity;
	controller->gpio_debounce = cfg->int_debounce;
	controller->gpio_int_bothedge = cfg->int_bothedge;
	callback[gpio] = cfg->callback;
	controller->gpio_inten = cfg->int_en;

	controller->gpio_intmask = mask;

	return QM_RC_OK;
}

qm_rc_t qm_gpio_get_config(const qm_gpio_t gpio,
			   qm_gpio_port_config_t *const cfg)
{
	QM_CHECK(gpio < QM_GPIO_NUM, QM_RC_EINVAL);
	QM_CHECK(cfg != NULL, QM_RC_EINVAL);

	qm_gpio_reg_t *const controller = QM_GPIO[gpio];

	cfg->direction = controller->gpio_swporta_ddr;
	cfg->int_en = controller->gpio_inten;
	cfg->int_type = controller->gpio_inttype_level;
	cfg->int_polarity = controller->gpio_int_polarity;
	cfg->int_debounce = controller->gpio_debounce;
	cfg->int_bothedge = controller->gpio_int_bothedge;
	cfg->callback = callback[gpio];

	return QM_RC_OK;
}

bool qm_gpio_read_pin(const qm_gpio_t gpio, const uint8_t pin)
{
	return (((QM_GPIO[gpio]->gpio_ext_porta) >> pin) & 1);
}

qm_rc_t qm_gpio_set_pin(const qm_gpio_t gpio, const uint8_t pin)
{
	QM_CHECK(gpio < QM_GPIO_NUM, QM_RC_EINVAL);
	QM_CHECK(pin <= QM_NUM_GPIO_PINS, QM_RC_EINVAL);

	QM_GPIO[gpio]->gpio_swporta_dr |= (1 << pin);

	return QM_RC_OK;
}

qm_rc_t qm_gpio_clear_pin(const qm_gpio_t gpio, const uint8_t pin)
{
	QM_CHECK(gpio < QM_GPIO_NUM, QM_RC_EINVAL);
	QM_CHECK(pin <= QM_NUM_GPIO_PINS, QM_RC_EINVAL);

	QM_GPIO[gpio]->gpio_swporta_dr &= ~(1 << pin);

	return QM_RC_OK;
}

uint32_t qm_gpio_read_port(const qm_gpio_t gpio)
{
	return (QM_GPIO[gpio]->gpio_ext_porta);
}

qm_rc_t qm_gpio_write_port(const qm_gpio_t gpio, const uint32_t val)
{
	QM_CHECK(gpio < QM_GPIO_NUM, QM_RC_EINVAL);

	QM_GPIO[gpio]->gpio_swporta_dr = val;

	return QM_RC_OK;
}
