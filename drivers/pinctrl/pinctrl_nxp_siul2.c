/*
 * Copyright 2022, 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/pinctrl.h>

/* SIUL2 Multiplexed Signal Configuration Register */
#define SIUL2_MSCR(n) (0x240 + 0x4 * (n))
/* SIUL2 Input Multiplexed Signal Configuration Register */
#define SIUL2_IMCR(n) (0xa40 + 0x4 * (n))

#define SIUL2_MSCR_MAX_IDX 512U
#define SIUL2_IMCR_MAX_IDX 512U

/*
 * Utility macro that expands to the SIUL2 base address if it exists or zero.
 * Note that some devices may have instance gaps, hence the need to keep them in the array.
 */
#define SIUL2_BASE_OR_ZERO(nodelabel)                                                              \
	COND_CODE_1(DT_NODE_EXISTS(DT_NODELABEL(nodelabel)),                                       \
		    (DT_REG_ADDR(DT_NODELABEL(nodelabel))), (0U))

static mem_addr_t siul2_bases[] = {
	SIUL2_BASE_OR_ZERO(siul2_0), SIUL2_BASE_OR_ZERO(siul2_1), SIUL2_BASE_OR_ZERO(siul2_2),
	SIUL2_BASE_OR_ZERO(siul2_3), SIUL2_BASE_OR_ZERO(siul2_4), SIUL2_BASE_OR_ZERO(siul2_5),
};

static void pinctrl_configure_pin(const pinctrl_soc_pin_t *pin)
{
	mem_addr_t base;

	/* Multiplexed Signal Configuration */
	__ASSERT_NO_MSG(pin->mscr.inst < ARRAY_SIZE(siul2_bases));
	base = siul2_bases[pin->mscr.inst];
	__ASSERT_NO_MSG(base != 0);

	__ASSERT_NO_MSG(pin->mscr.idx < SIUL2_MSCR_MAX_IDX);
	sys_write32(pin->mscr.val, (base + SIUL2_MSCR(pin->mscr.idx)));

	/* Input Multiplexed Signal Configuration */
	if (pin->mscr.val & SIUL2_MSCR_IBE_MASK) {
		__ASSERT_NO_MSG(pin->imcr.inst < ARRAY_SIZE(siul2_bases));
		base = siul2_bases[pin->imcr.inst];
		__ASSERT_NO_MSG(base != 0);

		__ASSERT_NO_MSG(pin->imcr.idx < SIUL2_IMCR_MAX_IDX);
		sys_write32(pin->imcr.val, (base + SIUL2_IMCR(pin->imcr.idx)));
	}
}

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, uintptr_t reg)
{
	ARG_UNUSED(reg);

	for (uint8_t i = 0; i < pin_cnt; i++) {
		pinctrl_configure_pin(pins++);
	}

	return 0;
}
