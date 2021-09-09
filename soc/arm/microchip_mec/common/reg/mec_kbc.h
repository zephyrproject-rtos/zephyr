/*
 * Copyright (c) 2021 Microchip Technology Inc. and its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MEC_KBC_H
#define _MEC_KBC_H

#include <stdint.h>
#include <stddef.h>

/* ---- EM8042 Keyboard Controller (KBC) ---- */

/* EC_KBC_STS and KBC_STS_RD bit definitions */
#define MCHP_KBC_STS_OBF_POS	0u
#define MCHP_KBC_STS_OBF	BIT(MCHP_KBC_STS_OBF_POS)
#define MCHP_KBC_STS_IBF_POS	1u
#define MCHP_KBC_STS_IBF	BIT(MCHP_KBC_STS_IBF_POS)
#define MCHP_KBC_STS_UD0_POS	2u
#define MCHP_KBC_STS_UD0	BIT(MCHP_KBC_STS_UD0_POS)
#define MCHP_KBC_STS_CD_POS	3u
#define MCHP_KBC_STS_CD		BIT(MCHP_KBC_STS_CD_POS)
#define MCHP_KBC_STS_UD1_POS	4u
#define MCHP_KBC_STS_UD1	BIT(MCHP_KBC_STS_UD1_POS)
#define MCHP_KBC_STS_AUXOBF_POS 5u
#define MCHP_KBC_STS_AUXOBF	BIT(MCHP_KBC_STS_AUXOBF_POS)
#define MCHP_KBC_STS_UD2_POS	6u
#define MCHP_KBC_STS_UD2_MASK0	0x03u
#define MCHP_KBC_STS_UD2_MASK	0xc0u
#define MCHP_KBC_STS_UD2_0_POS	6u
#define MCHP_KBC_STS_UD2_0	BIT(6)
#define MCHP_KBC_STS_UD2_1	BIT(7)

/* KBC_CTRL bit definitions */
#define MCHP_KBC_CTRL_UD3_POS		0u
#define MCHP_KBC_CTRL_UD3		BIT(MCHP_KBC_CTRL_UD3_POS)
#define MCHP_KBC_CTRL_SAEN_POS		1u
#define MCHP_KBC_CTRL_SAEN		BIT(MCHP_KBC_CTRL_SAEN_POS)
#define MCHP_KBC_CTRL_PCOBFEN_POS	2u
#define MCHP_KBC_CTRL_PCOBFEN		BIT(MCHP_KBC_CTRL_PCOBFEN_POS)
#define MCHP_KBC_CTRL_UD4_POS		3u
#define MCHP_KBC_CTRL_UD4_MASK0		0x03u
#define MCHP_KBC_CTRL_UD4_MASK		0x18u
#define MCHP_KBC_CTRL_OBFEN_POS		5u
#define MCHP_KBC_CTRL_OBFEN		BIT(MCHP_KBC_CTRL_OBFEN_POS)
#define MCHP_KBC_CTRL_UD5_POS		6u
#define MCHP_KBC_CTRL_UD5		BIT(MCHP_KBC_CTRL_UD5_POS)
#define MCHP_KBC_CTRL_AUXH_POS		7u
#define MCHP_KBC_CTRL_AUXH		BIT(MCHP_KBC_CTRL_AUXH_POS)

/* PCOBF register bit definitions */
#define MCHP_KBC_PCOBF_EN_POS	0u
#define MCHP_KBC_PCOBF_EN	BIT(MCHP_KBC_PCOBF_EN_POS)

/* KBC_PORT92_EN register bit definitions */
#define MCHP_KBC_PORT92_EN_POS	0u
#define MCHP_KBC_PORT92_EN	BIT(MCHP_KBC_PORT92_EN_POS)

/* HOST Port 92h emulation registers */
#define MCHP_PORT92_HOST_MASK			GENMASK(1, 0)
#define MCHP_PORT92_HOST_ALT_CPU_RST_POS	0
#define MCHP_PORT92_HOST_ALT_CPU_RST		BIT(0)
#define MCHP_PORT92_HOST_ALT_GA20_POS		1
#define MCHP_PORT92_HOST_ALT_GA20		BIT(1)

/* GATEA20_CTRL */
#define MCHP_PORT92_GA20_CTRL_MASK	BIT(0)
#define MCHP_PORT92_GA20_CTRL_VAL_POS	0
#define MCHP_PORT92_GA20_CTRL_VAL_HI	BIT(0)

/*
 * SETGA20L - writes of any data to this register causes
 * GATEA20 latch to be set.
 */
#define MCHP_PORT92_SETGA20L_MASK	BIT(0)
#define MCHP_PORT92_SETGA20L_SET_POS	0
#define MCHP_PORT92_SETGA20L_SET	BIT(0)

/*
 * RSTGA20L - writes of any data to this register causes
 * the GATEA20 latch to be reset
 */
#define MCHP_PORT92_RSTGA20L_MASK	BIT(0)
#define MCHP_PORT92_RSTGA20L_SET_POS	0
#define MCHP_PORT92_RSTGA20L_RST	BIT(0)

/* ACTV */
#define MCHP_PORT92_ACTV_MASK		BIT(0)
#define MCHP_PORT92_ACTV_ENABLE		BIT(0)

/** @brief 8042 Emulated Keyboard controller. Size = 820(0x334) */
struct kbc_regs {
	volatile uint32_t HOST_AUX_DATA;
	volatile uint32_t KBC_STS_RD;
	uint8_t RSVD1[0x100 - 0x08];
	volatile uint32_t EC_DATA;
	volatile uint32_t EC_KBC_STS;
	volatile uint32_t KBC_CTRL;
	volatile uint32_t EC_AUX_DATA;
	uint32_t RSVD2[1];
	volatile uint32_t PCOBF;
	uint8_t RSVD3[0x0330 - 0x0118];
	volatile uint32_t KBC_PORT92_EN;
};

/** @brief Fast Port92h Registers (PORT92) */
struct port92_regs {
	volatile uint32_t HOST_P92;
	uint8_t RSVD1[0x100u - 0x04u];
	volatile uint32_t GATEA20_CTRL;
	uint32_t RSVD2[1];
	volatile uint32_t SETGA20L;
	volatile uint32_t RSTGA20L;
	uint8_t RSVD3[0x0330ul - 0x0110ul];
	volatile uint32_t ACTV;
};

#endif /* #ifndef _MEC_KBC_H */
