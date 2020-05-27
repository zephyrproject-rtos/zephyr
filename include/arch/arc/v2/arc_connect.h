/*
 * Copyright (c) 2019 Synopsys.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARCv2 ARC Connect driver
 *
 * ARCv2 ARC Connect driver interface. Included by arc/arch.h.
 */

#ifndef ZEPHYR_INCLUDE_ARCH_ARC_V2_ARC_CONNECT_H_
#define ZEPHYR_INCLUDE_ARCH_ARC_V2_ARC_CONNECT_H_

#ifndef _ASMLANGUAGE
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define _ARC_V2_CONNECT_BCR 0x0d0
#define _ARC_V2_CONNECT_IDU_BCR 0x0d5
#define _ARC_V2_CONNECT_GFRC_BCR 0x0d6
#define _ARC_V2_CONNECT_CMD 0x600
#define _ARC_V2_CONNECT_WDATA 0x601
#define _ARC_V2_CONNECT_READBACK 0x602


#define ARC_CONNECT_CMD_CHECK_CORE_ID			0x0

#define ARC_CONNECT_CMD_INTRPT_GENERATE_IRQ		0x1
#define ARC_CONNECT_CMD_INTRPT_GENERATE_ACK		0x2
#define ARC_CONNECT_CMD_INTRPT_READ_STATUS		0x3
#define ARC_CONNECT_CMD_INTRPT_CHECK_SOURCE		0x4

#define ARC_CONNECT_CMD_SEMA_CLAIM_AND_READ		0x11
#define ARC_CONNECT_CMD_SEMA_RELEASE			0x12
#define ARC_CONNECT_CMD_SEMA_FORCE_RELEASE		0x13

#define ARC_CONNECT_CMD_MSG_SRAM_SET_ADDR		0x21
#define ARC_CONNECT_CMD_MSG_SRAM_READ_ADDR		0x22
#define ARC_CONNECT_CMD_MSG_SRAM_SET_ADDR_OFFSET	0x23
#define ARC_CONNECT_CMD_MSG_SRAM_READ_ADDR_OFFSET	0x24
#define ARC_CONNECT_CMD_MSG_SRAM_WRITE			0x25
#define ARC_CONNECT_CMD_MSG_SRAM_WRITE_INC		0x26
#define ARC_CONNECT_CMD_MSG_SRAM_WRITE_IMM		0x27
#define ARC_CONNECT_CMD_MSG_SRAM_READ			0x28
#define ARC_CONNECT_CMD_MSG_SRAM_READ_INC		0x29
#define ARC_CONNECT_CMD_MSG_SRAM_READ_IMM		0x2a
#define ARC_CONNECT_CMD_MSG_SRAM_SET_ECC_CTRL		0x2b
#define ARC_CONNECT_CMD_MSG_SRAM_READ_ECC_CTRL		0x2c

#define ARC_CONNECT_CMD_DEBUG_RESET			0x31
#define ARC_CONNECT_CMD_DEBUG_HALT			0x32
#define ARC_CONNECT_CMD_DEBUG_RUN			0x33
#define ARC_CONNECT_CMD_DEBUG_SET_MASK			0x34
#define ARC_CONNECT_CMD_DEBUG_READ_MASK			0x35
#define ARC_CONNECT_CMD_DEBUG_SET_SELECT		0x36
#define ARC_CONNECT_CMD_DEBUG_READ_SELECT		0x37
#define ARC_CONNECT_CMD_DEBUG_READ_EN			0x38
#define ARC_CONNECT_CMD_DEBUG_READ_CMD			0x39
#define ARC_CONNECT_CMD_DEBUG_READ_CORE			0x3a

#define ARC_CONNECT_CMD_GFRC_CLEAR			0x41
#define ARC_CONNECT_CMD_GFRC_READ_LO			0x42
#define ARC_CONNECT_CMD_GFRC_READ_HI			0x43
#define ARC_CONNECT_CMD_GFRC_ENABLE			0x44
#define ARC_CONNECT_CMD_GFRC_DISABLE			0x45
#define ARC_CONNECT_CMD_GFRC_READ_DISABLE		0x46
#define ARC_CONNECT_CMD_GFRC_SET_CORE			0x47
#define ARC_CONNECT_CMD_GFRC_READ_CORE			0x48
#define ARC_CONNECT_CMD_GFRC_READ_HALT			0x49

#define ARC_CONNECT_CMD_PDM_SET_PM			0x81
#define ARC_CONNECT_CMD_PDM_READ_PSTATUS		0x82

#define ARC_CONNECT_CMD_PMU_SET_PUCNT			0x51
#define ARC_CONNECT_CMD_PMU_READ_PUCNT			0x52
#define ARC_CONNECT_CMD_PMU_SET_RSTCNT			0x53
#define ARC_CONNECT_CMD_PMU_READ_RSTCNT			0x54
#define ARC_CONNECT_CMD_PMU_SET_PDCNT			0x55
#define ARC_CONNECT_CMD_PMU_READ_PDCNT			0x56

#define ARC_CONNECT_CMD_IDU_ENABLE			0x71
#define ARC_CONNECT_CMD_IDU_DISABLE			0x72
#define ARC_CONNECT_CMD_IDU_READ_ENABLE			0x73
#define ARC_CONNECT_CMD_IDU_SET_MODE			0x74
#define ARC_CONNECT_CMD_IDU_READ_MODE			0x75
#define ARC_CONNECT_CMD_IDU_SET_DEST			0x76
#define ARC_CONNECT_CMD_IDU_READ_DEST			0x77
#define ARC_CONNECT_CMD_IDU_GEN_CIRQ			0x78
#define ARC_CONNECT_CMD_IDU_ACK_CIRQ			0x79
#define ARC_CONNECT_CMD_IDU_CHECK_STATUS		0x7a
#define ARC_CONNECT_CMD_IDU_CHECK_SOURCE		0x7b
#define ARC_CONNECT_CMD_IDU_SET_MASK			0x7c
#define ARC_CONNECT_CMD_IDU_READ_MASK			0x7d
#define ARC_CONNECT_CMD_IDU_CHECK_FIRST			0x7e

/* the start intno of common interrupt managed by IDU */
#define ARC_CONNECT_IDU_IRQ_START			24

#define ARC_CONNECT_INTRPT_TRIGGER_LEVEL	0
#define ARC_CONNECT_INTRPT_TRIGGER_EDGE		1


#define ARC_CONNECT_DISTRI_MODE_ROUND_ROBIN	0
#define ARC_CONNECT_DISTRI_MODE_FIRST_ACK	1
#define ARC_CONNECT_DISTRI_ALL_DEST		2

struct arc_connect_cmd {
	union {
		struct {
#ifdef CONFIG_BIG_ENDIAN
			uint32_t pad:8, param:16, cmd:8;
#else
			uint32_t cmd:8, param:16, pad:8;
#endif
		};
		uint32_t val;
	};
};

struct arc_connect_bcr {
	union {
		struct {
#ifdef CONFIG_BIG_ENDIAN
			uint32_t pad4:6, pw_dom:1, pad3:1,
			idu:1, pad2:1, num_cores:6,
			pad:1,  gfrc:1, dbg:1, pw:1,
			msg:1, sem:1, ipi:1, slv:1,
			ver:8;
#else
			uint32_t ver:8,
			slv:1, ipi:1, sem:1, msg:1,
			pw:1, dbg:1, gfrc:1, pad:1,
			num_cores:6, pad2:1, idu:1,
			pad3:1, pw_dom:1, pad4:6;
#endif
		};
		uint32_t val;
	};
};

struct arc_connect_idu_bcr {
	union {
		struct {
#ifdef CONFIG_BIG_ENDIAN
			uint32_t pad:21, cirqnum:3, ver:8;
#else
			uint32_t ver:8, cirqnum:3, pad:21;
#endif
		};
		uint32_t val;
	};
};

static inline void z_arc_connect_cmd(uint32_t cmd, uint32_t param)
{
	struct arc_connect_cmd regval;

	regval.pad = 0;
	regval.cmd = cmd;
	regval.param = param;

	z_arc_v2_aux_reg_write(_ARC_V2_CONNECT_CMD, regval.val);
}

static inline void z_arc_connect_cmd_data(uint32_t cmd, uint32_t param,
				   uint32_t data)
{
	z_arc_v2_aux_reg_write(_ARC_V2_CONNECT_WDATA, data);
	z_arc_connect_cmd(cmd, param);
}

static inline uint32_t z_arc_connect_cmd_readback(void)
{
	return z_arc_v2_aux_reg_read(_ARC_V2_CONNECT_READBACK);
}


/* inter-core interrupt related functions */
extern void z_arc_connect_ici_generate(uint32_t core_id);
extern void z_arc_connect_ici_ack(uint32_t core_id);
extern uint32_t z_arc_connect_ici_read_status(uint32_t core_id);
extern uint32_t z_arc_connect_ici_check_src(void);
extern void z_arc_connect_ici_clear(void);

/* inter-core debug related functions */
extern void z_arc_connect_debug_reset(uint32_t core_mask);
extern void z_arc_connect_debug_halt(uint32_t core_mask);
extern void z_arc_connect_debug_run(uint32_t core_mask);
extern void z_arc_connect_debug_mask_set(uint32_t core_mask, uint32_t mask);
extern uint32_t z_arc_connect_debug_mask_read(uint32_t core_mask);
extern void z_arc_connect_debug_select_set(uint32_t core_mask);
extern uint32_t z_arc_connect_debug_select_read(void);
extern uint32_t z_arc_connect_debug_en_read(void);
extern uint32_t z_arc_connect_debug_cmd_read(void);
extern uint32_t z_arc_connect_debug_core_read(void);

/* global free-running counter(gfrc) related functions */
extern void z_arc_connect_gfrc_clear(void);
extern uint64_t z_arc_connect_gfrc_read(void);
extern void z_arc_connect_gfrc_enable(void);
extern void z_arc_connect_gfrc_disable(void);
extern void z_arc_connect_gfrc_core_set(uint32_t core_mask);
extern uint32_t z_arc_connect_gfrc_halt_read(void);
extern uint32_t z_arc_connect_gfrc_core_read(void);

/* interrupt distribute unit related functions */
extern void z_arc_connect_idu_enable(void);
extern void z_arc_connect_idu_disable(void);
extern uint32_t z_arc_connect_idu_read_enable(void);
extern void z_arc_connect_idu_set_mode(uint32_t irq_num,
	uint16_t trigger_mode, uint16_t distri_mode);
extern uint32_t z_arc_connect_idu_read_mode(uint32_t irq_num);
extern void z_arc_connect_idu_set_dest(uint32_t irq_num, uint32_t core_mask);
extern uint32_t z_arc_connect_idu_read_dest(uint32_t irq_num);
extern void z_arc_connect_idu_gen_cirq(uint32_t irq_num);
extern void z_arc_connect_idu_ack_cirq(uint32_t irq_num);
extern uint32_t z_arc_connect_idu_check_status(uint32_t irq_num);
extern uint32_t z_arc_connect_idu_check_source(uint32_t irq_num);
extern void z_arc_connect_idu_set_mask(uint32_t irq_num, uint32_t mask);
extern uint32_t z_arc_connect_idu_read_mask(uint32_t irq_num);
extern uint32_t z_arc_connect_idu_check_first(uint32_t irq_num);

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_ARC_V2_ARC_CONNECT_H_ */
