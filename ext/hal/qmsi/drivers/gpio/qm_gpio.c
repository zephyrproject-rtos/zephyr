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

#include "qm_gpio.h"

#if (HAS_SOC_CONTEXT_RETENTION)
#include "power_states.h"
#endif /* HAS_SOC_CONTEXT_RETENTION */

#define ENABLE_PCLK (0x1)

#ifndef UNIT_TEST
qm_gpio_reg_t *qm_gpio[QM_GPIO_NUM] = {(qm_gpio_reg_t *)QM_GPIO_BASE,
#if (HAS_AON_GPIO)
				       (qm_gpio_reg_t *)QM_AON_GPIO_BASE
#endif /* HAS_AON_GPIO */
};
#endif /* UNIT_TEST */

static void (*callback[QM_GPIO_NUM])(void *, uint32_t);
static void *callback_data[QM_GPIO_NUM];

static void gpio_isr(const qm_gpio_t gpio)
{
	const uint32_t int_status = QM_GPIO[gpio]->gpio_intstatus;

#if (HAS_SOC_CONTEXT_RETENTION)
	if (QM_SCSS_GP->gps0 & QM_GPS0_POWER_STATES_MASK) {
		qm_power_soc_restore();
	}
#endif

	if (callback[gpio]) {
		(*callback[gpio])(callback_data[gpio], int_status);
	}

	/* This will clear all pending interrupts flags in status */
	QM_GPIO[gpio]->gpio_porta_eoi = int_status;
	/* Read back EOI register to avoid a spurious interrupt due to EOI
	 * propagation delay */
	QM_GPIO[gpio]->gpio_porta_eoi;
}

QM_ISR_DECLARE(qm_gpio_0_isr)
{
	gpio_isr(QM_GPIO_0);
	QM_ISR_EOI(QM_IRQ_GPIO_0_INT_VECTOR);
}

#if (HAS_AON_GPIO)
QM_ISR_DECLARE(qm_aon_gpio_0_isr)
{
	gpio_isr(QM_AON_GPIO_0);
	QM_ISR_EOI(QM_IRQ_AON_GPIO_0_INT_VECTOR);
}
#endif

int qm_gpio_set_config(const qm_gpio_t gpio,
		       const qm_gpio_port_config_t *const cfg)
{
	QM_CHECK(gpio < QM_GPIO_NUM, -EINVAL);
	QM_CHECK(cfg != NULL, -EINVAL);

	qm_gpio_reg_t *const controller = QM_GPIO[gpio];

	uint32_t mask = controller->gpio_intmask;
	controller->gpio_intmask = 0xffffffff;

	controller->gpio_swporta_ddr = cfg->direction;
	controller->gpio_inten = cfg->int_en;
	controller->gpio_inttype_level = cfg->int_type;
	controller->gpio_int_polarity = cfg->int_polarity;
	controller->gpio_debounce = cfg->int_debounce;
	controller->gpio_int_bothedge = cfg->int_bothedge;
	controller->gpio_ls_sync |= ENABLE_PCLK;
	callback[gpio] = cfg->callback;
	callback_data[gpio] = cfg->callback_data;

	controller->gpio_intmask = mask;

	return 0;
}

int qm_gpio_read_pin(const qm_gpio_t gpio, const uint8_t pin,
		     qm_gpio_state_t *const state)
{
	QM_CHECK(gpio < QM_GPIO_NUM, -EINVAL);
	QM_CHECK(pin <= QM_NUM_GPIO_PINS, -EINVAL);
	QM_CHECK(state != NULL, -EINVAL);

	*state = ((QM_GPIO[gpio]->gpio_ext_porta) >> pin) & 1;

	return 0;
}

int qm_gpio_set_pin(const qm_gpio_t gpio, const uint8_t pin)
{
	QM_CHECK(gpio < QM_GPIO_NUM, -EINVAL);
	QM_CHECK(pin <= QM_NUM_GPIO_PINS, -EINVAL);

	QM_GPIO[gpio]->gpio_swporta_dr |= (1 << pin);

	return 0;
}

int qm_gpio_clear_pin(const qm_gpio_t gpio, const uint8_t pin)
{
	QM_CHECK(gpio < QM_GPIO_NUM, -EINVAL);
	QM_CHECK(pin <= QM_NUM_GPIO_PINS, -EINVAL);

	QM_GPIO[gpio]->gpio_swporta_dr &= ~(1 << pin);

	return 0;
}

int qm_gpio_set_pin_state(const qm_gpio_t gpio, const uint8_t pin,
			  const qm_gpio_state_t state)
{
	QM_CHECK(gpio < QM_GPIO_NUM, -EINVAL);
	QM_CHECK(pin <= QM_NUM_GPIO_PINS, -EINVAL);
	QM_CHECK(state < QM_GPIO_STATE_NUM, -EINVAL);

	uint32_t reg = QM_GPIO[gpio]->gpio_swporta_dr;
	reg ^= (-state ^ reg) & (1 << pin);
	QM_GPIO[gpio]->gpio_swporta_dr = reg;

	return 0;
}

int qm_gpio_read_port(const qm_gpio_t gpio, uint32_t *const port)
{
	QM_CHECK(gpio < QM_GPIO_NUM, -EINVAL);
	QM_CHECK(port != NULL, -EINVAL);

	*port = QM_GPIO[gpio]->gpio_ext_porta;

	return 0;
}

int qm_gpio_write_port(const qm_gpio_t gpio, const uint32_t val)
{
	QM_CHECK(gpio < QM_GPIO_NUM, -EINVAL);

	QM_GPIO[gpio]->gpio_swporta_dr = val;

	return 0;
}

#if (ENABLE_RESTORE_CONTEXT)
int qm_gpio_save_context(const qm_gpio_t gpio, qm_gpio_context_t *const ctx)
{
	QM_CHECK(gpio < QM_GPIO_NUM, -EINVAL);
	QM_CHECK(ctx != NULL, -EINVAL);

	qm_gpio_reg_t *const controller = QM_GPIO[gpio];

	if (gpio == QM_GPIO_0) {
		ctx->gpio_swporta_dr = controller->gpio_swporta_dr;
		ctx->gpio_swporta_ddr = controller->gpio_swporta_ddr;
		ctx->gpio_swporta_ctl = controller->gpio_swporta_ctl;
		ctx->gpio_inten = controller->gpio_inten;
		ctx->gpio_intmask = controller->gpio_intmask;
		ctx->gpio_inttype_level = controller->gpio_inttype_level;
		ctx->gpio_int_polarity = controller->gpio_int_polarity;
		ctx->gpio_debounce = controller->gpio_debounce;
		ctx->gpio_ls_sync = controller->gpio_ls_sync;
		ctx->gpio_int_bothedge = controller->gpio_int_bothedge;
	}

	return 0;
}

int qm_gpio_restore_context(const qm_gpio_t gpio,
			    const qm_gpio_context_t *const ctx)
{
	QM_CHECK(gpio < QM_GPIO_NUM, -EINVAL);
	QM_CHECK(ctx != NULL, -EINVAL);

	qm_gpio_reg_t *const controller = QM_GPIO[gpio];

	if (gpio == QM_GPIO_0) {
		controller->gpio_intmask = 0xffffffff;
		controller->gpio_swporta_dr = ctx->gpio_swporta_dr;
		controller->gpio_swporta_ddr = ctx->gpio_swporta_ddr;
		controller->gpio_swporta_ctl = ctx->gpio_swporta_ctl;
		controller->gpio_inten = ctx->gpio_inten;
		controller->gpio_inttype_level = ctx->gpio_inttype_level;
		controller->gpio_int_polarity = ctx->gpio_int_polarity;
		controller->gpio_debounce = ctx->gpio_debounce;
		controller->gpio_ls_sync = ctx->gpio_ls_sync;
		controller->gpio_int_bothedge = ctx->gpio_int_bothedge;
		controller->gpio_intmask = ctx->gpio_intmask;
	}

	return 0;
}
#else
int qm_gpio_save_context(const qm_gpio_t gpio, qm_gpio_context_t *const ctx)
{
	(void)gpio;
	(void)ctx;

	return 0;
}

int qm_gpio_restore_context(const qm_gpio_t gpio,
			    const qm_gpio_context_t *const ctx)
{
	(void)gpio;
	(void)ctx;

	return 0;
}
#endif /* ENABLE_RESTORE_CONTEXT */
