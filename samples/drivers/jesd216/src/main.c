/*
 * Copyright (c) 2020 Peter Bigot Consulting, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <jesd216.h>
#include <stdio.h>
#include <inttypes.h>
#include <string.h>

#if DT_HAS_COMPAT_STATUS_OKAY(jedec_spi_nor)
#define FLASH_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(jedec_spi_nor)
#elif DT_HAS_COMPAT_STATUS_OKAY(nordic_qspi_nor)
#define FLASH_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(nordic_qspi_nor)
#elif DT_HAS_COMPAT_STATUS_OKAY(st_stm32_qspi_nor)
#define FLASH_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(st_stm32_qspi_nor)
#elif DT_HAS_COMPAT_STATUS_OKAY(st_stm32_ospi_nor)
#define FLASH_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(st_stm32_ospi_nor)
#else
#error Unsupported flash driver
#define FLASH_NODE DT_INVALID_NODE
#endif

typedef void (*dw_extractor)(const struct jesd216_param_header *php,
			     const struct jesd216_bfp *bfp);

static const char * const mode_tags[] = {
	[JESD216_MODE_044] = "QSPI XIP",
	[JESD216_MODE_088] = "OSPI XIP",
	[JESD216_MODE_111] = "1-1-1",
	[JESD216_MODE_112] = "1-1-2",
	[JESD216_MODE_114] = "1-1-4",
	[JESD216_MODE_118] = "1-1-8",
	[JESD216_MODE_122] = "1-2-2",
	[JESD216_MODE_144] = "1-4-4",
	[JESD216_MODE_188] = "1-8-8",
	[JESD216_MODE_222] = "2-2-2",
	[JESD216_MODE_444] = "4-4-4",
};

static void summarize_dw1(const struct jesd216_param_header *php,
			  const struct jesd216_bfp *bfp)
{
	uint32_t dw1 = sys_le32_to_cpu(bfp->dw1);

	printf("DTR Clocking %ssupported\n",
	       (dw1 & JESD216_SFDP_BFP_DW1_DTRCLK_FLG) ? "" : "not ");

	static const char *const addr_bytes[] = {
		[JESD216_SFDP_BFP_DW1_ADDRBYTES_VAL_3B] = "3-Byte only",
		[JESD216_SFDP_BFP_DW1_ADDRBYTES_VAL_3B4B] = "3- or 4-Byte",
		[JESD216_SFDP_BFP_DW1_ADDRBYTES_VAL_4B] = "4-Byte only",
		[3] = "Reserved",
	};

	printf("Addressing: %s\n", addr_bytes[(dw1 & JESD216_SFDP_BFP_DW1_ADDRBYTES_MASK)
					      >> JESD216_SFDP_BFP_DW1_ADDRBYTES_SHFT]);

	static const char *const bsersz[] = {
		[0] = "Reserved 00b",
		[JESD216_SFDP_BFP_DW1_BSERSZ_VAL_4KSUP] = "uniform",
		[2] = "Reserved 01b",
		[JESD216_SFDP_BFP_DW1_BSERSZ_VAL_4KNOTSUP] = "not uniform",
	};

	printf("4-KiBy erase: %s\n", bsersz[(dw1 & JESD216_SFDP_BFP_DW1_BSERSZ_MASK)
					    >> JESD216_SFDP_BFP_DW1_BSERSZ_SHFT]);

	for (size_t mode = 0; mode < ARRAY_SIZE(mode_tags); ++mode) {
		const char *tag = mode_tags[mode];

		if (tag) {
			struct jesd216_instr cmd;
			int rc = jesd216_bfp_read_support(php, bfp,
							  (enum jesd216_mode_type)mode,
							  &cmd);

			if (rc == 0) {
				printf("Support %s\n", tag);
			} else if (rc > 0) {
				printf("Support %s: instr %02Xh, %u mode clocks, %u waits\n",
				       tag, cmd.instr, cmd.mode_clocks, cmd.wait_states);
			}
		}
	}
}

static void summarize_dw2(const struct jesd216_param_header *php,
			  const struct jesd216_bfp *bfp)
{
	printf("Flash density: %u bytes\n", (uint32_t)(jesd216_bfp_density(bfp) / 8));
}

static void summarize_dw89(const struct jesd216_param_header *php,
			   const struct jesd216_bfp *bfp)
{
	struct jesd216_erase_type etype;
	uint32_t typ_ms;
	int typ_max_mul;

	for (uint8_t idx = 1; idx < JESD216_NUM_ERASE_TYPES; ++idx) {
		if (jesd216_bfp_erase(bfp, idx, &etype) == 0) {
			typ_max_mul = jesd216_bfp_erase_type_times(php, bfp,
								   idx, &typ_ms);

			printf("ET%u: instr %02Xh for %u By", idx, etype.cmd,
			       (uint32_t)BIT(etype.exp));
			if (typ_max_mul > 0) {
				printf("; typ %u ms, max %u ms",
				       typ_ms, typ_max_mul * typ_ms);
			}
			printf("\n");
		}
	}
}

static void summarize_dw11(const struct jesd216_param_header *php,
			   const struct jesd216_bfp *bfp)
{
	struct jesd216_bfp_dw11 dw11;

	if (jesd216_bfp_decode_dw11(php, bfp, &dw11) != 0) {
		return;
	}

	printf("Chip erase: typ %u ms, max %u ms\n",
	       dw11.chip_erase_ms, dw11.typ_max_factor * dw11.chip_erase_ms);

	printf("Byte program: type %u + %u * B us, max %u + %u * B us\n",
	       dw11.byte_prog_first_us, dw11.byte_prog_addl_us,
	       dw11.typ_max_factor * dw11.byte_prog_first_us,
	       dw11.typ_max_factor * dw11.byte_prog_addl_us);

	printf("Page program: typ %u us, max %u us\n",
	       dw11.page_prog_us,
	       dw11.typ_max_factor * dw11.page_prog_us);

	printf("Page size: %u By\n", dw11.page_size);
}

static void summarize_dw12(const struct jesd216_param_header *php,
			   const struct jesd216_bfp *bfp)
{
	uint32_t dw12 = sys_le32_to_cpu(bfp->dw10[2]);
	uint32_t dw13 = sys_le32_to_cpu(bfp->dw10[3]);

	/* Inverted logic flag: 1 means not supported */
	if ((dw12 & JESD216_SFDP_BFP_DW12_SUSPRESSUP_FLG) != 0) {
		return;
	}

	uint8_t susp_instr = dw13 >> 24;
	uint8_t resm_instr = dw13 >> 16;
	uint8_t psusp_instr = dw13 >> 8;
	uint8_t presm_instr = dw13 >> 0;

	printf("Suspend: %02Xh ; Resume: %02Xh\n",
	       susp_instr, resm_instr);
	if ((susp_instr != psusp_instr)
	    || (resm_instr != presm_instr)) {
		printf("Program suspend: %02Xh ; Resume: %02Xh\n",
		       psusp_instr, presm_instr);
	}
}

static void summarize_dw14(const struct jesd216_param_header *php,
			   const struct jesd216_bfp *bfp)
{
	struct jesd216_bfp_dw14 dw14;

	if (jesd216_bfp_decode_dw14(php, bfp, &dw14) != 0) {
		return;
	}
	printf("DPD: Enter %02Xh, exit %02Xh ; delay %u ns ; poll 0x%02x\n",
	       dw14.enter_dpd_instr, dw14.exit_dpd_instr,
	       dw14.exit_delay_ns, dw14.poll_options);
}

static void summarize_dw15(const struct jesd216_param_header *php,
			   const struct jesd216_bfp *bfp)
{
	struct jesd216_bfp_dw15 dw15;

	if (jesd216_bfp_decode_dw15(php, bfp, &dw15) != 0) {
		return;
	}
	printf("HOLD or RESET Disable: %ssupported\n",
	       dw15.hold_reset_disable ? "" : "un");
	printf("QER: %u\n", dw15.qer);
	if (dw15.support_044) {
		printf("0-4-4 Mode methods: entry 0x%01x ; exit 0x%02x\n",
		       dw15.entry_044, dw15.exit_044);
	} else {
		printf("0-4-4 Mode: not supported");
	}
	printf("4-4-4 Mode sequences: enable 0x%02x ; disable 0x%01x\n",
	       dw15.enable_444, dw15.disable_444);
}

static void summarize_dw16(const struct jesd216_param_header *php,
			   const struct jesd216_bfp *bfp)
{
	struct jesd216_bfp_dw16 dw16;

	if (jesd216_bfp_decode_dw16(php, bfp, &dw16) != 0) {
		return;
	}

	uint8_t addr_support = jesd216_bfp_addrbytes(bfp);

	/* Don't display bits when 4-byte addressing is not supported. */
	if (addr_support != JESD216_SFDP_BFP_DW1_ADDRBYTES_VAL_3B) {
		printf("4-byte addressing support: enter 0x%02x, exit 0x%03x\n",
		       dw16.enter_4ba, dw16.exit_4ba);
	}
	printf("Soft Reset and Rescue Sequence support: 0x%02x\n",
	       dw16.srrs_support);
	printf("Status Register 1 support: 0x%02x\n",
	       dw16.sr1_interface);
}

/* Indexed from 1 to match JESD216 data word numbering */
static const dw_extractor extractor[] = {
	[1] = summarize_dw1,
	[2] = summarize_dw2,
	[8] = summarize_dw89,
	[11] = summarize_dw11,
	[12] = summarize_dw12,
	[14] = summarize_dw14,
	[15] = summarize_dw15,
	[16] = summarize_dw16,
};

static void dump_bfp(const struct jesd216_param_header *php,
		     const struct jesd216_bfp *bfp)
{
	uint8_t dw = 1;
	uint8_t limit = MIN(1U + php->len_dw, ARRAY_SIZE(extractor));

	printf("Summary of BFP content:\n");
	while (dw < limit) {
		dw_extractor ext = extractor[dw];

		if (ext != 0) {
			ext(php, bfp);
		}
		++dw;
	}
}

static void dump_bytes(const struct jesd216_param_header *php,
		       const uint32_t *dw)
{
	char buffer[4 * 3 + 1]; /* B1 B2 B3 B4 */
	uint8_t nw = 0;

	printf(" [\n\t");
	while (nw < php->len_dw) {
		const uint8_t *u8p = (const uint8_t *)&dw[nw];
		++nw;

		bool emit_nl = (nw == php->len_dw) || ((nw % 4) == 0);

		sprintf(buffer, "%02x %02x %02x %02x",
			u8p[0], u8p[1], u8p[2], u8p[3]);
		if (emit_nl) {
			printf("%s\n\t", buffer);
		} else {
			printf("%s  ", buffer);
		}
	}
	printf("];\n");
}

int main(void)
{
	const struct device *const dev = DEVICE_DT_GET(FLASH_NODE);

	if (!device_is_ready(dev)) {
		printf("%s: device not ready\n", dev->name);
		return 0;
	}

	const uint8_t decl_nph = 5;
	union {
		uint8_t raw[JESD216_SFDP_SIZE(decl_nph)];
		struct jesd216_sfdp_header sfdp;
	} u;
	const struct jesd216_sfdp_header *hp = &u.sfdp;
	int rc = flash_sfdp_read(dev, 0, u.raw, sizeof(u.raw));

	if (rc != 0) {
		printf("Read SFDP not supported: device not JESD216-compliant "
		       "(err %d)\n", rc);
		return 0;
	}

	uint32_t magic = jesd216_sfdp_magic(hp);

	if (magic != JESD216_SFDP_MAGIC) {
		printf("SFDP magic %08x invalid", magic);
		return 0;
	}

	printf("%s: SFDP v %u.%u AP %x with %u PH\n", dev->name,
		hp->rev_major, hp->rev_minor, hp->access, 1 + hp->nph);

	const struct jesd216_param_header *php = hp->phdr;
	const struct jesd216_param_header *phpe = php + MIN(decl_nph, 1 + hp->nph);

	while (php != phpe) {
		uint16_t id = jesd216_param_id(php);
		uint32_t addr = jesd216_param_addr(php);

		printf("PH%u: %04x rev %u.%u: %u DW @ %x\n",
		       (uint32_t)(php - hp->phdr), id, php->rev_major, php->rev_minor,
		       php->len_dw, addr);

		uint32_t dw[php->len_dw];

		rc = flash_sfdp_read(dev, addr, dw, sizeof(dw));
		if (rc != 0) {
			printf("Read failed: %d\n", rc);
			return 0;
		}

		if (id == JESD216_SFDP_PARAM_ID_BFP) {
			const struct jesd216_bfp *bfp = (struct jesd216_bfp *)dw;

			dump_bfp(php, bfp);
			printf("size = <%u>;\n", (uint32_t)jesd216_bfp_density(bfp));
			printf("sfdp-bfp =");
		} else {
			printf("sfdp-%04x =", id);
		}

		dump_bytes(php, dw);

		++php;
	}

	uint8_t id[3];

	rc = flash_read_jedec_id(dev, id);
	if (rc == 0) {
		printf("jedec-id = [%02x %02x %02x];\n",
		       id[0], id[1], id[2]);
	} else {
		printf("JEDEC ID read failed: %d\n", rc);
	}
	return 0;
}
