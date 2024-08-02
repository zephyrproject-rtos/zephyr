/*
 * Copyright (c) 2021 Microchip Technology Inc. and its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MEC_ACPI_EC_H
#define _MEC_ACPI_EC_H

#include <stdint.h>
#include <stddef.h>

#define MCHP_ACPI_EC_SPACING		0x0400u
#define MCHP_ACPI_EC_SPACING_PWROF2	10

/* EC_STS and OS_CMD_STS(read) bit definitions */
#define MCHP_ACPI_EC_STS_OBF_POS	0
#define MCHP_ACPI_EC_STS_OBF		BIT(MCHP_ACPI_EC_STS_OBF_POS)
#define MCHP_ACPI_EC_STS_IBF_POS	1
#define MCHP_ACPI_EC_STS_IBF		BIT(MCHP_ACPI_EC_STS_IBF_POS)
#define MCHP_ACPI_EC_STS_UD1A_POS	2
#define MCHP_ACPI_EC_STS_UD1A		BIT(MCHP_ACPI_EC_STS_UD1A_POS)
#define MCHP_ACPI_EC_STS_CMD_POS	3
#define MCHP_ACPI_EC_STS_CMD		BIT(MCHP_ACPI_EC_STS_CMD_POS)
#define MCHP_ACPI_EC_STS_BURST_POS	4
#define MCHP_ACPI_EC_STS_BURST		BIT(MCHP_ACPI_EC_STS_BURST_POS)
#define MCHP_ACPI_EC_STS_SCI_POS	5
#define MCHP_ACPI_EC_STS_SCI		BIT(MCHP_ACPI_EC_STS_SCI_POS)
#define MCHP_ACPI_EC_STS_SMI_POS	6
#define MCHP_ACPI_EC_STS_SMI		BIT(MCHP_ACPI_EC_STS_SMI_POS)
#define MCHP_ACPI_EC_STS_UD0A_POS	7
#define MCHP_ACPI_EC_STS_UD0A		BIT(MCHP_ACPI_EC_STS_UD0A_POS)

/* EC_BYTE_CTRL and OS_BYTE_CTRL */
#define MCHP_ACPI_EC_BYTE_CTRL_4B_POS	0
#define MCHP_ACPI_EC_BYTE_CTRL_4B_EN	BIT(MCHP_ACPI_EC_BYTE_CTRL_4B_POS)

/*  ACPI_PM1 peripheral */

/* ACPI_PM1 RT/EC Status 1 */
#define MCHP_ACPI_PM1_RT_STS1_REG_OFS		0
#define MCHP_ACPI_PM1_EC_STS1_REG_OFS		0x0100u
#define MCHP_ACPI_PM1_STS1_REG_MASK		0x0000u

/* ACPI_PM1 RT/EC Status 2 */
#define MCHP_ACPI_PM1_RT_STS2_REG_OFS		1u
#define MCHP_ACPI_PM1_EC_STS2_REG_OFS		0x0101u
#define MCHP_ACPI_PM1_STS2_REG_MASK		0x008fu
#define MCHP_ACPI_PM1_STS2_PWRBTN		BIT(0)
#define MCHP_ACPI_PM1_STS2_SLPBTN		BIT(1)
#define MCHP_ACPI_PM1_STS2_RTC			BIT(2)
#define MCHP_ACPI_PM1_STS2_PWRBTNOR		BIT(3)
#define MCHP_ACPI_PM1_STS2_WAK			BIT(7)

/* ACPI_PM1 RT/EC Enable 1 */
#define MCHP_ACPI_PM1_RT_EN1_REG_OFS		0x0002u
#define MCHP_ACPI_PM1_EC_EN1_REG_OFS		0x0102u
#define MCHP_ACPI_PM1_EN1_REG_MASK		0u

/* ACPI_PM1 RT/EC Enable 2 */
#define MCHP_ACPI_PM1_RT_EN2_REG_OFS		3u
#define MCHP_ACPI_PM1_EC_EN2_REG_OFS		0x0103u
#define MCHP_ACPI_PM1_EN2_REG_MASK		0x0007u
#define MCHP_ACPI_PM1_EN2_PWRBTN		BIT(0)
#define MCHP_ACPI_PM1_EN2_SLPBTN		BIT(1)
#define MCHP_ACPI_PM1_EN2_RTC			BIT(2)

/* ACPI_PM1 RT/EC Control 1 */
#define MCHP_ACPI_PM1_RT_CTRL1_REG_OFS		4u
#define MCHP_ACPI_PM1_EC_CTRL1_REG_OFS		0x0104u
#define MCHP_ACPI_PM1_CTRL1_REG_MASK		0u

/* ACPI_PM1 RT/EC Control 2 */
#define MCHP_ACPI_PM1_RT_CTRL2_REG_OFS		5ul
#define MCHP_ACPI_PM1_EC_CTRL2_REG_OFS		0x0105u
#define MCHP_ACPI_PM1_CTRL2_REG_MASK		0x003eu
#define MCHP_ACPI_PM1_CTRL2_PWRBTNOR_EN		BIT(1)
#define MCHP_ACPI_PM1_CTRL2_SLP_TYPE_POS	2
#define MCHP_ACPI_PM1_CTRL2_SLP_TYPE_MASK	SHLU32(3, 2)
#define MCHP_ACPI_PM1_CTRL2_SLP_EN		BIT(5)

/* ACPI_PM1 RT/EC Control 21 */
#define MCHP_ACPI_PM1_RT_CTRL21_REG_OFS		0x0006u
#define MCHP_ACPI_PM1_EC_CTRL21_REG_OFS		0x0106u
#define MCHP_ACPI_PM1_CTRL21_REG_MASK		0u

/* ACPI_PM1 RT/EC Control 22 */
#define MCHP_ACPI_PM1_RT_CTRL22_REG_OFS		7u
#define MCHP_ACPI_PM1_EC_CTRL22_REG_OFS		0x0107u
#define MCHP_ACPI_PM1_CTRL22_REG_MASK		0u

/* ACPI_PM1 EC PM Status register */
#define MCHP_ACPI_PM1_EC_PM_STS_REG_OFS		0x0110u
#define MCHP_ACPI_PM1_EC_PM_STS_REG_MASK	0x01u
#define MCHP_ACPI_PM1_EC_PM_STS_SCI		0x01u

/** @brief ACPI EC Registers (ACPI_EC) */
struct acpi_ec_regs {
	volatile uint32_t OS_DATA;
	volatile uint8_t OS_CMD_STS;
	volatile uint8_t OS_BYTE_CTRL;
	uint8_t RSVD1[0x100u - 0x06u];
	volatile uint32_t EC2OS_DATA;
	volatile uint8_t EC_STS;
	volatile uint8_t EC_BYTE_CTRL;
	uint8_t RSVD2[2];
	volatile uint32_t OS2EC_DATA;
};

/** @brief ACPI PM1 Registers (ACPI_PM1) */
struct acpi_pm1_regs {
	volatile uint8_t RT_STS1;
	volatile uint8_t RT_STS2;
	volatile uint8_t RT_EN1;
	volatile uint8_t RT_EN2;
	volatile uint8_t RT_CTRL1;
	volatile uint8_t RT_CTRL2;
	volatile uint8_t RT_CTRL21;
	volatile uint8_t RT_CTRL22;
	uint8_t RSVD1[(0x100u - 0x008u)];
	volatile uint8_t EC_STS1;
	volatile uint8_t EC_STS2;
	volatile uint8_t EC_EN1;
	volatile uint8_t EC_EN2;
	volatile uint8_t EC_CTRL1;
	volatile uint8_t EC_CTRL2;
	volatile uint8_t EC_CTRL21;
	volatile uint8_t EC_CTRL22;
	uint8_t RSVD2[(0x0110u - 0x0108u)];
	volatile uint8_t EC_PM_STS;
	uint8_t RSVD3[3];
};

#endif	/* #ifndef _MEC_ACPI_EC_H */
