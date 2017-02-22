/*
 * Copyright (c) 2017, Intel Corporation
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

#include "qm_ss_gpio.h"

static void (*callback[QM_SS_GPIO_NUM])(void *data, uint32_t int_status);
static void *callback_data[QM_SS_GPIO_NUM];

static uint32_t gpio_base[QM_SS_GPIO_NUM] = {QM_SS_GPIO_0_BASE,
					     QM_SS_GPIO_1_BASE};

static void ss_gpio_isr_handler(qm_ss_gpio_t gpio)
{
	uint32_t int_status = 0;
	uint32_t controller = gpio_base[gpio];

	int_status = __builtin_arc_lr(controller + QM_SS_GPIO_INTSTATUS);

	if (callback[gpio]) {
		callback[gpio](callback_data[gpio], int_status);
	}

	__builtin_arc_sr(int_status, controller + QM_SS_GPIO_PORTA_EOI);
}

QM_ISR_DECLARE(qm_ss_gpio_0_isr)
{
	ss_gpio_isr_handler(QM_SS_GPIO_0);
}

QM_ISR_DECLARE(qm_ss_gpio_1_isr)
{
	ss_gpio_isr_handler(QM_SS_GPIO_1);
}

int qm_ss_gpio_set_config(const qm_ss_gpio_t gpio,
			  const qm_ss_gpio_port_config_t *const cfg)
{
	uint32_t controller, reg_ls_sync_;

	QM_CHECK(gpio < QM_SS_GPIO_NUM, -EINVAL);
	QM_CHECK(cfg != NULL, -EINVAL);

	controller = gpio_base[gpio];

#if (HAS_SS_GPIO_CLK_ENABLE)
	/*
	 * SS GPIO Clock gate (CLKEN) is enabled here, because it is local to
	 * the peripheral block and not a part of the SoC power management clock
	 * gating.
	 */
	__builtin_arc_sr(BIT(0), controller + QM_SS_GPIO_CLKEN);
#endif /* HAS_SS_GPIO_CLK_ENABLE */

	__builtin_arc_sr(0xFFFFFFFF, controller + QM_SS_GPIO_INTMASK);

	__builtin_arc_sr(cfg->direction, controller + QM_SS_GPIO_SWPORTA_DDR);
	__builtin_arc_sr(cfg->int_type, controller + QM_SS_GPIO_INTTYPE_LEVEL);
	__builtin_arc_sr(cfg->int_polarity,
			 controller + QM_SS_GPIO_INT_POLARITY);
	__builtin_arc_sr(cfg->int_debounce, controller + QM_SS_GPIO_DEBOUNCE);

#if (HAS_SS_GPIO_INTERRUPT_BOTHEDGE)
	__builtin_arc_sr(cfg->int_bothedge,
			 controller + QM_SS_GPIO_INT_BOTHEDGE);
#endif /* HAS_SS_GPIO_INTERRUPT_BOTHEDGE */

	callback[gpio] = cfg->callback;
	callback_data[gpio] = cfg->callback_data;

	/* Synchronize the level-sensitive interrupts to pclk_intr. */
	reg_ls_sync_ = __builtin_arc_lr(gpio_base[gpio] + QM_SS_GPIO_LS_SYNC);
	__builtin_arc_sr(reg_ls_sync_ | BIT(0),
			 controller + QM_SS_GPIO_LS_SYNC);

	__builtin_arc_sr(cfg->int_en, controller + QM_SS_GPIO_INTEN);

	__builtin_arc_sr(~cfg->int_en, controller + QM_SS_GPIO_INTMASK);

	return 0;
}

int qm_ss_gpio_read_pin(const qm_ss_gpio_t gpio, const uint8_t pin,
			qm_ss_gpio_state_t *const state)
{
	QM_CHECK(gpio < QM_SS_GPIO_NUM, -EINVAL);
	QM_CHECK(pin <= QM_SS_GPIO_NUM_PINS, -EINVAL);
	QM_CHECK(state != NULL, -EINVAL);

	*state =
	    ((__builtin_arc_lr(gpio_base[gpio] + QM_SS_GPIO_EXT_PORTA) >> pin) &
	     1);

	return 0;
}

int qm_ss_gpio_set_pin(const qm_ss_gpio_t gpio, const uint8_t pin)
{
	uint32_t val;
	QM_CHECK(gpio < QM_SS_GPIO_NUM, -EINVAL);
	QM_CHECK(pin <= QM_SS_GPIO_NUM_PINS, -EINVAL);

	val = __builtin_arc_lr(gpio_base[gpio] + QM_SS_GPIO_SWPORTA_DR) |
	      BIT(pin);
	__builtin_arc_sr(val, gpio_base[gpio] + QM_SS_GPIO_SWPORTA_DR);

	return 0;
}

int qm_ss_gpio_clear_pin(const qm_ss_gpio_t gpio, const uint8_t pin)
{
	uint32_t val;
	QM_CHECK(gpio < QM_SS_GPIO_NUM, -EINVAL);
	QM_CHECK(pin <= QM_SS_GPIO_NUM_PINS, -EINVAL);

	val = __builtin_arc_lr(gpio_base[gpio] + QM_SS_GPIO_SWPORTA_DR);
	val &= ~BIT(pin);
	__builtin_arc_sr(val, gpio_base[gpio] + QM_SS_GPIO_SWPORTA_DR);

	return 0;
}

int qm_ss_gpio_set_pin_state(const qm_ss_gpio_t gpio, const uint8_t pin,
			     const qm_ss_gpio_state_t state)
{
	uint32_t val;
	QM_CHECK(gpio < QM_SS_GPIO_NUM, -EINVAL);
	QM_CHECK(state < QM_SS_GPIO_STATE_NUM, -EINVAL);

	val = __builtin_arc_lr(gpio_base[gpio] + QM_SS_GPIO_SWPORTA_DR);
	val ^= (-state ^ val) & (1 << pin);
	__builtin_arc_sr(val, gpio_base[gpio] + QM_SS_GPIO_SWPORTA_DR);

	return 0;
}

int qm_ss_gpio_read_port(const qm_ss_gpio_t gpio, uint32_t *const port)
{
	QM_CHECK(gpio < QM_SS_GPIO_NUM, -EINVAL);
	QM_CHECK(port != NULL, -EINVAL);

	*port = (__builtin_arc_lr(gpio_base[gpio] + QM_SS_GPIO_EXT_PORTA));

	return 0;
}

int qm_ss_gpio_write_port(const qm_ss_gpio_t gpio, const uint32_t val)
{
	QM_CHECK(gpio < QM_SS_GPIO_NUM, -EINVAL);

	__builtin_arc_sr(val, gpio_base[gpio] + QM_SS_GPIO_SWPORTA_DR);

	return 0;
}

#if (ENABLE_RESTORE_CONTEXT)
int qm_ss_gpio_save_context(const qm_ss_gpio_t gpio,
			    qm_ss_gpio_context_t *const ctx)
{
	uint32_t controller;

	QM_CHECK(gpio < QM_SS_GPIO_NUM, -EINVAL);
	QM_CHECK(ctx != NULL, -EINVAL);

	controller = gpio_base[gpio];

	ctx->gpio_swporta_dr =
	    __builtin_arc_lr(controller + QM_SS_GPIO_SWPORTA_DR);
	ctx->gpio_swporta_ddr =
	    __builtin_arc_lr(controller + QM_SS_GPIO_SWPORTA_DDR);
	ctx->gpio_inten = __builtin_arc_lr(controller + QM_SS_GPIO_INTEN);
	ctx->gpio_intmask = __builtin_arc_lr(controller + QM_SS_GPIO_INTMASK);
	ctx->gpio_inttype_level =
	    __builtin_arc_lr(controller + QM_SS_GPIO_INTTYPE_LEVEL);
	ctx->gpio_int_polarity =
	    __builtin_arc_lr(controller + QM_SS_GPIO_INT_POLARITY);
	ctx->gpio_debounce = __builtin_arc_lr(controller + QM_SS_GPIO_DEBOUNCE);
	ctx->gpio_ls_sync = __builtin_arc_lr(controller + QM_SS_GPIO_LS_SYNC);

	return 0;
}

int qm_ss_gpio_restore_context(const qm_ss_gpio_t gpio,
			       const qm_ss_gpio_context_t *const ctx)
{
	uint32_t controller;

	QM_CHECK(gpio < QM_SS_GPIO_NUM, -EINVAL);
	QM_CHECK(ctx != NULL, -EINVAL);

	controller = gpio_base[gpio];

	__builtin_arc_sr(0xffffffff, controller + QM_SS_GPIO_INTMASK);
	__builtin_arc_sr(ctx->gpio_swporta_dr,
			 controller + QM_SS_GPIO_SWPORTA_DR);
	__builtin_arc_sr(ctx->gpio_swporta_ddr,
			 controller + QM_SS_GPIO_SWPORTA_DDR);
	__builtin_arc_sr(ctx->gpio_inten, controller + QM_SS_GPIO_INTEN);
	__builtin_arc_sr(ctx->gpio_inttype_level,
			 controller + QM_SS_GPIO_INTTYPE_LEVEL);
	__builtin_arc_sr(ctx->gpio_int_polarity,
			 controller + QM_SS_GPIO_INT_POLARITY);
	__builtin_arc_sr(ctx->gpio_debounce, controller + QM_SS_GPIO_DEBOUNCE);
	__builtin_arc_sr(ctx->gpio_ls_sync, controller + QM_SS_GPIO_LS_SYNC);
	__builtin_arc_sr(ctx->gpio_intmask, controller + QM_SS_GPIO_INTMASK);

	return 0;
}
#else
int qm_ss_gpio_save_context(const qm_ss_gpio_t gpio,
			    qm_ss_gpio_context_t *const ctx)
{
	(void)gpio;
	(void)ctx;

	return 0;
}

int qm_ss_gpio_restore_context(const qm_ss_gpio_t gpio,
			       const qm_ss_gpio_context_t *const ctx)
{
	(void)gpio;
	(void)ctx;

	return 0;
}
#endif /* ENABLE_RESTORE_CONTEXT */
