/*
 * Copyright (c) 2019 Intel Corporation
 * Copyright (c) 2021 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_xec_espi_host_dev

#include <kernel.h>
#include <soc.h>
#include <errno.h>
#include <drivers/espi.h>
#include <drivers/clock_control/mchp_xec_clock_control.h>
#include <drivers/interrupt_controller/intc_mchp_xec_ecia.h>
#include <dt-bindings/interrupt-controller/mchp-xec-ecia.h>
#include <logging/log.h>
#include <sys/sys_io.h>
#include <sys/util.h>
#include "espi_utils.h"
#include "espi_mchp_xec_v2.h"

#define CONNECT_IRQ_MBOX0	NULL
#define CONNECT_IRQ_KBC0	NULL
#define CONNECT_IRQ_ACPI_EC0	NULL
#define CONNECT_IRQ_ACPI_EC1	NULL
#define CONNECT_IRQ_ACPI_EC2	NULL
#define CONNECT_IRQ_ACPI_EC3	NULL
#define CONNECT_IRQ_ACPI_EC4	NULL
#define CONNECT_IRQ_ACPI_PM1	NULL
#define CONNECT_IRQ_EMI0	NULL
#define CONNECT_IRQ_EMI1	NULL
#define CONNECT_IRQ_EMI2	NULL
#define CONNECT_IRQ_RTC0	NULL
#define CONNECT_IRQ_P80BD0	NULL

#define INIT_MBOX0		NULL
#define INIT_KBC0		NULL
#define INIT_ACPI_EC0		NULL
#define INIT_ACPI_EC1		NULL
#define INIT_ACPI_EC2		NULL
#define INIT_ACPI_EC3		NULL
#define INIT_ACPI_EC4		NULL
#define INIT_ACPI_PM1		NULL
#define INIT_EMI0		NULL
#define INIT_EMI1		NULL
#define INIT_EMI2		NULL
#define INIT_RTC0		NULL
#define INIT_P80BD0		NULL
#define	INIT_UART0		NULL
#define	INIT_UART1		NULL

/* BARs as defined in LPC spec chapter 11 */
#define ESPI_XEC_KBC_BAR_ADDRESS    0x00600000
#define ESPI_XEC_UART0_BAR_ADDRESS  0x03F80000
#define ESPI_XEC_MBOX_BAR_ADDRESS   0x03600000
#define ESPI_XEC_PORT80_BAR_ADDRESS 0x00800000
#define ESPI_XEC_PORT81_BAR_ADDRESS 0x00810000
#define ESPI_XEC_ACPI_EC0_BAR_ADDRESS   0x00620000

/* Espi peripheral has 3 uart ports */
#define ESPI_PERIPHERAL_UART_PORT0  0
#define ESPI_PERIPHERAL_UART_PORT1  1

#define UART_DEFAULT_IRQ_POS	    2u
#define UART_DEFAULT_IRQ	    BIT(UART_DEFAULT_IRQ_POS)

/* PCR */
#define XEC_PCR_REG_BASE						\
	((struct pcr_regs *)(DT_REG_ADDR(DT_NODELABEL(pcr))))

struct xec_espi_host_sram_config {
	uint32_t host_sram1_base;
	uint32_t host_sram2_base;
	uint16_t ec_sram1_ofs;
	uint16_t ec_sram2_ofs;
	uint8_t sram1_acc_size;
	uint8_t sram2_acc_size;
};

struct xec_espi_host_dev_config {
	const struct device *parent;
	uint32_t reg_base;		/* logical device registers */
	uint32_t host_mem_base;		/* 32-bit host memory address */
	uint16_t host_io_base;		/* 16-bit host I/O address */
	uint8_t ldn;			/* Logical device number */
	uint8_t num_ecia;
	uint32_t *girqs;
};

struct xec_acpi_ec_config {
	uintptr_t regbase;
	uint32_t ibf_ecia_info;
	uint32_t obe_ecia_info;
};


#ifdef CONFIG_ESPI_PERIPHERAL_XEC_MAILBOX

BUILD_ASSERT(DT_NODE_HAS_STATUS(DT_NODELABEL(mbox0), okay),
	     "XEC mbox0 DT node is disabled!");

static struct xec_mbox_config {
	uintptr_t regbase;
	uint32_t ecia_info;
};

static const struct xec_mbox0_config xec_mbox0_cfg = {
	.regbase = DT_REG_ADDR(DT_NODELABEL(mbox0)),
	.ecia_info = DT_PROP_BY_IDX(DT_NODELABEL(mbox0), girqs, 0),
};

/* dev is a pointer to espi0 (parent) device */
static void mbox0_isr(const struct device *dev)
{
	uint8_t girq = MCHP_XEC_ECIA_GIRQ(xec_mbox0_cfg.ecia_info);
	uint8_t bitpos = MCHP_XEC_ECIA_GIRQ_POS(xec_mbox0_cfg.ecia_info);

	/* clear GIRQ source, inline version */
	mchp_soc_ecia_girq_src_clr(girq, bitpos);
}

static int connect_irq_mbox0(const struct device *dev)
{
	/* clear GIRQ source */
	mchp_xec_ecia_info_girq_src_clr(xec_mbox0_cfg.ecia_info);

	IRQ_CONNECT(DT_IRQN(DT_NODELABLE(mbox0)),
		    DT_IRQ(DT_NODELABLE(mbox0), priority),
		    acpi_ec0_isr,
		    DEVICE_DT_GET(DT_NODELABEL(espi0)),
		    0);
	irq_enable(DT_IRQN(DT_NODELABLE(mbox0)));

	/* enable GIRQ source */
	mchp_xec_ecia_info_girq_src_en(xec_mbox0_cfg.ecia_info);

	return 0;
}

/* Called by eSPI Bus init, eSPI reset de-assertion, and eSPI Platform Reset
 * de-assertion.
 */
static int init_mbox0(const struct device *dev)
{
	struct espi_xec_config *const cfg = ESPI_XEC_CONFIG(dev);
	struct espi_iom_regs *regs = (struct espi_iom_regs *)cfg->base_addr;

	regs->IOHBAR[IOB_MBOX] = ESPI_XEC_MBOX_BAR_ADDRESS |
				 MCHP_ESPI_IO_BAR_HOST_VALID;
	return 0;
}

#undef	CONNECT_IRQ_MBOX0
#define	CONNECT_IRQ_MBOX0	connect_irq_mbox0
#undef	INIT_MBOX0
#define	INIT_MBOX0		init_mbox0

#endif /* CONFIG_ESPI_PERIPHERAL_XEC_MAILBOX */

#ifdef CONFIG_ESPI_PERIPHERAL_8042_KBC

BUILD_ASSERT(DT_NODE_HAS_STATUS(DT_NODELABEL(kbc0), okay),
	     "XEC kbc0 DT node is disabled!");

struct xec_kbc0_config {
	uintptr_t regbase;
	uint32_t ibf_ecia_info;
	uint32_t obe_ecia_info;
};

static const struct xec_kbc0_config xec_kbc0_cfg = {
	.regbase = DT_REG_ADDR(DT_NODELABEL(kbc0)),
	.ibf_ecia_info = DT_PROP_BY_IDX(DT_NODELABEL(kbc0), girqs, 1),
	.obe_ecia_info = DT_PROP_BY_IDX(DT_NODELABEL(kbc0), girqs, 0),
};

static void kbc0_ibf_isr(const struct device *dev)
{
	struct kbc_regs *kbc_hw = (struct kbc_regs *)xec_kbc0_cfg.regbase;
	struct espi_xec_data *const data =
		(struct espi_xec_data *const)dev->data;


	/* The high byte contains information from the host,
	 * and the lower byte speficies if the host sent
	 * a command or data. 1 = Command.
	 */
	uint32_t isr_data = ((kbc_hw->EC_DATA & 0xFF) << E8042_ISR_DATA_POS) |
				((kbc_hw->EC_KBC_STS & MCHP_KBC_STS_CD) <<
				 E8042_ISR_CMD_DATA_POS);

	struct espi_event evt = {
		.evt_type = ESPI_BUS_PERIPHERAL_NOTIFICATION,
		.evt_details = ESPI_PERIPHERAL_8042_KBC,
		.evt_data = isr_data
	};

	espi_send_callbacks(&data->callbacks, dev, evt);

	mchp_xec_ecia_info_girq_src_clr(xec_kbc0_cfg.ibf_ecia_info);
}

static void kbc0_obe_isr(const struct device *dev)
{
	/* disable and clear GIRQ interrupt and status */
	mchp_xec_ecia_info_girq_src_dis(xec_kbc0_cfg.obe_ecia_info);
	mchp_xec_ecia_info_girq_src_clr(xec_kbc0_cfg.obe_ecia_info);
}

/* dev is a pointer to espi0 device */
static int kbc0_rd_req(const struct device *dev, enum lpc_peripheral_opcode op,
		       uint32_t *data)
{
	struct kbc_regs *kbc_hw = (struct kbc_regs *)xec_kbc0_cfg.regbase;

	ARG_UNUSED(dev);

	if (op >= E8042_START_OPCODE && op <= E8042_MAX_OPCODE) {
		/* Make sure kbc 8042 is on */
		if (!(kbc_hw->KBC_CTRL & MCHP_KBC_CTRL_OBFEN)) {
			return -ENOTSUP;
		}

		switch (op) {
		case E8042_OBF_HAS_CHAR:
			/* EC has written data back to host. OBF is
			 * automatically cleared after host reads
			 * the data
			 */
			*data = kbc_hw->EC_KBC_STS & MCHP_KBC_STS_OBF ? 1 : 0;
			break;
		case E8042_IBF_HAS_CHAR:
			*data = kbc_hw->EC_KBC_STS & MCHP_KBC_STS_IBF ? 1 : 0;
			break;
		case E8042_READ_KB_STS:
			*data = kbc_hw->EC_KBC_STS;
			break;
		default:
			return -EINVAL;
		}
	} else {
		return -ENOTSUP;
	}

	return 0;
}

/* dev is a pointer to espi0 device */
static int kbc0_wr_req(const struct device *dev, enum lpc_peripheral_opcode op,
		       uint32_t *data)
{
	struct kbc_regs *kbc_hw = (struct kbc_regs *)xec_kbc0_cfg.regbase;

	volatile uint32_t __attribute__((unused)) dummy;

	ARG_UNUSED(dev);

	if (op >= E8042_START_OPCODE && op <= E8042_MAX_OPCODE) {
		/* Make sure kbc 8042 is on */
		if (!(kbc_hw->KBC_CTRL & MCHP_KBC_CTRL_OBFEN)) {
			return -ENOTSUP;
		}

		switch (op) {
		case E8042_WRITE_KB_CHAR:
			kbc_hw->EC_DATA = *data & 0xff;
			break;
		case E8042_WRITE_MB_CHAR:
			kbc_hw->EC_AUX_DATA = *data & 0xff;
			break;
		case E8042_RESUME_IRQ:
			mchp_xec_ecia_info_girq_src_clr(
				xec_kbc0_cfg.ibf_ecia_info);
			mchp_xec_ecia_info_girq_src_en(
				xec_kbc0_cfg.ibf_ecia_info);
			break;
		case E8042_PAUSE_IRQ:
			mchp_xec_ecia_info_girq_src_dis(
				xec_kbc0_cfg.ibf_ecia_info);
			break;
		case E8042_CLEAR_OBF:
			dummy = kbc_hw->HOST_AUX_DATA;
			break;
		case E8042_SET_FLAG:
			/* FW shouldn't modify these flags directly */
			*data &= ~(MCHP_KBC_STS_OBF | MCHP_KBC_STS_IBF |
				   MCHP_KBC_STS_AUXOBF);
			kbc_hw->EC_KBC_STS |= *data;
			break;
		case E8042_CLEAR_FLAG:
			/* FW shouldn't modify these flags directly */
			*data |= (MCHP_KBC_STS_OBF | MCHP_KBC_STS_IBF |
				  MCHP_KBC_STS_AUXOBF);
			kbc_hw->EC_KBC_STS &= ~(*data);
			break;
		default:
			return -EINVAL;
		}
	} else {
		return -ENOTSUP;
	}

	return 0;
}

static int connect_irq_kbc0(const struct device *dev)
{
	/* clear GIRQ source */
	mchp_xec_ecia_info_girq_src_clr(xec_kbc0_cfg.ibf_ecia_info);
	mchp_xec_ecia_info_girq_src_clr(xec_kbc0_cfg.obe_ecia_info);

	IRQ_CONNECT(DT_IRQ_BY_NAME(DT_NODELABEL(kbc0), kbc_ibf, irq),
		    DT_IRQ_BY_NAME(DT_NODELABEL(kbc0), kbc_ibf, priority),
		    kbc0_ibf_isr,
		    DEVICE_DT_GET(DT_NODELABEL(espi0)),
		    0);
	irq_enable(DT_IRQ_BY_NAME(DT_NODELABEL(kbc0), kbc_ibf, irq));

	IRQ_CONNECT(DT_IRQ_BY_NAME(DT_NODELABEL(kbc0), kbc_obe, irq),
		    DT_IRQ_BY_NAME(DT_NODELABEL(kbc0), kbc_obe, priority),
		    kbc0_obe_isr,
		    DEVICE_DT_GET(DT_NODELABEL(espi0)),
		    0);
	irq_enable(DT_IRQ_BY_NAME(DT_NODELABEL(kbc0), kbc_obe, irq));

	/* enable GIRQ sources */
	mchp_xec_ecia_info_girq_src_en(xec_kbc0_cfg.ibf_ecia_info);
	mchp_xec_ecia_info_girq_src_en(xec_kbc0_cfg.obe_ecia_info);

	return 0;
}

static int init_kbc0(const struct device *dev)
{
	struct espi_xec_config *const cfg = ESPI_XEC_CONFIG(dev);
	struct espi_iom_regs *regs = (struct espi_iom_regs *)cfg->base_addr;
	struct kbc_regs *kbc_hw = (struct kbc_regs *)xec_kbc0_cfg.regbase;

	kbc_hw->KBC_CTRL |= MCHP_KBC_CTRL_AUXH;
	kbc_hw->KBC_CTRL |= MCHP_KBC_CTRL_OBFEN;
	/* This is the activate register, but the HAL has a funny name */
	kbc_hw->KBC_PORT92_EN = MCHP_KBC_PORT92_EN;
	regs->IOHBAR[IOB_KBC] = ESPI_XEC_KBC_BAR_ADDRESS |
				MCHP_ESPI_IO_BAR_HOST_VALID;

	return 0;
}

#undef	CONNECT_IRQ_KBC0
#define	CONNECT_IRQ_KBC0	connect_irq_kbc0
#undef	INIT_KBC0
#define	INIT_KBC0		init_kbc0

#endif /* CONFIG_ESPI_PERIPHERAL_8042_KBC */

#ifdef CONFIG_ESPI_PERIPHERAL_HOST_IO

static const struct xec_acpi_ec_config xec_acpi_ec0_cfg = {
	.regbase = DT_REG_ADDR(DT_NODELABEL(acpi_ec0)),
	.ibf_ecia_info = DT_PROP_BY_IDX(DT_NODELABEL(acpi_ec0), girqs, 0),
	.obe_ecia_info = DT_PROP_BY_IDX(DT_NODELABEL(acpi_ec0), girqs, 1),
};

static void acpi_ec0_ibf_isr(const struct device *dev)
{
	struct espi_xec_data *const data =
		(struct espi_xec_data *const)dev->data;
	struct espi_event evt = { ESPI_BUS_PERIPHERAL_NOTIFICATION,
		ESPI_PERIPHERAL_HOST_IO, ESPI_PERIPHERAL_NODATA
	};

	espi_send_callbacks(&data->callbacks, dev, evt);

	/* clear GIRQ status */
	mchp_xec_ecia_info_girq_src_clr(xec_acpi_ec0_cfg.ibf_ecia_info);
}

static void acpi_ec0_obe_isr(const struct device *dev)
{
	/* disable and clear GIRQ status */
	mchp_xec_ecia_info_girq_src_dis(xec_acpi_ec0_cfg.obe_ecia_info);
	mchp_xec_ecia_info_girq_src_clr(xec_acpi_ec0_cfg.obe_ecia_info);
}

static int eacpi_rd_req(const struct device *dev,
			enum lpc_peripheral_opcode op,
			uint32_t *data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(op);
	ARG_UNUSED(data);

	return -EINVAL;
}

static int eacpi_wr_req(const struct device *dev,
			enum lpc_peripheral_opcode op,
			uint32_t *data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(op);
	ARG_UNUSED(data);

	return -EINVAL;
}

static int connect_irq_acpi_ec0(const struct device *dev)
{
	mchp_xec_ecia_info_girq_src_clr(xec_acpi_ec0_cfg.ibf_ecia_info);
	mchp_xec_ecia_info_girq_src_clr(xec_acpi_ec0_cfg.obe_ecia_info);

	IRQ_CONNECT(DT_IRQ_BY_NAME(DT_NODELABEL(acpi_ec0), acpi_ibf, irq),
		    DT_IRQ_BY_NAME(DT_NODELABEL(acpi_ec0), acpi_ibf, priority),
		    acpi_ec0_ibf_isr,
		    DEVICE_DT_GET(DT_NODELABEL(espi0)),
		    0);
	irq_enable(DT_IRQ_BY_NAME(DT_NODELABEL(acpi_ec0), acpi_ibf, irq));

	IRQ_CONNECT(DT_IRQ_BY_NAME(DT_NODELABEL(acpi_ec0), acpi_obe, irq),
		    DT_IRQ_BY_NAME(DT_NODELABEL(acpi_ec0), acpi_obe, priority),
		    acpi_ec0_obe_isr,
		    DEVICE_DT_GET(DT_NODELABEL(espi0)),
		    0);
	irq_enable(DT_IRQ_BY_NAME(DT_NODELABEL(acpi_ec0), acpi_obe, irq));

	mchp_xec_ecia_info_girq_src_en(xec_acpi_ec0_cfg.ibf_ecia_info);
	mchp_xec_ecia_info_girq_src_en(xec_acpi_ec0_cfg.obe_ecia_info);

	return 0;
}

static int init_acpi_ec0(const struct device *dev)
{
	struct espi_xec_config *const cfg = ESPI_XEC_CONFIG(dev);
	struct espi_iom_regs *regs = (struct espi_iom_regs *)cfg->base_addr;

	regs->IOHBAR[IOB_ACPI_EC0] = ESPI_XEC_ACPI_EC0_BAR_ADDRESS |
				MCHP_ESPI_IO_BAR_HOST_VALID;

	return 0;
}

#undef	CONNECT_IRQ_ACPI_EC0
#define	CONNECT_IRQ_ACPI_EC0	connect_irq_acpi_ec0
#undef	INIT_ACPI_EC0
#define	INIT_ACPI_EC0		init_acpi_ec0

#endif /* CONFIG_ESPI_PERIPHERAL_HOST_IO */

#if defined(CONFIG_ESPI_PERIPHERAL_EC_HOST_CMD) || \
	defined(CONFIG_ESPI_PERIPHERAL_HOST_IO_PVT)

static const struct xec_acpi_ec_config xec_acpi_ec1_cfg = {
	.regbase = DT_REG_ADDR(DT_NODELABEL(acpi_ec1)),
	.ibf_ecia_info = DT_PROP_BY_IDX(DT_NODELABEL(acpi_ec1), girqs, 0),
	.obe_ecia_info = DT_PROP_BY_IDX(DT_NODELABEL(acpi_ec1), girqs, 1),
};

static void acpi_ec1_ibf_isr(const struct device *dev)
{
	struct espi_xec_data *const data =
		(struct espi_xec_data *const)dev->data;
	struct espi_event evt = {
		.evt_type = ESPI_BUS_PERIPHERAL_NOTIFICATION,
#ifdef CONFIG_ESPI_PERIPHERAL_EC_HOST_CMD
		.evt_details = ESPI_PERIPHERAL_EC_HOST_CMD,
#else
		.evt_details = ESPI_PERIPHERAL_HOST_IO_PVT,
#endif
		.evt_data = ESPI_PERIPHERAL_NODATA
	};

	espi_send_callbacks(&data->callbacks, dev, evt);

	/* clear GIRQ status */
	mchp_xec_ecia_info_girq_src_clr(xec_acpi_ec1_cfg.ibf_ecia_info);
}

static void acpi_ec1_obe_isr(const struct device *dev)
{
	/* disable and clear GIRQ status */
	mchp_xec_ecia_info_girq_src_dis(xec_acpi_ec1_cfg.obe_ecia_info);
	mchp_xec_ecia_info_girq_src_clr(xec_acpi_ec1_cfg.obe_ecia_info);
}

static int connect_irq_acpi_ec1(const struct device *dev)
{
	mchp_xec_ecia_info_girq_src_clr(xec_acpi_ec1_cfg.ibf_ecia_info);
	mchp_xec_ecia_info_girq_src_clr(xec_acpi_ec1_cfg.obe_ecia_info);

	IRQ_CONNECT(DT_IRQ_BY_NAME(DT_NODELABEL(acpi_ec1), acpi_ibf, irq),
		    DT_IRQ_BY_NAME(DT_NODELABEL(acpi_ec1), acpi_ibf, priority),
		    acpi_ec1_ibf_isr,
		    DEVICE_DT_GET(DT_NODELABEL(espi0)),
		    0);
	irq_enable(DT_IRQ_BY_NAME(DT_NODELABEL(acpi_ec1), acpi_ibf, irq));

	IRQ_CONNECT(DT_IRQ_BY_NAME(DT_NODELABEL(acpi_ec1), acpi_obe, irq),
		    DT_IRQ_BY_NAME(DT_NODELABEL(acpi_ec1), acpi_obe, priority),
		    acpi_ec1_obe_isr,
		    DEVICE_DT_GET(DT_NODELABEL(espi0)),
		    0);
	irq_enable(DT_IRQ_BY_NAME(DT_NODELABEL(acpi_ec1), acpi_obe, irq));

	mchp_xec_ecia_info_girq_src_en(xec_acpi_ec1_cfg.ibf_ecia_info);
	mchp_xec_ecia_info_girq_src_en(xec_acpi_ec1_cfg.obe_ecia_info);

	return 0;
}

static int init_acpi_ec1(const struct device *dev)
{
	struct espi_xec_config *const cfg = ESPI_XEC_CONFIG(dev);
	struct espi_iom_regs *regs = (struct espi_iom_regs *)cfg->base_addr;

#ifdef CONFIG_ESPI_PERIPHERAL_EC_HOST_CMD
	regs->IOHBAR[IOB_ACPI_EC1] =
				(CONFIG_ESPI_PERIPHERAL_HOST_CMD_DATA_PORT_NUM << 16) |
				MCHP_ESPI_IO_BAR_HOST_VALID;
#else
	regs->IOHBAR[IOB_ACPI_EC1] =
		CONFIG_ESPI_PERIPHERAL_HOST_IO_PVT_PORT_NUM |
		MCHP_ESPI_IO_BAR_HOST_VALID;
	regs->IOHBAR[IOB_MBOX] = ESPI_XEC_MBOX_BAR_ADDRESS |
				 MCHP_ESPI_IO_BAR_HOST_VALID;
#endif

	return 0;
}

#undef	CONNECT_IRQ_ACPI_EC1
#define	CONNECT_IRQ_ACPI_EC1	connect_irq_acpi_ec1
#undef	INIT_ACPI_EC1
#define	INIT_ACPI_EC1		init_acpi_ec1

#endif /* CONFIG_ESPI_PERIPHERAL_EC_HOST_CMD || CONFIG_ESPI_PERIPHERAL_HOST_IO_PVT */

#ifdef CONFIG_ESPI_PERIPHERAL_EC_HOST_CMD

BUILD_ASSERT(DT_NODE_HAS_STATUS(DT_NODELABEL(emi0), okay),
	     "XEC EMI0 DT node is disabled!");

struct xec_emi_config {
	uintptr_t regbase;
};

static const struct xec_emi_config xec_emi0_cfg = {
	.regbase = DT_REG_ADDR(DT_NODELABEL(emi0)),
};

#ifdef CONFIG_ESPI_PERIPHERAL_ACPI_SHM_REGION
static uint8_t ec_host_cmd_sram[CONFIG_ESPI_XEC_PERIPHERAL_HOST_CMD_PARAM_SIZE +
						CONFIG_ESPI_XEC_PERIPHERAL_ACPI_SHD_MEM_SIZE];
#else
static uint8_t ec_host_cmd_sram[CONFIG_ESPI_XEC_PERIPHERAL_HOST_CMD_PARAM_SIZE];
#endif

static int init_emi0(const struct device *dev)
{
	struct espi_xec_config *const cfg = ESPI_XEC_CONFIG(dev);
	struct espi_iom_regs *regs = (struct espi_iom_regs *)cfg->base_addr;
	struct emi_regs *emi_hw =
		(struct emi_regs *)xec_emi0_cfg.regbase;

	regs->IOHBAR[IOB_EMI0] =
				(CONFIG_ESPI_PERIPHERAL_HOST_CMD_PARAM_PORT_NUM << 16) |
				MCHP_ESPI_IO_BAR_HOST_VALID;

	emi_hw->MEM_BA_0 = (uint32_t)ec_host_cmd_sram;
#ifdef CONFIG_ESPI_PERIPHERAL_ACPI_SHM_REGION
	emi_hw->MEM_RL_0 = CONFIG_ESPI_XEC_PERIPHERAL_HOST_CMD_PARAM_SIZE +
						CONFIG_ESPI_XEC_PERIPHERAL_ACPI_SHD_MEM_SIZE;
#else
	emi_hw->MEM_RL_0 = CONFIG_ESPI_XEC_PERIPHERAL_HOST_CMD_PARAM_SIZE;
#endif
	emi_hw->MEM_WL_0 = CONFIG_ESPI_XEC_PERIPHERAL_HOST_CMD_PARAM_SIZE;

	return 0;
}

#undef	INIT_EMI0
#define	INIT_EMI0		init_emi0

#endif /* CONFIG_ESPI_PERIPHERAL_EC_HOST_CMD */

#ifdef CONFIG_ESPI_PERIPHERAL_CUSTOM_OPCODE

static int ecust_rd_req(const struct device *dev,
			enum lpc_peripheral_opcode op,
			uint32_t *data)
{
	ARG_UNUSED(dev);

	switch (op) {
#ifdef CONFIG_ESPI_PERIPHERAL_EC_HOST_CMD
	case ECUSTOM_HOST_CMD_GET_PARAM_MEMORY:
		*data = (uint32_t)ec_host_cmd_sram;
		break;
#endif
	default:
		return -EINVAL;
	}

	return 0;
}

static int ecust_wr_req(const struct device *dev,
			enum lpc_peripheral_opcode op,
			uint32_t *data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(op);
	ARG_UNUSED(data);

	return -EINVAL;
}

#endif /* CONFIG_ESPI_PERIPHERAL_CUSTOM_OPCODE */

#if defined(CONFIG_ESPI_PERIPHERAL_EC_HOST_CMD) && \
	defined(CONFIG_ESPI_PERIPHERAL_ACPI_SHM_REGION)

static int eacpi_shm_rd_req(const struct device *dev,
			enum lpc_peripheral_opcode op,
			uint32_t *data)
{
	ARG_UNUSED(dev);

	switch (op) {
	case EACPI_GET_SHARED_MEMORY:
		*data = (uint32_t)&ec_host_cmd_sram[CONFIG_ESPI_XEC_PERIPHERAL_HOST_CMD_PARAM_SIZE];
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int eacpi_shm_wr_req(const struct device *dev,
			enum lpc_peripheral_opcode op,
			uint32_t *data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(op);
	ARG_UNUSED(data);

	return -EINVAL;
}

#endif /* CONFIG_ESPI_PERIPHERAL_ACPI_SHM_REGION */


#ifdef CONFIG_ESPI_PERIPHERAL_DEBUG_PORT_80

struct xec_p80bd_config {
	uintptr_t regbase;
	uint32_t ecia_info;
};

static const struct xec_p80bd_config xec_p80bd0_cfg = {
	.regbase = DT_REG_ADDR(DT_NODELABEL(p80bd0)),
	.ecia_info = DT_PROP_BY_IDX(DT_NODELABEL(p80bd0), girqs, 0),
};

/*
 * MEC172x P80 BIOS Debug Port hardware captures writes to its 4-byte I/O range
 * Hardware provides status indicating byte lane(s) of each write.
 * We must decode the byte lane information and produce one or more
 * notification packets.
 */
static void p80bd0_isr(const struct device *dev)
{
	struct espi_xec_data *const data =
		(struct espi_xec_data *const)dev->data;
	struct p80bd_regs *p80regs =
		(struct p80bd_regs *)xec_p80bd0_cfg.regbase;
	struct espi_event evt = { ESPI_BUS_PERIPHERAL_NOTIFICATION, 0,
				  ESPI_PERIPHERAL_NODATA };
	int count = 8; /* limit ISR to 8 bytes */
	uint32_t dattr = p80regs->EC_DA;

	/* b[7:0]=8-bit value written, b[15:8]=attributes */
	while ((dattr & MCHP_P80BD_ECDA_NE) && (count--)) { /* Not empty? */
		/* espi_event protocol No Data value is 0 so pick a bit and
		 * set it. This depends on the application.
		 */
		evt.evt_data = (dattr & 0xffu) | BIT(16);
		switch (dattr & MCHP_P80BD_ECDA_LANE_MSK) {
		case MCHP_P80BD_ECDA_LANE_0:
			evt.evt_details |= (ESPI_PERIPHERAL_INDEX_0 << 16) |
					   ESPI_PERIPHERAL_DEBUG_PORT80;
			break;
		case MCHP_P80BD_ECDA_LANE_1:
			evt.evt_details |= (ESPI_PERIPHERAL_INDEX_1 << 16) |
					   ESPI_PERIPHERAL_DEBUG_PORT80;
			break;
		case MCHP_P80BD_ECDA_LANE_2:
			break;
		case MCHP_P80BD_ECDA_LANE_3:
			break;
		default:
			break;
		}

		if (evt.evt_details) {
			espi_send_callbacks(&data->callbacks, dev, evt);
			evt.evt_details = 0;
		}
	}

	/* clear GIRQ status */
	mchp_xec_ecia_info_girq_src_clr(xec_p80bd0_cfg.ecia_info);
}

static int connect_irq_p80bd0(const struct device *dev)
{
	mchp_xec_ecia_info_girq_src_clr(xec_p80bd0_cfg.ecia_info);

	IRQ_CONNECT(DT_IRQN(DT_NODELABEL(p80bd0)),
		    DT_IRQ(DT_NODELABEL(acpi_ec1), priority),
		    p80bd0_isr,
		    DEVICE_DT_GET(DT_NODELABEL(espi0)),
		    0);
	irq_enable(DT_IRQN(DT_NODELABEL(p80bd0)));

	mchp_xec_ecia_info_girq_src_en(xec_p80bd0_cfg.ecia_info);

	return 0;
}

static int init_p80bd0(const struct device *dev)
{
	struct espi_xec_config *const cfg = ESPI_XEC_CONFIG(dev);
	struct espi_iom_regs *regs = (struct espi_iom_regs *)cfg->base_addr;
	struct p80bd_regs *p80bd_hw =
		(struct p80bd_regs *)xec_p80bd0_cfg.regbase;

	regs->IOHBAR[IOB_P80BD] = ESPI_XEC_PORT80_BAR_ADDRESS |
				  MCHP_ESPI_IO_BAR_HOST_VALID;

	p80bd_hw->ACTV = 1;
	p80bd_hw->STS_IEN = MCHP_P80BD_SI_THR_IEN;

	return 0;
}

#undef	CONNECT_IRQ_P80BD0
#define	CONNECT_IRQ_P80BD0	connect_irq_p80bd0
#undef	INIT_P80BD0
#define	INIT_P80BD0		init_p80bd0

#endif /* CONFIG_ESPI_PERIPHERAL_DEBUG_PORT_80 */

#ifdef CONFIG_ESPI_PERIPHERAL_UART

#if CONFIG_ESPI_PERIPHERAL_UART_SOC_MAPPING == 0
int init_uart0(const struct device *dev)
{
	struct espi_xec_config *const cfg = ESPI_XEC_CONFIG(dev);
	struct espi_iom_regs *regs = (struct espi_iom_regs *)cfg->base_addr;

	regs->IOHBAR[IOB_UART0] = ESPI_XEC_UART0_BAR_ADDRESS |
				  MCHP_ESPI_IO_BAR_HOST_VALID;

	return 0;
}

#undef	INIT_UART0
#define	INIT_UART0		init_uart0

#elif CONFIG_ESPI_PERIPHERAL_UART_SOC_MAPPING == 1
int init_uart1(const struct device *dev)
{
	struct espi_xec_config *const cfg = ESPI_XEC_CONFIG(dev);
	struct espi_iom_regs *regs = (struct espi_iom_regs *)cfg->base_addr;

	regs->IOHBAR[IOB_UART1] = ESPI_XEC_UART0_BAR_ADDRESS |
				  MCHP_ESPI_IO_BAR_HOST_VALID;

	return 0;
}

#undef	INIT_UART1
#define	INIT_UART1		init_uart1
#endif /* CONFIG_ESPI_PERIPHERAL_UART_SOC_MAPPING */
#endif /* CONFIG_ESPI_PERIPHERAL_UART */

typedef int (*host_dev_irq_connect)(const struct device *dev);

static const host_dev_irq_connect hdic_tbl[] = {
	CONNECT_IRQ_MBOX0,
	CONNECT_IRQ_KBC0,
	CONNECT_IRQ_ACPI_EC0,
	CONNECT_IRQ_ACPI_EC1,
	CONNECT_IRQ_ACPI_EC2,
	CONNECT_IRQ_ACPI_EC3,
	CONNECT_IRQ_ACPI_EC4,
	CONNECT_IRQ_ACPI_PM1,
	CONNECT_IRQ_EMI0,
	CONNECT_IRQ_EMI1,
	CONNECT_IRQ_EMI2,
	CONNECT_IRQ_RTC0,
	CONNECT_IRQ_P80BD0,
};

typedef int (*host_dev_init)(const struct device *dev);

static const host_dev_init hd_init_tbl[] = {
	INIT_MBOX0,
	INIT_KBC0,
	INIT_ACPI_EC0,
	INIT_ACPI_EC1,
	INIT_ACPI_EC2,
	INIT_ACPI_EC3,
	INIT_ACPI_EC4,
	INIT_ACPI_PM1,
	INIT_EMI0,
	INIT_EMI1,
	INIT_EMI2,
	INIT_RTC0,
	INIT_P80BD0,
	INIT_UART0,
	INIT_UART1,
};

int xec_host_dev_connect_irqs(const struct device *dev)
{
	int ret = 0;

	for (int i = 0; i < ARRAY_SIZE(hdic_tbl); i++) {
		if (hdic_tbl[i] == NULL) {
			continue;
		}

		ret = hdic_tbl[i](dev);
		if (ret < 0) {
			break;
		}
	}

	return ret;
}

int xec_host_dev_init(const struct device *dev)
{
	int ret = 0;

	for (int i = 0; i < ARRAY_SIZE(hd_init_tbl); i++) {
		if (hd_init_tbl[i] == NULL) {
			continue;
		}

		ret = hd_init_tbl[i](dev);
		if (ret < 0) {
			break;
		}
	}

	return ret;
}

#ifdef CONFIG_ESPI_PERIPHERAL_CHANNEL

typedef int (*xec_lpc_req)(const struct device *,
			   enum lpc_peripheral_opcode,
			   uint32_t *);

struct espi_lpc_req {
	uint16_t opcode_start;
	uint16_t opcode_max;
	xec_lpc_req rd_req;
	xec_lpc_req wr_req;
};

static const struct espi_lpc_req espi_lpc_req_tbl[] = {
#ifdef CONFIG_ESPI_PERIPHERAL_8042_KBC
	{ E8042_START_OPCODE, E8042_MAX_OPCODE, kbc0_rd_req, kbc0_wr_req },
#endif
#ifdef CONFIG_ESPI_PERIPHERAL_HOST_IO
	{ EACPI_START_OPCODE, EACPI_MAX_OPCODE, eacpi_rd_req, eacpi_wr_req },
#endif
#if defined(CONFIG_ESPI_PERIPHERAL_EC_HOST_CMD) && \
	defined(CONFIG_ESPI_PERIPHERAL_ACPI_SHM_REGION)
	{ EACPI_GET_SHARED_MEMORY, EACPI_GET_SHARED_MEMORY, eacpi_shm_rd_req, eacpi_shm_wr_req},
#endif
#ifdef CONFIG_ESPI_PERIPHERAL_CUSTOM_OPCODE
	{ ECUSTOM_START_OPCODE, ECUSTOM_MAX_OPCODE, ecust_rd_req, ecust_wr_req},
#endif
};

static int espi_xec_lpc_req(const struct device *dev,
			    enum lpc_peripheral_opcode op,
			    uint32_t  *data, uint8_t write)
{
	ARG_UNUSED(dev);

	for (int i = 0; i < ARRAY_SIZE(espi_lpc_req_tbl); i++) {
		const struct espi_lpc_req *req = &espi_lpc_req_tbl[i];

		if ((op >= req->opcode_start) && (op <= req->opcode_max)) {
			if (write) {
				return req->wr_req(dev, op, data);
			} else {
				return req->rd_req(dev, op, data);
			}
		}
	}

	return -ENOTSUP;
}

/* dev = pointer to espi0 device */
int espi_xec_read_lpc_request(const struct device *dev,
			      enum lpc_peripheral_opcode op,
			      uint32_t  *data)
{
	return espi_xec_lpc_req(dev, op, data, 0);
}

int espi_xec_write_lpc_request(const struct device *dev,
			       enum lpc_peripheral_opcode op,
			       uint32_t  *data)
{
	return espi_xec_lpc_req(dev, op, data, 1);
}
#else
int espi_xec_write_lpc_request(const struct device *dev,
				      enum lpc_peripheral_opcode op,
				      uint32_t *data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(op);
	ARG_UNUSED(data);

	return -ENOTSUP;
}

int espi_xec_read_lpc_request(const struct device *dev,
				     enum lpc_peripheral_opcode op,
				     uint32_t  *data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(op);
	ARG_UNUSED(data);

	return -ENOTSUP;
}
#endif /* CONFIG_ESPI_PERIPHERAL_CHANNEL */
