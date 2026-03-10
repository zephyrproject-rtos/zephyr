/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define CPU_CLOCK 64000000U
#define FAST_BITBANG_HW_SUPPORT 1

static ALWAYS_INLINE void swdp_ll_pin_input(void *const base, uint8_t pin)
{
	NRF_GPIO_Type *reg = base;

	reg->PIN_CNF[pin] = 0b0000;
}

static ALWAYS_INLINE void swdp_ll_pin_output(void *const base, uint8_t pin)
{
	NRF_GPIO_Type *reg = base;

	reg->PIN_CNF[pin] = 0b0001;
}

static ALWAYS_INLINE void swdp_ll_pin_set(void *const base, uint8_t pin)
{
	NRF_GPIO_Type *reg = base;

	reg->OUTSET = BIT(pin);
}

static ALWAYS_INLINE void swdp_ll_pin_clr(void *const base, uint8_t pin)
{
	NRF_GPIO_Type *reg = base;

	reg->OUTCLR = BIT(pin);
}

static ALWAYS_INLINE uint32_t swdp_ll_pin_get(void *const base, uint8_t pin)
{
	NRF_GPIO_Type *reg = base;

	return ((reg->IN >> pin) & 1);
}
