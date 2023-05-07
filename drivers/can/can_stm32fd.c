/*
 * Copyright (c) 2022 Vestas Wind Systems A/S
 * Copyright (c) 2020 Alexander Wachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/can.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/__assert.h>
#include <soc.h>
#include <stm32_ll_rcc.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>

#include "can_mcan.h"

LOG_MODULE_REGISTER(can_stm32fd, CONFIG_CAN_LOG_LEVEL);

#define DT_DRV_COMPAT st_stm32_fdcan

/*
 * The STMicroelectronics STM32 FDCAN definitions correspond to those found in the
 * STMicroelectronics STM32G4 Series Reference manual (RM0440), Rev 7.
 *
 * This controller uses a Bosch M_CAN like register layout, but some registers are unimplemented,
 * some registers are mapped to other register offsets, and some registers have had their bit fields
 * remapped.
 *
 * Apart from the definitions below please note the following limitations:
 * - TEST register SVAL, TXBNS, PVAL, and TXBNP bits are not available.
 * - CCCR register VMM and UTSU bits are not available.
 * - TXBC register TFQS, NDTB, and TBSA fields are not available.
 */

/* Interrupt register */
#define CAN_STM32FD_IR_ARA  BIT(23)
#define CAN_STM32FD_IR_PED  BIT(22)
#define CAN_STM32FD_IR_PEA  BIT(21)
#define CAN_STM32FD_IR_WDI  BIT(20)
#define CAN_STM32FD_IR_BO   BIT(19)
#define CAN_STM32FD_IR_EW   BIT(18)
#define CAN_STM32FD_IR_EP   BIT(17)
#define CAN_STM32FD_IR_ELO  BIT(16)
#define CAN_STM32FD_IR_TOO  BIT(15)
#define CAN_STM32FD_IR_MRAF BIT(14)
#define CAN_STM32FD_IR_TSW  BIT(13)
#define CAN_STM32FD_IR_TEFL BIT(12)
#define CAN_STM32FD_IR_TEFF BIT(11)
#define CAN_STM32FD_IR_TEFN BIT(10)
#define CAN_STM32FD_IR_TFE  BIT(9)
#define CAN_STM32FD_IR_TCF  BIT(8)
#define CAN_STM32FD_IR_TC   BIT(7)
#define CAN_STM32FD_IR_HPM  BIT(6)
#define CAN_STM32FD_IR_RF1L BIT(5)
#define CAN_STM32FD_IR_RF1F BIT(4)
#define CAN_STM32FD_IR_RF1N BIT(3)
#define CAN_STM32FD_IR_RF0L BIT(2)
#define CAN_STM32FD_IR_RF0F BIT(1)
#define CAN_STM32FD_IR_RF0N BIT(0)

/* Interrupt Enable register */
#define CAN_STM32FD_IE_ARAE  BIT(23)
#define CAN_STM32FD_IE_PEDE  BIT(22)
#define CAN_STM32FD_IE_PEAE  BIT(21)
#define CAN_STM32FD_IE_WDIE  BIT(20)
#define CAN_STM32FD_IE_BOE   BIT(19)
#define CAN_STM32FD_IE_EWE   BIT(18)
#define CAN_STM32FD_IE_EPE   BIT(17)
#define CAN_STM32FD_IE_ELOE  BIT(16)
#define CAN_STM32FD_IE_TOOE  BIT(15)
#define CAN_STM32FD_IE_MRAFE BIT(14)
#define CAN_STM32FD_IE_TSWE  BIT(13)
#define CAN_STM32FD_IE_TEFLE BIT(12)
#define CAN_STM32FD_IE_TEFFE BIT(11)
#define CAN_STM32FD_IE_TEFNE BIT(10)
#define CAN_STM32FD_IE_TFEE  BIT(9)
#define CAN_STM32FD_IE_TCFE  BIT(8)
#define CAN_STM32FD_IE_TCE   BIT(7)
#define CAN_STM32FD_IE_HPME  BIT(6)
#define CAN_STM32FD_IE_RF1LE BIT(5)
#define CAN_STM32FD_IE_RF1FE BIT(4)
#define CAN_STM32FD_IE_RF1NE BIT(3)
#define CAN_STM32FD_IE_RF0LE BIT(2)
#define CAN_STM32FD_IE_RF0FE BIT(1)
#define CAN_STM32FD_IE_RF0NE BIT(0)

/* Interrupt Line Select register */
#define CAN_STM32FD_ILS_PERR    BIT(6)
#define CAN_STM32FD_ILS_BERR    BIT(5)
#define CAN_STM32FD_ILS_MISC    BIT(4)
#define CAN_STM32FD_ILS_TFERR   BIT(3)
#define CAN_STM32FD_ILS_SMSG    BIT(2)
#define CAN_STM32FD_ILS_RXFIFO1 BIT(1)
#define CAN_STM32FD_ILS_RXFIFO0 BIT(0)

/* Global filter configuration register */
#define CAN_STM32FD_RXGFC      0x080
#define CAN_STM32FD_RXGFC_LSE  GENMASK(27, 24)
#define CAN_STM32FD_RXGFC_LSS  GENMASK(20, 16)
#define CAN_STM32FD_RXGFC_F0OM BIT(9)
#define CAN_STM32FD_RXGFC_F1OM BIT(8)
#define CAN_STM32FD_RXGFC_ANFS GENMASK(5, 4)
#define CAN_STM32FD_RXGFC_ANFE GENMASK(3, 2)
#define CAN_STM32FD_RXGFC_RRFS BIT(1)
#define CAN_STM32FD_RXGFC_RRFE BIT(0)

/* Extended ID AND Mask register */
#define CAN_STM32FD_XIDAM 0x084

/* High Priority Message Status register */
#define CAN_STM32FD_HPMS 0x088

/* Rx FIFO 0 Status register */
#define CAN_STM32FD_RXF0S 0x090

/* Rx FIFO 0 Acknowledge register */
#define CAN_STM32FD_RXF0A 0x094

/* Rx FIFO 1 Status register */
#define CAN_STM32FD_RXF1S 0x098

/* Rx FIFO 1 Acknowledge register */
#define CAN_STM32FD_RXF1A 0x09C

/* Tx Buffer Configuration register */
#define CAN_STM32FD_TXBC_TFQM BIT(24)

/* Tx Buffer Request Pending register */
#define CAN_STM32FD_TXBRP 0x0C8

/* Tx Buffer Add Request register */
#define CAN_STM32FD_TXBAR 0x0CC

/* Tx Buffer Cancellation Request register */
#define CAN_STM32FD_TXBCR 0x0D0

/* Tx Buffer Transmission Occurred register */
#define CAN_STM32FD_TXBTO 0x0D4

/* Tx Buffer Cancellation Finished register */
#define CAN_STM32FD_TXBCF 0x0D8

/* Tx Buffer Transmission Interrupt Enable register */
#define CAN_STM32FD_TXBTIE 0x0DC

/* Tx Buffer Cancellation Finished Interrupt Enable register */
#define CAN_STM32FD_TXBCIE 0x0E0

/* Tx Event FIFO Status register */
#define CAN_STM32FD_TXEFS 0x0E4

/* Tx Event FIFO Acknowledge register */
#define CAN_STM32FD_TXEFA 0x0E8

/* Register address indicating unsupported register */
#define CAN_STM32FD_REGISTER_UNSUPPORTED UINT16_MAX

/* This symbol takes the value 1 if one of the device instances */
/* is configured in dts with a domain clock */
#if STM32_DT_INST_DEV_DOMAIN_CLOCK_SUPPORT
#define STM32_CANFD_DOMAIN_CLOCK_SUPPORT 1
#else
#define STM32_CANFD_DOMAIN_CLOCK_SUPPORT 0
#endif

struct can_stm32fd_config {
	mm_reg_t base;
	size_t pclk_len;
	const struct stm32_pclken *pclken;
	void (*config_irq)(void);
	const struct pinctrl_dev_config *pcfg;
	uint8_t clock_divider;
};

static inline uint16_t can_stm32fd_remap_reg(uint16_t reg)
{
	uint16_t remap;

	switch (reg) {
	case CAN_MCAN_SIDFC:
		__fallthrough;
	case CAN_MCAN_XIDFC:
		__fallthrough;
	case CAN_MCAN_NDAT1:
		__fallthrough;
	case CAN_MCAN_NDAT2:
		__fallthrough;
	case CAN_MCAN_RXF0C:
		__fallthrough;
	case CAN_MCAN_RXBC:
		__fallthrough;
	case CAN_MCAN_RXF1C:
		__fallthrough;
	case CAN_MCAN_RXESC:
		__fallthrough;
	case CAN_MCAN_TXESC:
		__fallthrough;
	case CAN_MCAN_TXEFC:
		__ASSERT_NO_MSG(false);
		remap = CAN_STM32FD_REGISTER_UNSUPPORTED;
	case CAN_MCAN_XIDAM:
		remap = CAN_STM32FD_XIDAM;
		break;
	case CAN_MCAN_RXF0S:
		remap = CAN_STM32FD_RXF0S;
		break;
	case CAN_MCAN_RXF0A:
		remap = CAN_STM32FD_RXF0A;
		break;
	case CAN_MCAN_RXF1S:
		remap = CAN_STM32FD_RXF1S;
		break;
	case CAN_MCAN_RXF1A:
		remap = CAN_STM32FD_RXF1A;
		break;
	case CAN_MCAN_TXBRP:
		remap = CAN_STM32FD_TXBRP;
		break;
	case CAN_MCAN_TXBAR:
		remap = CAN_STM32FD_TXBAR;
		break;
	case CAN_MCAN_TXBCR:
		remap = CAN_STM32FD_TXBCR;
		break;
	case CAN_MCAN_TXBTO:
		remap = CAN_STM32FD_TXBTO;
		break;
	case CAN_MCAN_TXBCF:
		remap = CAN_STM32FD_TXBCF;
		break;
	case CAN_MCAN_TXBTIE:
		remap = CAN_STM32FD_TXBTIE;
		break;
	case CAN_MCAN_TXBCIE:
		remap = CAN_STM32FD_TXBCIE;
		break;
	case CAN_MCAN_TXEFS:
		remap = CAN_STM32FD_TXEFS;
		break;
	case CAN_MCAN_TXEFA:
		remap = CAN_STM32FD_TXEFA;
		break;
	default:
		/* No register address remap needed */
		remap = reg;
		break;
	};

	return remap;
}

static int can_stm32fd_read_reg(const struct device *dev, uint16_t reg, uint32_t *val)
{
	const struct can_mcan_config *mcan_config = dev->config;
	const struct can_stm32fd_config *stm32fd_config = mcan_config->custom;
	uint16_t remap;
	uint32_t bits;
	int err;

	remap = can_stm32fd_remap_reg(reg);
	if (remap == CAN_STM32FD_REGISTER_UNSUPPORTED) {
		return -ENOTSUP;
	}

	err = can_mcan_sys_read_reg(stm32fd_config->base, remap, &bits);
	if (err != 0) {
		return err;
	}

	*val = 0U;

	switch (reg) {
	case CAN_MCAN_IR:
		/* Remap IR bits */
		*val |= FIELD_PREP(CAN_MCAN_IR_ARA,  FIELD_GET(CAN_STM32FD_IR_ARA, bits));
		*val |= FIELD_PREP(CAN_MCAN_IR_PED,  FIELD_GET(CAN_STM32FD_IR_PED, bits));
		*val |= FIELD_PREP(CAN_MCAN_IR_PEA,  FIELD_GET(CAN_STM32FD_IR_PEA, bits));
		*val |= FIELD_PREP(CAN_MCAN_IR_WDI,  FIELD_GET(CAN_STM32FD_IR_WDI, bits));
		*val |= FIELD_PREP(CAN_MCAN_IR_BO,   FIELD_GET(CAN_STM32FD_IR_BO, bits));
		*val |= FIELD_PREP(CAN_MCAN_IR_EW,   FIELD_GET(CAN_STM32FD_IR_EW, bits));
		*val |= FIELD_PREP(CAN_MCAN_IR_EP,   FIELD_GET(CAN_STM32FD_IR_EP, bits));
		*val |= FIELD_PREP(CAN_MCAN_IR_ELO,  FIELD_GET(CAN_STM32FD_IR_ELO, bits));
		*val |= FIELD_PREP(CAN_MCAN_IR_TOO,  FIELD_GET(CAN_STM32FD_IR_TOO, bits));
		*val |= FIELD_PREP(CAN_MCAN_IR_MRAF, FIELD_GET(CAN_STM32FD_IR_MRAF, bits));
		*val |= FIELD_PREP(CAN_MCAN_IR_TSW,  FIELD_GET(CAN_STM32FD_IR_TSW, bits));
		*val |= FIELD_PREP(CAN_MCAN_IR_TEFL, FIELD_GET(CAN_STM32FD_IR_TEFL, bits));
		*val |= FIELD_PREP(CAN_MCAN_IR_TEFF, FIELD_GET(CAN_STM32FD_IR_TEFF, bits));
		*val |= FIELD_PREP(CAN_MCAN_IR_TEFN, FIELD_GET(CAN_STM32FD_IR_TEFN, bits));
		*val |= FIELD_PREP(CAN_MCAN_IR_TFE,  FIELD_GET(CAN_STM32FD_IR_TFE, bits));
		*val |= FIELD_PREP(CAN_MCAN_IR_TCF,  FIELD_GET(CAN_STM32FD_IR_TCF, bits));
		*val |= FIELD_PREP(CAN_MCAN_IR_TC,   FIELD_GET(CAN_STM32FD_IR_TC, bits));
		*val |= FIELD_PREP(CAN_MCAN_IR_HPM,  FIELD_GET(CAN_STM32FD_IR_HPM, bits));
		*val |= FIELD_PREP(CAN_MCAN_IR_RF1L, FIELD_GET(CAN_STM32FD_IR_RF1L, bits));
		*val |= FIELD_PREP(CAN_MCAN_IR_RF1F, FIELD_GET(CAN_STM32FD_IR_RF1F, bits));
		*val |= FIELD_PREP(CAN_MCAN_IR_RF1N, FIELD_GET(CAN_STM32FD_IR_RF1N, bits));
		*val |= FIELD_PREP(CAN_MCAN_IR_RF0L, FIELD_GET(CAN_STM32FD_IR_RF0L, bits));
		*val |= FIELD_PREP(CAN_MCAN_IR_RF0F, FIELD_GET(CAN_STM32FD_IR_RF0F, bits));
		*val |= FIELD_PREP(CAN_MCAN_IR_RF0N, FIELD_GET(CAN_STM32FD_IR_RF0N, bits));
		break;
	case CAN_MCAN_IE:
		/* Remap IE bits */
		*val |= FIELD_PREP(CAN_MCAN_IE_ARAE,  FIELD_GET(CAN_STM32FD_IE_ARAE, bits));
		*val |= FIELD_PREP(CAN_MCAN_IE_PEDE,  FIELD_GET(CAN_STM32FD_IE_PEDE, bits));
		*val |= FIELD_PREP(CAN_MCAN_IE_PEAE,  FIELD_GET(CAN_STM32FD_IE_PEAE, bits));
		*val |= FIELD_PREP(CAN_MCAN_IE_WDIE,  FIELD_GET(CAN_STM32FD_IE_WDIE, bits));
		*val |= FIELD_PREP(CAN_MCAN_IE_BOE,   FIELD_GET(CAN_STM32FD_IE_BOE, bits));
		*val |= FIELD_PREP(CAN_MCAN_IE_EWE,   FIELD_GET(CAN_STM32FD_IE_EWE, bits));
		*val |= FIELD_PREP(CAN_MCAN_IE_EPE,   FIELD_GET(CAN_STM32FD_IE_EPE, bits));
		*val |= FIELD_PREP(CAN_MCAN_IE_ELOE,  FIELD_GET(CAN_STM32FD_IE_ELOE, bits));
		*val |= FIELD_PREP(CAN_MCAN_IE_TOOE,  FIELD_GET(CAN_STM32FD_IE_TOOE, bits));
		*val |= FIELD_PREP(CAN_MCAN_IE_MRAFE, FIELD_GET(CAN_STM32FD_IE_MRAFE, bits));
		*val |= FIELD_PREP(CAN_MCAN_IE_TSWE,  FIELD_GET(CAN_STM32FD_IE_TSWE, bits));
		*val |= FIELD_PREP(CAN_MCAN_IE_TEFLE, FIELD_GET(CAN_STM32FD_IE_TEFLE, bits));
		*val |= FIELD_PREP(CAN_MCAN_IE_TEFFE, FIELD_GET(CAN_STM32FD_IE_TEFFE, bits));
		*val |= FIELD_PREP(CAN_MCAN_IE_TEFNE, FIELD_GET(CAN_STM32FD_IE_TEFNE, bits));
		*val |= FIELD_PREP(CAN_MCAN_IE_TFEE,  FIELD_GET(CAN_STM32FD_IE_TFEE, bits));
		*val |= FIELD_PREP(CAN_MCAN_IE_TCFE,  FIELD_GET(CAN_STM32FD_IE_TCFE, bits));
		*val |= FIELD_PREP(CAN_MCAN_IE_TCE,   FIELD_GET(CAN_STM32FD_IE_TCE, bits));
		*val |= FIELD_PREP(CAN_MCAN_IE_HPME,  FIELD_GET(CAN_STM32FD_IE_HPME, bits));
		*val |= FIELD_PREP(CAN_MCAN_IE_RF1LE, FIELD_GET(CAN_STM32FD_IE_RF1LE, bits));
		*val |= FIELD_PREP(CAN_MCAN_IE_RF1FE, FIELD_GET(CAN_STM32FD_IE_RF1FE, bits));
		*val |= FIELD_PREP(CAN_MCAN_IE_RF1NE, FIELD_GET(CAN_STM32FD_IE_RF1NE, bits));
		*val |= FIELD_PREP(CAN_MCAN_IE_RF0LE, FIELD_GET(CAN_STM32FD_IE_RF0LE, bits));
		*val |= FIELD_PREP(CAN_MCAN_IE_RF0FE, FIELD_GET(CAN_STM32FD_IE_RF0FE, bits));
		*val |= FIELD_PREP(CAN_MCAN_IE_RF0NE, FIELD_GET(CAN_STM32FD_IE_RF0NE, bits));
		break;
	case CAN_MCAN_ILS:
		/* Only remap ILS groups used in can_mcan.c */
		if ((bits & CAN_STM32FD_ILS_RXFIFO1) != 0U) {
			*val |= CAN_MCAN_ILS_RF1LL | CAN_MCAN_ILS_RF1FL | CAN_MCAN_ILS_RF1NL;
		}

		if ((bits & CAN_STM32FD_ILS_RXFIFO0) != 0U) {
			*val |= CAN_MCAN_ILS_RF0LL | CAN_MCAN_ILS_RF0FL | CAN_MCAN_ILS_RF0NL;
		}
		break;
	case CAN_MCAN_GFC:
		/* Map fields from RXGFC excluding STM32 FDCAN LSS and LSE fields */
		*val = bits & (CAN_MCAN_GFC_ANFS | CAN_MCAN_GFC_ANFE |
		       CAN_MCAN_GFC_RRFS | CAN_MCAN_GFC_RRFE);
		break;
	default:
		/* No field remap needed */
		*val = bits;
		break;
	};

	return 0;
}

static int can_stm32fd_write_reg(const struct device *dev, uint16_t reg, uint32_t val)
{
	const struct can_mcan_config *mcan_config = dev->config;
	const struct can_stm32fd_config *stm32fd_config = mcan_config->custom;
	uint32_t bits = 0U;
	uint16_t remap;

	remap = can_stm32fd_remap_reg(reg);
	if (remap == CAN_STM32FD_REGISTER_UNSUPPORTED) {
		return -ENOTSUP;
	}

	switch (reg) {
	case CAN_MCAN_IR:
		/* Remap IR bits, ignoring unsupported bits */
		bits |= FIELD_PREP(CAN_STM32FD_IR_ARA,  FIELD_GET(CAN_MCAN_IR_ARA, val));
		bits |= FIELD_PREP(CAN_STM32FD_IR_PED,  FIELD_GET(CAN_MCAN_IR_PED, val));
		bits |= FIELD_PREP(CAN_STM32FD_IR_PEA,  FIELD_GET(CAN_MCAN_IR_PEA, val));
		bits |= FIELD_PREP(CAN_STM32FD_IR_WDI,  FIELD_GET(CAN_MCAN_IR_WDI, val));
		bits |= FIELD_PREP(CAN_STM32FD_IR_BO,   FIELD_GET(CAN_MCAN_IR_BO, val));
		bits |= FIELD_PREP(CAN_STM32FD_IR_EW,   FIELD_GET(CAN_MCAN_IR_EW, val));
		bits |= FIELD_PREP(CAN_STM32FD_IR_EP,   FIELD_GET(CAN_MCAN_IR_EP, val));
		bits |= FIELD_PREP(CAN_STM32FD_IR_ELO,  FIELD_GET(CAN_MCAN_IR_ELO, val));
		bits |= FIELD_PREP(CAN_STM32FD_IR_TOO,  FIELD_GET(CAN_MCAN_IR_TOO, val));
		bits |= FIELD_PREP(CAN_STM32FD_IR_MRAF, FIELD_GET(CAN_MCAN_IR_MRAF, val));
		bits |= FIELD_PREP(CAN_STM32FD_IR_TSW,  FIELD_GET(CAN_MCAN_IR_TSW, val));
		bits |= FIELD_PREP(CAN_STM32FD_IR_TEFL, FIELD_GET(CAN_MCAN_IR_TEFL, val));
		bits |= FIELD_PREP(CAN_STM32FD_IR_TEFF, FIELD_GET(CAN_MCAN_IR_TEFF, val));
		bits |= FIELD_PREP(CAN_STM32FD_IR_TEFN, FIELD_GET(CAN_MCAN_IR_TEFN, val));
		bits |= FIELD_PREP(CAN_STM32FD_IR_TFE,  FIELD_GET(CAN_MCAN_IR_TFE, val));
		bits |= FIELD_PREP(CAN_STM32FD_IR_TCF,  FIELD_GET(CAN_MCAN_IR_TCF, val));
		bits |= FIELD_PREP(CAN_STM32FD_IR_TC,   FIELD_GET(CAN_MCAN_IR_TC, val));
		bits |= FIELD_PREP(CAN_STM32FD_IR_HPM,  FIELD_GET(CAN_MCAN_IR_HPM, val));
		bits |= FIELD_PREP(CAN_STM32FD_IR_RF1L, FIELD_GET(CAN_MCAN_IR_RF1L, val));
		bits |= FIELD_PREP(CAN_STM32FD_IR_RF1F, FIELD_GET(CAN_MCAN_IR_RF1F, val));
		bits |= FIELD_PREP(CAN_STM32FD_IR_RF1N, FIELD_GET(CAN_MCAN_IR_RF1N, val));
		bits |= FIELD_PREP(CAN_STM32FD_IR_RF0L, FIELD_GET(CAN_MCAN_IR_RF0L, val));
		bits |= FIELD_PREP(CAN_STM32FD_IR_RF0F, FIELD_GET(CAN_MCAN_IR_RF0F, val));
		bits |= FIELD_PREP(CAN_STM32FD_IR_RF0N, FIELD_GET(CAN_MCAN_IR_RF0N, val));
		break;
	case CAN_MCAN_IE:
		/* Remap IE bits, ignoring unsupported bits */
		bits |= FIELD_PREP(CAN_STM32FD_IE_ARAE,  FIELD_GET(CAN_MCAN_IE_ARAE, val));
		bits |= FIELD_PREP(CAN_STM32FD_IE_PEDE,  FIELD_GET(CAN_MCAN_IE_PEDE, val));
		bits |= FIELD_PREP(CAN_STM32FD_IE_PEAE,  FIELD_GET(CAN_MCAN_IE_PEAE, val));
		bits |= FIELD_PREP(CAN_STM32FD_IE_WDIE,  FIELD_GET(CAN_MCAN_IE_WDIE, val));
		bits |= FIELD_PREP(CAN_STM32FD_IE_BOE,   FIELD_GET(CAN_MCAN_IE_BOE, val));
		bits |= FIELD_PREP(CAN_STM32FD_IE_EWE,   FIELD_GET(CAN_MCAN_IE_EWE, val));
		bits |= FIELD_PREP(CAN_STM32FD_IE_EPE,   FIELD_GET(CAN_MCAN_IE_EPE, val));
		bits |= FIELD_PREP(CAN_STM32FD_IE_ELOE,  FIELD_GET(CAN_MCAN_IE_ELOE, val));
		bits |= FIELD_PREP(CAN_STM32FD_IE_TOOE,  FIELD_GET(CAN_MCAN_IE_TOOE, val));
		bits |= FIELD_PREP(CAN_STM32FD_IE_MRAFE, FIELD_GET(CAN_MCAN_IE_MRAFE, val));
		bits |= FIELD_PREP(CAN_STM32FD_IE_TSWE,  FIELD_GET(CAN_MCAN_IE_TSWE, val));
		bits |= FIELD_PREP(CAN_STM32FD_IE_TEFLE, FIELD_GET(CAN_MCAN_IE_TEFLE, val));
		bits |= FIELD_PREP(CAN_STM32FD_IE_TEFFE, FIELD_GET(CAN_MCAN_IE_TEFFE, val));
		bits |= FIELD_PREP(CAN_STM32FD_IE_TEFNE, FIELD_GET(CAN_MCAN_IE_TEFNE, val));
		bits |= FIELD_PREP(CAN_STM32FD_IE_TFEE,  FIELD_GET(CAN_MCAN_IE_TFEE, val));
		bits |= FIELD_PREP(CAN_STM32FD_IE_TCFE,  FIELD_GET(CAN_MCAN_IE_TCFE, val));
		bits |= FIELD_PREP(CAN_STM32FD_IE_TCE,   FIELD_GET(CAN_MCAN_IE_TCE, val));
		bits |= FIELD_PREP(CAN_STM32FD_IE_HPME,  FIELD_GET(CAN_MCAN_IE_HPME, val));
		bits |= FIELD_PREP(CAN_STM32FD_IE_RF1LE, FIELD_GET(CAN_MCAN_IE_RF1LE, val));
		bits |= FIELD_PREP(CAN_STM32FD_IE_RF1FE, FIELD_GET(CAN_MCAN_IE_RF1FE, val));
		bits |= FIELD_PREP(CAN_STM32FD_IE_RF1NE, FIELD_GET(CAN_MCAN_IE_RF1NE, val));
		bits |= FIELD_PREP(CAN_STM32FD_IE_RF0LE, FIELD_GET(CAN_MCAN_IE_RF0LE, val));
		bits |= FIELD_PREP(CAN_STM32FD_IE_RF0FE, FIELD_GET(CAN_MCAN_IE_RF0FE, val));
		bits |= FIELD_PREP(CAN_STM32FD_IE_RF0NE, FIELD_GET(CAN_MCAN_IE_RF0NE, val));
		break;
	case CAN_MCAN_ILS:
		/* Only remap ILS groups used in can_mcan.c */
		if ((val & (CAN_MCAN_ILS_RF1LL | CAN_MCAN_ILS_RF1FL | CAN_MCAN_ILS_RF1NL)) != 0U) {
			bits |= CAN_STM32FD_ILS_RXFIFO1;
		}

		if ((val & (CAN_MCAN_ILS_RF0LL | CAN_MCAN_ILS_RF0FL | CAN_MCAN_ILS_RF0NL)) != 0U) {
			bits |= CAN_STM32FD_ILS_RXFIFO0;
		}
		break;
	case CAN_MCAN_GFC:
		/* Map fields to RXGFC including STM32 FDCAN LSS and LSE fields */
		bits |= FIELD_PREP(CAN_STM32FD_RXGFC_LSS, CONFIG_CAN_MAX_STD_ID_FILTER) |
			FIELD_PREP(CAN_STM32FD_RXGFC_LSE, CONFIG_CAN_MAX_EXT_ID_FILTER);
		bits |= val & (CAN_MCAN_GFC_ANFS | CAN_MCAN_GFC_ANFE |
			CAN_MCAN_GFC_RRFS | CAN_MCAN_GFC_RRFE);
		break;
	default:
		/* No field remap needed */
		bits = val;
		break;
	};

	return can_mcan_sys_write_reg(stm32fd_config->base, remap, bits);
}

static int can_stm32fd_get_core_clock(const struct device *dev, uint32_t *rate)
{
	const uint32_t rate_tmp = LL_RCC_GetFDCANClockFreq(LL_RCC_FDCAN_CLKSOURCE);

	ARG_UNUSED(dev);

	if (rate_tmp == LL_RCC_PERIPH_FREQUENCY_NO) {
		LOG_ERR("Can't read core clock");
		return -EIO;
	}

	if (FDCAN_CONFIG->CKDIV == 0) {
		*rate = rate_tmp;
	} else {
		*rate = rate_tmp / (FDCAN_CONFIG->CKDIV << 1);
	}

	return 0;
}

static int can_stm32fd_clock_enable(const struct device *dev)
{
	int ret;
	const struct can_mcan_config *mcan_cfg = dev->config;
	const struct can_stm32fd_config *stm32fd_cfg = mcan_cfg->custom;
	const struct device *const clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);

	if (!device_is_ready(clk)) {
		return -ENODEV;
	}

	if (IS_ENABLED(STM32_CANFD_DOMAIN_CLOCK_SUPPORT) && (stm32fd_cfg->pclk_len > 1)) {
		ret = clock_control_configure(clk,
				(clock_control_subsys_t)&stm32fd_cfg->pclken[1],
				NULL);
		if (ret < 0) {
			LOG_ERR("Could not select can_stm32fd domain clock");
			return ret;
		}
	}

	ret = clock_control_on(clk, (clock_control_subsys_t)&stm32fd_cfg->pclken[0]);
	if (ret < 0) {
		return ret;
	}

	if (stm32fd_cfg->clock_divider != 0) {
		can_mcan_enable_configuration_change(dev);
		FDCAN_CONFIG->CKDIV = stm32fd_cfg->clock_divider >> 1;
	}

	return 0;
}

static int can_stm32fd_init(const struct device *dev)
{
	const struct can_mcan_config *mcan_cfg = dev->config;
	const struct can_stm32fd_config *stm32fd_cfg = mcan_cfg->custom;
	uint32_t rxgfc;
	int ret;

	/* Configure dt provided device signals when available */
	ret = pinctrl_apply_state(stm32fd_cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("CAN pinctrl setup failed (%d)", ret);
		return ret;
	}

	ret = can_stm32fd_clock_enable(dev);
	if (ret < 0) {
		LOG_ERR("Could not turn on CAN clock (%d)", ret);
		return ret;
	}

	can_mcan_enable_configuration_change(dev);

	/* Setup STM32 FDCAN Global Filter Configuration register */
	ret = can_mcan_read_reg(dev, CAN_STM32FD_RXGFC, &rxgfc);
	if (ret != 0) {
		return ret;
	}

	rxgfc |= FIELD_PREP(CAN_STM32FD_RXGFC_LSS, CONFIG_CAN_MAX_STD_ID_FILTER) |
		 FIELD_PREP(CAN_STM32FD_RXGFC_LSE, CONFIG_CAN_MAX_EXT_ID_FILTER);

	ret = can_mcan_write_reg(dev, CAN_STM32FD_RXGFC, rxgfc);
	if (ret != 0) {
		return ret;
	}

	/* Setup STM32 FDCAN Tx buffer configuration register */
	ret = can_mcan_write_reg(dev, CAN_MCAN_TXBC, CAN_STM32FD_TXBC_TFQM);
	if (ret != 0) {
		return ret;
	}

	ret = can_mcan_init(dev);
	if (ret != 0) {
		return ret;
	}

	stm32fd_cfg->config_irq();

	return ret;
}

static const struct can_driver_api can_stm32fd_driver_api = {
	.get_capabilities = can_mcan_get_capabilities,
	.start = can_mcan_start,
	.stop = can_mcan_stop,
	.set_mode = can_mcan_set_mode,
	.set_timing = can_mcan_set_timing,
	.send = can_mcan_send,
	.add_rx_filter = can_mcan_add_rx_filter,
	.remove_rx_filter = can_mcan_remove_rx_filter,
	.get_state = can_mcan_get_state,
#ifndef CONFIG_CAN_AUTO_BUS_OFF_RECOVERY
	.recover = can_mcan_recover,
#endif /* CONFIG_CAN_AUTO_BUS_OFF_RECOVERY */
	.get_core_clock = can_stm32fd_get_core_clock,
	.get_max_bitrate = can_mcan_get_max_bitrate,
	.get_max_filters = can_mcan_get_max_filters,
	.set_state_change_callback = can_mcan_set_state_change_callback,
	.timing_min = {
		.sjw = 0x01,
		.prop_seg = 0x00,
		.phase_seg1 = 0x01,
		.phase_seg2 = 0x01,
		.prescaler = 0x01
	},
	.timing_max = {
		.sjw = 0x80,
		.prop_seg = 0x00,
		.phase_seg1 = 0x100,
		.phase_seg2 = 0x80,
		.prescaler = 0x200
	},
#ifdef CONFIG_CAN_FD_MODE
	.set_timing_data = can_mcan_set_timing_data,
	.timing_data_min = {
		.sjw = 0x01,
		.prop_seg = 0x00,
		.phase_seg1 = 0x01,
		.phase_seg2 = 0x01,
		.prescaler = 0x01
	},
	.timing_data_max = {
		.sjw = 0x10,
		.prop_seg = 0x00,
		.phase_seg1 = 0x20,
		.phase_seg2 = 0x10,
		.prescaler = 0x20
	}
#endif /* CONFIG_CAN_FD_MODE */
};

/* Assert that the Message RAM configuration meets the hardware limitiations */
BUILD_ASSERT(NUM_STD_FILTER_ELEMENTS <= 28, "Maximum standard filter elements exceeded");
BUILD_ASSERT(NUM_EXT_FILTER_ELEMENTS <= 8, "Maximum extended filter elements exceeded");
BUILD_ASSERT(NUM_RX_FIFO0_ELEMENTS == 3, "Rx FIFO 0 elements must be 3");
BUILD_ASSERT(NUM_RX_FIFO1_ELEMENTS == 3, "Rx FIFO 1 elements must be 3");
BUILD_ASSERT(NUM_RX_BUF_ELEMENTS == 0, "Rx Buffer elements must be 0");
BUILD_ASSERT(NUM_TX_BUF_ELEMENTS == 3, "Tx Buffer elements must be 0");

#define CAN_STM32FD_IRQ_CFG_FUNCTION(inst)                                     \
static void config_can_##inst##_irq(void)                                      \
{                                                                              \
	LOG_DBG("Enable CAN" #inst " IRQ");                                    \
	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(inst, line_0, irq),                    \
		    DT_INST_IRQ_BY_NAME(inst, line_0, priority),               \
		    can_mcan_line_0_isr, DEVICE_DT_INST_GET(inst), 0);         \
	irq_enable(DT_INST_IRQ_BY_NAME(inst, line_0, irq));                    \
	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(inst, line_1, irq),                    \
		    DT_INST_IRQ_BY_NAME(inst, line_1, priority),               \
		    can_mcan_line_1_isr, DEVICE_DT_INST_GET(inst), 0);         \
	irq_enable(DT_INST_IRQ_BY_NAME(inst, line_1, irq));                    \
}

#define CAN_STM32FD_CFG_INST(inst)					\
	PINCTRL_DT_INST_DEFINE(inst);					\
	static const struct stm32_pclken can_stm32fd_pclken_##inst[] =	\
					STM32_DT_INST_CLOCKS(inst);	\
									\
	static const struct can_stm32fd_config can_stm32fd_cfg_##inst = { \
		.base = (mm_reg_t)DT_INST_REG_ADDR_BY_NAME(inst, m_can), \
		.pclken = can_stm32fd_pclken_##inst,			\
		.pclk_len = DT_INST_NUM_CLOCKS(inst),			\
		.config_irq = config_can_##inst##_irq,			\
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),		\
		.clock_divider = DT_INST_PROP_OR(inst, clk_divider, 0)  \
	};								\
									\
	static const struct can_mcan_config can_mcan_cfg_##inst =	\
		CAN_MCAN_DT_CONFIG_INST_GET(inst, &can_stm32fd_cfg_##inst, \
					    can_stm32fd_read_reg,	\
					    can_stm32fd_write_reg);

#define CAN_STM32FD_DATA_INST(inst)					\
	static struct can_mcan_data can_mcan_data_##inst =		\
		CAN_MCAN_DATA_INITIALIZER((struct can_mcan_msg_sram *)	\
			DT_INST_REG_ADDR_BY_NAME(inst, message_ram),	\
			NULL);

#define CAN_STM32FD_DEVICE_INST(inst)					\
	DEVICE_DT_INST_DEFINE(inst, &can_stm32fd_init, NULL,		\
			      &can_mcan_data_##inst, &can_mcan_cfg_##inst, \
			      POST_KERNEL, CONFIG_CAN_INIT_PRIORITY,	\
			      &can_stm32fd_driver_api);

#define CAN_STM32FD_INST(inst)     \
CAN_STM32FD_IRQ_CFG_FUNCTION(inst) \
CAN_STM32FD_CFG_INST(inst)         \
CAN_STM32FD_DATA_INST(inst)        \
CAN_STM32FD_DEVICE_INST(inst)

DT_INST_FOREACH_STATUS_OKAY(CAN_STM32FD_INST)
