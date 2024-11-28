/*
 * Copyright (c) 2022 Vestas Wind Systems A/S
 * Copyright (c) 2020 Alexander Wachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/can.h>
#include <zephyr/drivers/can/can_mcan.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/__assert.h>
#include <soc.h>
#include <stm32_ll_rcc.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>

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
	mem_addr_t mram;
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
		break;
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
		__fallthrough;
	case CAN_MCAN_IE:
		/* Remap IR/IE bits, ignoring unsupported bits */
		/* Group 1 map bits 23-16 (stm32fd) to 29-22 (mcan) */
		*val |= ((bits & GENMASK(23, 16)) << 6);

		/* Group 2 map bits 15-11 (stm32fd) to 18-14 (mcan) */
		*val |= ((bits & GENMASK(15, 11)) << 3);

		/* Group 3 map bits 10-4 (stm32fd) to 12-6 (mcan) */
		*val |= ((bits & GENMASK(10, 4)) << 2);

		/* Group 4 map bits 3-1 (stm32fd) to 4-2 (mcan) */
		*val |= ((bits & GENMASK(3, 1)) << 1);

		/* Group 5 map bits 0 (mcan) to 0 (stm32fd) */
		*val |= ((bits & GENMASK(0, 0)) << 0);
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
		__fallthrough;
	case CAN_MCAN_IE:
		/* Remap IR/IE bits, ignoring unsupported bits */
		/* Group 1 map bits 29-22 (mcan) to 23-16 (stm32fd) */
		bits |= ((val & GENMASK(29, 22)) >> 6);

		/* Group 2 map bits 18-14 (mcan) to 15-11 (stm32fd) */
		bits |= ((val & GENMASK(18, 14)) >> 3);

		/* Group 3 map bits 12-6 (mcan) to 10-4 (stm32fd) */
		bits |= ((val & GENMASK(12, 6)) >> 2);

		/* Group 4 map bits 4-2 (mcan) to 3-1 (stm32fd) */
		bits |= ((val & GENMASK(4, 2)) >> 1);

		/* Group 5 map bits 0 (mcan) to 0 (stm32fd) */
		bits |= ((val & GENMASK(0, 0)) >> 0);
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

static int can_stm32fd_read_mram(const struct device *dev, uint16_t offset, void *dst, size_t len)
{
	const struct can_mcan_config *mcan_config = dev->config;
	const struct can_stm32fd_config *stm32fd_config = mcan_config->custom;

	return can_mcan_sys_read_mram(stm32fd_config->mram, offset, dst, len);
}

static int can_stm32fd_write_mram(const struct device *dev, uint16_t offset, const void *src,
				size_t len)
{
	const struct can_mcan_config *mcan_config = dev->config;
	const struct can_stm32fd_config *stm32fd_config = mcan_config->custom;

	return can_mcan_sys_write_mram(stm32fd_config->mram, offset, src, len);
}

static int can_stm32fd_clear_mram(const struct device *dev, uint16_t offset, size_t len)
{
	const struct can_mcan_config *mcan_config = dev->config;
	const struct can_stm32fd_config *stm32fd_config = mcan_config->custom;

	return can_mcan_sys_clear_mram(stm32fd_config->mram, offset, len);
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

static DEVICE_API(can, can_stm32fd_driver_api) = {
	.get_capabilities = can_mcan_get_capabilities,
	.start = can_mcan_start,
	.stop = can_mcan_stop,
	.set_mode = can_mcan_set_mode,
	.set_timing = can_mcan_set_timing,
	.send = can_mcan_send,
	.add_rx_filter = can_mcan_add_rx_filter,
	.remove_rx_filter = can_mcan_remove_rx_filter,
	.get_state = can_mcan_get_state,
#ifdef CONFIG_CAN_MANUAL_RECOVERY_MODE
	.recover = can_mcan_recover,
#endif /* CONFIG_CAN_MANUAL_RECOVERY_MODE */
	.get_core_clock = can_stm32fd_get_core_clock,
	.get_max_filters = can_mcan_get_max_filters,
	.set_state_change_callback = can_mcan_set_state_change_callback,
	.timing_min = CAN_MCAN_TIMING_MIN_INITIALIZER,
	.timing_max = CAN_MCAN_TIMING_MAX_INITIALIZER,
#ifdef CONFIG_CAN_FD_MODE
	.set_timing_data = can_mcan_set_timing_data,
	.timing_data_min = CAN_MCAN_TIMING_DATA_MIN_INITIALIZER,
	.timing_data_max = CAN_MCAN_TIMING_DATA_MAX_INITIALIZER,
#endif /* CONFIG_CAN_FD_MODE */
};

static const struct can_mcan_ops can_stm32fd_ops = {
	.read_reg = can_stm32fd_read_reg,
	.write_reg = can_stm32fd_write_reg,
	.read_mram = can_stm32fd_read_mram,
	.write_mram = can_stm32fd_write_mram,
	.clear_mram = can_stm32fd_clear_mram,
};

#define CAN_STM32FD_BUILD_ASSERT_MRAM_CFG(inst)					\
	BUILD_ASSERT(CAN_MCAN_DT_INST_MRAM_STD_FILTER_ELEMENTS(inst) == 28,	\
		     "Standard filter elements must be 28");			\
	BUILD_ASSERT(CAN_MCAN_DT_INST_MRAM_EXT_FILTER_ELEMENTS(inst) == 8,	\
		     "Extended filter elements must be 8");			\
	BUILD_ASSERT(CAN_MCAN_DT_INST_MRAM_RX_FIFO0_ELEMENTS(inst) == 3,	\
		     "Rx FIFO 0 elements must be 3");				\
	BUILD_ASSERT(CAN_MCAN_DT_INST_MRAM_RX_FIFO1_ELEMENTS(inst) == 3,	\
		     "Rx FIFO 1 elements must be 3");				\
	BUILD_ASSERT(CAN_MCAN_DT_INST_MRAM_RX_BUFFER_ELEMENTS(inst) == 0,	\
		     "Rx Buffer elements must be 0");				\
	BUILD_ASSERT(CAN_MCAN_DT_INST_MRAM_TX_EVENT_FIFO_ELEMENTS(inst) == 3,	\
		     "Tx Event FIFO elements must be 3");			\
	BUILD_ASSERT(CAN_MCAN_DT_INST_MRAM_TX_BUFFER_ELEMENTS(inst) == 3,	\
		     "Tx Buffer elements must be 0");

#define CAN_STM32FD_IRQ_CFG_FUNCTION(inst)                                     \
static void config_can_##inst##_irq(void)                                      \
{                                                                              \
	LOG_DBG("Enable CAN" #inst " IRQ");                                    \
	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(inst, int0, irq),                      \
		    DT_INST_IRQ_BY_NAME(inst, int0, priority),                 \
		    can_mcan_line_0_isr, DEVICE_DT_INST_GET(inst), 0);         \
	irq_enable(DT_INST_IRQ_BY_NAME(inst, int0, irq));                      \
	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(inst, int1, irq),                      \
		    DT_INST_IRQ_BY_NAME(inst, int1, priority),                 \
		    can_mcan_line_1_isr, DEVICE_DT_INST_GET(inst), 0);         \
	irq_enable(DT_INST_IRQ_BY_NAME(inst, int1, irq));                      \
}

#define CAN_STM32FD_CFG_INST(inst)					\
	BUILD_ASSERT(CAN_MCAN_DT_INST_MRAM_ELEMENTS_SIZE(inst) <=	\
		     CAN_MCAN_DT_INST_MRAM_SIZE(inst),			\
		     "Insufficient Message RAM size to hold elements");	\
									\
	PINCTRL_DT_INST_DEFINE(inst);					\
	CAN_MCAN_CALLBACKS_DEFINE(can_stm32fd_cbs_##inst,		\
				  CAN_MCAN_DT_INST_MRAM_TX_BUFFER_ELEMENTS(inst), \
				  CONFIG_CAN_MAX_STD_ID_FILTER,		\
				  CONFIG_CAN_MAX_EXT_ID_FILTER);	\
									\
	static const struct stm32_pclken can_stm32fd_pclken_##inst[] =	\
					STM32_DT_INST_CLOCKS(inst);	\
									\
	static const struct can_stm32fd_config can_stm32fd_cfg_##inst = { \
		.base = CAN_MCAN_DT_INST_MCAN_ADDR(inst),		\
		.mram = CAN_MCAN_DT_INST_MRAM_ADDR(inst),		\
		.pclken = can_stm32fd_pclken_##inst,			\
		.pclk_len = DT_INST_NUM_CLOCKS(inst),			\
		.config_irq = config_can_##inst##_irq,			\
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),		\
		.clock_divider = DT_INST_PROP_OR(inst, clk_divider, 0)  \
	};								\
									\
	static const struct can_mcan_config can_mcan_cfg_##inst =	\
		CAN_MCAN_DT_CONFIG_INST_GET(inst, &can_stm32fd_cfg_##inst, \
					    &can_stm32fd_ops,		\
					    &can_stm32fd_cbs_##inst);

#define CAN_STM32FD_DATA_INST(inst)					\
	static struct can_mcan_data can_mcan_data_##inst =		\
		CAN_MCAN_DATA_INITIALIZER(NULL);

#define CAN_STM32FD_DEVICE_INST(inst)						\
	CAN_DEVICE_DT_INST_DEFINE(inst, can_stm32fd_init, NULL,			\
				  &can_mcan_data_##inst, &can_mcan_cfg_##inst,	\
				  POST_KERNEL, CONFIG_CAN_INIT_PRIORITY,	\
				  &can_stm32fd_driver_api);

#define CAN_STM32FD_INST(inst)          \
CAN_STM32FD_BUILD_ASSERT_MRAM_CFG(inst) \
CAN_STM32FD_IRQ_CFG_FUNCTION(inst)      \
CAN_STM32FD_CFG_INST(inst)              \
CAN_STM32FD_DATA_INST(inst)             \
CAN_STM32FD_DEVICE_INST(inst)

DT_INST_FOREACH_STATUS_OKAY(CAN_STM32FD_INST)
