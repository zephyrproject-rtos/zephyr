/*
 * Copyright (c) 2022 IoT.bzh
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <kernel.h>
#include <clock_soc.h>
#include <dt-bindings/clock/renesas/r8a7795-cpg-mssr.h>

/* Realtime Module Stop Control Register offsets */
const uint16_t rmstpcr[] = {
	0x110, 0x114, 0x118, 0x11c,
	0x120, 0x124, 0x128, 0x12c,
	0x980, 0x984, 0x988, 0x98c,
};

/* Software Reset Register offsets */
const uint16_t srcr[] = {
	0x0A0, 0x0A8, 0x0B0, 0x0B8,
	0x0BC, 0x0C4, 0x1C8, 0x1CC,
	0x920, 0x924, 0x928, 0x92C,
};

/* CAN-FD Clock Frequency Control Register */
#define CANFDCKCR                 0x244

/* Clock stop bit */
#define CANFDCKCR_CKSTP           BIT(8)

/* CANFD Clock */
#define CANFDCKCR_PARENT_CLK_RATE 800000000
#define CANFDCKCR_DIVIDER_MASK    0x1FF

#define S3D4_CLK_RATE             66600000 /* SCIF clock */

int soc_cpg_core_clock_endisable(uint32_t base_address, uint32_t module,
				 uint32_t rate, bool enable)
{
	uint32_t divider;
	unsigned int key;
	int ret;

	/* Only support CANFD core clock at the moment */
	if (module != R8A7795_CLK_CANFD) {
		return -EINVAL;
	}

	key = irq_lock();

	if (enable) {
		if ((CANFDCKCR_PARENT_CLK_RATE % rate) != 0) {
			__ASSERT(true, "Can not generate "
				 "%u from CANFD parent clock", rate);
			ret = -EINVAL;
			goto unlock;
		}
		divider = (CANFDCKCR_PARENT_CLK_RATE / rate) - 1;
		if (divider > CANFDCKCR_DIVIDER_MASK) {
			__ASSERT(true, "Can not generate %u "
				 "from CANFD parent clock", rate);
			ret = -EINVAL;
			goto unlock;
		}

		sys_write32(~divider, base_address + CPGWPR);
		sys_write32(divider, base_address + CANFDCKCR);
		/* Wait for at least one cycle of the RCLK clock (@ ca. 32 kHz) */
		k_sleep(K_USEC(35));
	} else {
		sys_write32(~CANFDCKCR_CKSTP, base_address + CPGWPR);
		sys_write32(CANFDCKCR_CKSTP, base_address + CANFDCKCR);
		/* Wait for at least one cycle of the RCLK clock (@ ca. 32 kHz) */
		k_sleep(K_USEC(35));
	}

unlock:
	irq_unlock(key);
	return ret;
}

int soc_cpg_get_rate(uint32_t base_address, uint32_t module, uint32_t *rate)
{
	int ret = 0;
	uint32_t val;

	switch (module) {
	case R8A7795_CLK_CANFD:
		val = sys_read32(base_address + CANFDCKCR);
		if (val & CANFDCKCR_CKSTP) {
			*rate = 0;
		} else {
			val &= CANFDCKCR_DIVIDER_MASK;
			*rate = CANFDCKCR_PARENT_CLK_RATE / (val + 1);
		}
		break;
	case R8A7795_CLK_S3D4:
		*rate = S3D4_CLK_RATE;
		break;
	default:
		ret = -ENOTSUP;
		break;
	}

	return ret;
}

int soc_cpg_get_mstpcr_offset(uint32_t reg, uint16_t *offset)
{
	if (reg > ARRAY_SIZE(rmstpcr)) {
		return -EINVAL;
	}

	*offset = rmstpcr[reg];

	return 0;
}

int soc_cpg_get_srcr_offset(uint32_t reg, uint16_t *offset)
{
	if (reg > ARRAY_SIZE(srcr)) {
		return -EINVAL;
	}

	*offset = srcr[reg];

	return 0;
}
