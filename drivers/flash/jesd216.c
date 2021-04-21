/*
 * Copyright (c) 2020 Peter Bigot Consulting, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <sys/types.h>
#include <kernel.h>
#include "jesd216.h"
#include "spi_nor.h"

static bool extract_instr(uint16_t packed,
			  struct jesd216_instr *res)
{
	bool rv = (res != NULL);

	if (rv) {
		res->instr = packed >> 8;
		res->mode_clocks = (packed >> 5) & 0x07;
		res->wait_states = packed & 0x1F;
	}
	return rv;
}

int jesd216_bfp_read_support(const struct jesd216_param_header *php,
			     const struct jesd216_bfp *bfp,
			     enum jesd216_mode_type mode,
			     struct jesd216_instr *res)
{
	int rv = -ENOTSUP;

	switch (mode) {
	case JESD216_MODE_044:
		if ((php->len_dw >= 15)
		    && (sys_le32_to_cpu(bfp->dw10[5]) & BIT(9))) {
			rv = 0;
		}
		break;
	case JESD216_MODE_088:
		if ((php->len_dw >= 19)
		    && (sys_le32_to_cpu(bfp->dw10[9]) & BIT(9))) {
			rv = 0;
		}
		break;
	case JESD216_MODE_111:
		rv = 0;
		break;
	case JESD216_MODE_112:
		if (sys_le32_to_cpu(bfp->dw1) & BIT(16)) {
			uint32_t dw4 = sys_le32_to_cpu(bfp->dw4);

			rv = extract_instr(dw4 >> 0, res);
		}
		break;
	case JESD216_MODE_114:
		if (sys_le32_to_cpu(bfp->dw1) & BIT(22)) {
			uint32_t dw3 = sys_le32_to_cpu(bfp->dw3);

			rv = extract_instr(dw3 >> 16, res);
		}
		break;
	case JESD216_MODE_118:
		if (php->len_dw >= 17) {
			uint32_t dw17 = sys_le32_to_cpu(bfp->dw10[7]);

			if ((dw17 >> 24) != 0) {
				rv = extract_instr(dw17 >> 16, res);
			}
		}
		break;
	case JESD216_MODE_122:
		if (sys_le32_to_cpu(bfp->dw1) & BIT(20)) {
			uint32_t dw4 = sys_le32_to_cpu(bfp->dw4);

			rv = extract_instr(dw4 >> 16, res);
		}
		break;
	case JESD216_MODE_144:
		if (sys_le32_to_cpu(bfp->dw1) & BIT(21)) {
			uint32_t dw3 = sys_le32_to_cpu(bfp->dw3);

			rv = extract_instr(dw3 >> 0, res);
		}
		break;
	case JESD216_MODE_188:
		if (php->len_dw >= 17) {
			uint32_t dw17 = sys_le32_to_cpu(bfp->dw10[7]);

			if ((uint8_t)(dw17 >> 8) != 0) {
				rv = extract_instr(dw17 >> 0, res);
			}
		}
		break;
	case JESD216_MODE_222:
		if (sys_le32_to_cpu(bfp->dw5) & BIT(0)) {
			uint32_t dw6 = sys_le32_to_cpu(bfp->dw6);

			rv = extract_instr(dw6 >> 16, res);
		}
		break;
	case JESD216_MODE_444:
		if (sys_le32_to_cpu(bfp->dw5) & BIT(4)) {
			uint32_t dw7 = sys_le32_to_cpu(bfp->dw7);

			rv = extract_instr(dw7 >> 16, res);
		}
		break;
	/* Not clear how to detect these; they are identified only by
	 * enable/disable sequences.
	 */
	case JESD216_MODE_44D4D:
	case JESD216_MODE_888:
	case JESD216_MODE_8D8D8D:
		break;
	default:
		rv = -EINVAL;
	}

	return rv;
}

int jesd216_bfp_erase(const struct jesd216_bfp *bfp,
		       uint8_t idx,
		       struct jesd216_erase_type *etp)
{
	__ASSERT_NO_MSG((idx > 0) && (idx <= JESD216_NUM_ERASE_TYPES));

	/* Types 1 and 2 are in dw8, types 3 and 4 in dw9 */
	const uint32_t *dwp = &bfp->dw8 + (idx - 1U) / 2U;
	uint32_t dw = sys_le32_to_cpu(*dwp);

	/* Type 2(4) is in the upper half of the value. */
	if ((idx & 0x01) == 0x00) {
		dw >>= 16;
	}

	/* Extract the exponent and command */
	uint8_t exp = (uint8_t)dw;
	uint8_t cmd = (uint8_t)(dw >> 8);

	if (exp == 0) {
		return -EINVAL;
	}
	etp->cmd = cmd;
	etp->exp = exp;
	return 0;
}

int jesd216_bfp_erase_type_times(const struct jesd216_param_header *php,
				 const struct jesd216_bfp *bfp,
				 uint8_t idx,
				 uint32_t *typ_ms)
{
	__ASSERT_NO_MSG((idx > 0) && (idx <= JESD216_NUM_ERASE_TYPES));

	/* DW10 introduced in JESD216A */
	if (php->len_dw < 10) {
		return -ENOTSUP;
	}

	uint32_t dw10 = sys_le32_to_cpu(bfp->dw10[0]);

	/* Each 7-bit erase time entry has a 5-bit count in the lower
	 * bits, and a 2-bit unit in the upper bits.  The actual count
	 * is the field content plus one.
	 *
	 * The entries start with ET1 at bit 4.  The low four bits
	 * encode a value that is offset and scaled to produce a
	 * multipler to convert from typical time to maximum time.
	 */
	unsigned int count = 1 + ((dw10 >> (4 + (idx - 1) * 7)) & 0x1F);
	unsigned int units = ((dw10 >> (4 + 5 + (idx - 1) * 7)) & 0x03);
	unsigned int max_factor = 2 * (1 + (dw10 & 0x0F));

	switch (units) {
	case 0x00:		/* 1 ms */
		*typ_ms = count;
		break;
	case 0x01:		/* 16 ms */
		*typ_ms = count * 16;
		break;
	case 0x02:		/* 128 ms */
		*typ_ms = count * 128;
		break;
	case 0x03:		/* 1 s */
		*typ_ms = count * MSEC_PER_SEC;
		break;
	}

	return max_factor;
}

int jesd216_bfp_decode_dw11(const struct jesd216_param_header *php,
			    const struct jesd216_bfp *bfp,
			    struct jesd216_bfp_dw11 *res)
{
	/* DW11 introduced in JESD216A */
	if (php->len_dw < 11) {
		return -ENOTSUP;
	}

	uint32_t dw11 = sys_le32_to_cpu(bfp->dw10[1]);
	uint32_t value = 1 + ((dw11 >> 24) & 0x1F);

	switch ((dw11 >> 29) & 0x03) {
	case 0x00: /* 16 ms */
		value *= 16;
		break;
	case 0x01:
		value *= 256;
		break;
	case 0x02:
		value *= 4 * MSEC_PER_SEC;
		break;
	case 0x03:
		value *= 64 * MSEC_PER_SEC;
		break;
	}
	res->chip_erase_ms = value;

	value = 1 + ((dw11 >> 19) & 0x0F);
	if (dw11 & BIT(23)) {
		value *= 8;
	}
	res->byte_prog_addl_us = value;

	value = 1 + ((dw11 >> 14) & 0x0F);
	if (dw11 & BIT(18)) {
		value *= 8;
	}
	res->byte_prog_first_us = value;

	value = 1 + ((dw11 >> 8) & 0x01F);
	if (dw11 & BIT(13)) {
		value *= 64;
	} else {
		value *= 8;
	}
	res->page_prog_us = value;

	res->page_size = BIT((dw11 >> 4) & 0x0F);
	res->typ_max_factor = 2 * (1 + (dw11 & 0x0F));

	return 0;
}

int jesd216_bfp_decode_dw14(const struct jesd216_param_header *php,
			    const struct jesd216_bfp *bfp,
			    struct jesd216_bfp_dw14 *res)
{
	/* DW14 introduced in JESD216A */
	if (php->len_dw < 14) {
		return -ENOTSUP;
	}

	uint32_t dw14 = sys_le32_to_cpu(bfp->dw10[4]);

	if (dw14 & BIT(31)) {
		return -ENOTSUP;
	}

	res->enter_dpd_instr = (dw14 >> 23) & 0xFF;
	res->exit_dpd_instr = (dw14 >> 15) & 0xFF;

	uint32_t value = 1 + ((dw14 >> 8) & 0x1F);

	switch ((dw14 >> 13) & 0x03) {
	case 0x00: /* 128 ns */
		value *= 128;
		break;
	case 0x01: /* 1 us */
		value *= NSEC_PER_USEC;
		break;
	case 0x02: /* 8 us */
		value *= 8 * NSEC_PER_USEC;
		break;
	case 0x03: /* 64 us */
		value *= 64 * NSEC_PER_USEC;
		break;
	}

	res->exit_delay_ns = value;

	res->poll_options = (dw14 >> 2) & 0x3F;

	return 0;
}

int jesd216_bfp_decode_dw15(const struct jesd216_param_header *php,
			    const struct jesd216_bfp *bfp,
			    struct jesd216_bfp_dw15 *res)
{
	/* DW15 introduced in JESD216A */
	if (php->len_dw < 15) {
		return -ENOTSUP;
	}

	uint32_t dw15 = sys_le32_to_cpu(bfp->dw10[5]);

	res->hold_reset_disable = (dw15 & BIT(23)) != 0U;
	res->qer = (dw15 >> 20) & 0x07;
	res->entry_044 = (dw15 >> 16) & 0x0F;
	res->exit_044 = (dw15 >> 10) & 0x3F;
	res->support_044 = (dw15 & BIT(9)) != 0U;
	res->enable_444 = (dw15 >> 4) & 0x1F;
	res->disable_444 = (dw15 >> 0) & 0x0F;

	return 0;
}

int jesd216_bfp_decode_dw16(const struct jesd216_param_header *php,
			    const struct jesd216_bfp *bfp,
			    struct jesd216_bfp_dw16 *res)
{
	/* DW16 introduced in JESD216A */
	if (php->len_dw < 16) {
		return -ENOTSUP;
	}

	uint32_t dw16 = sys_le32_to_cpu(bfp->dw10[6]);

	res->enter_4ba = (dw16 >> 24) & 0xFF;
	res->exit_4ba = (dw16 >> 14) & 0x3FF;
	res->srrs_support = (dw16 >> 8) & 0x3F;
	res->sr1_interface = (dw16 >> 0) & 0x7F;

	return 0;
}
