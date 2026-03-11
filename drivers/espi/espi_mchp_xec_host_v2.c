/*
 * Copyright (c) 2019 Intel Corporation
 * Copyright (c) 2021 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_xec_espi_host_dev

#include <soc.h>
#include <zephyr/drivers/clock_control/mchp_xec_clock_control.h>
#include <zephyr/drivers/espi.h>
#include <zephyr/drivers/espi/mchp_xec_espi.h>
#include <zephyr/drivers/interrupt_controller/intc_mchp_xec_ecia.h>
#include <zephyr/dt-bindings/interrupt-controller/mchp-xec-ecia.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>
#include "espi_utils.h"
#include "espi_mchp_xec_v2.h"

#define CONNECT_IRQ_MBOX0    NULL
#define CONNECT_IRQ_KBC0     NULL
#define CONNECT_IRQ_ACPI_EC0 NULL
#define CONNECT_IRQ_ACPI_EC1 NULL
#define CONNECT_IRQ_ACPI_EC2 NULL
#define CONNECT_IRQ_ACPI_EC3 NULL
#define CONNECT_IRQ_ACPI_EC4 NULL
#define CONNECT_IRQ_ACPI_PM1 NULL
#define CONNECT_IRQ_EMI0     NULL
#define CONNECT_IRQ_EMI1     NULL
#define CONNECT_IRQ_EMI2     NULL
#define CONNECT_IRQ_RTC0     NULL
#define CONNECT_IRQ_P80BD0   NULL

#define INIT_MBOX0    NULL
#define INIT_KBC0     NULL
#define INIT_ACPI_EC0 NULL
#define INIT_ACPI_EC1 NULL
#define INIT_ACPI_EC2 NULL
#define INIT_ACPI_EC3 NULL
#define INIT_ACPI_EC4 NULL
#define INIT_ACPI_PM1 NULL
#define INIT_EMI0     NULL
#define INIT_EMI1     NULL
#define INIT_EMI2     NULL
#define INIT_RTC0     NULL
#define INIT_P80BD0   NULL
#define INIT_UART0    NULL
#define INIT_UART1    NULL

/* Host I/O space addresses for default set of peripherals mapped by eSPI */
#define ESPI_XEC_KBC_HOST_ADDR      0x60U
#define ESPI_XEC_FKBC_HOST_ADDR     0x92U
#define ESPI_XEC_UART_HOST_ADDR     0x3F0U
#define ESPI_XEC_PORT80_HOST_ADDR   0x80U
#define ESPI_XEC_PORT81_HOST_ADDR   0x81U
#define ESPI_XEC_ACPI_EC0_HOST_ADDR 0x62U

/* Espi peripheral has 3 uart ports */
#define ESPI_PERIPHERAL_UART_PORT0 0
#define ESPI_PERIPHERAL_UART_PORT1 1

#define UART_DEFAULT_IRQ 4U

/* eSPI */
#define XEC_ESPI0_NODE     DT_NODELABEL(espi0)
#define XEC_ESPI0_IOC_BASE DT_REG_ADDR_BY_NAME(XEC_ESPI0_NODE, io)
#define XEC_ESPI0_MC_BASE  DT_REG_ADDR_BY_NAME(XEC_ESPI0_NODE, mem)

/* XEC eSPI peripheral channel devices with memory BARs and the SRAM BARs Host address bits[47:32]
 * are specified by two registers. Get the values from device tree.
 */
#define XEC_PC_DEV_MBAR_HOST_ADDR_HIGH DT_PROP_OR(XEC_ESPI0_NODE, host_memmap_addr_high, 0)
#define XEC_PC_SRAM_BAR_HOST_ADDR_HIGH DT_PROP_OR(XEC_ESPI0_NODE, sram_bar_addr_high, 0)

/* PCR */
#define XEC_PCR_REG_BASE DT_REG_ADDR(DT_NODELABEL(pcr))

#define MCHP_P80_MAX_BYTE_COUNT  4
#define MCHP_P80_FIFO_READ_COUNT 8

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
	uint32_t reg_base;      /* logical device registers */
	uint32_t host_mem_base; /* 32-bit host memory address */
	uint16_t host_io_base;  /* 16-bit host I/O address */
	uint8_t ldn;            /* Logical device number */
	uint8_t num_ecia;
	uint32_t *girqs;
};

struct xec_acpi_ec_config {
	uintptr_t regbase;
	uint32_t ibf_ecia_info;
	uint32_t obe_ecia_info;
	uint32_t host_mem_addr;
	uint16_t host_io_addr;
	uint8_t obf_sirq_slot_val;
};

static void xec_ecia_info_girq_ctrl(uint32_t ecia_info, uint8_t en)
{
	uint8_t girq = MCHP_XEC_ECIA_GIRQ(ecia_info);
	uint8_t girq_pos = MCHP_XEC_ECIA_GIRQ_POS(ecia_info);

	soc_ecia_girq_ctrl(girq, girq_pos, en);
}

static void xec_ecia_info_girq_src_clear(uint32_t ecia_info)
{
	uint8_t girq = MCHP_XEC_ECIA_GIRQ(ecia_info);
	uint8_t girq_pos = MCHP_XEC_ECIA_GIRQ_POS(ecia_info);

	soc_ecia_girq_status_clear(girq, girq_pos);
}

#ifdef CONFIG_ESPI_PERIPHERAL_EC_HOST_CMD

#ifdef CONFIG_ESPI_PERIPHERAL_ACPI_SHM_REGION
static uint8_t ec_host_cmd_sram[CONFIG_ESPI_XEC_PERIPHERAL_HOST_CMD_PARAM_SIZE +
				CONFIG_ESPI_XEC_PERIPHERAL_ACPI_SHD_MEM_SIZE] __aligned(8);
#else
static uint8_t ec_host_cmd_sram[CONFIG_ESPI_XEC_PERIPHERAL_HOST_CMD_PARAM_SIZE] __aligned(8);
#endif

#endif /* CONFIG_ESPI_PERIPHERAL_EC_HOST_CMD */

#ifdef CONFIG_ESPI_PERIPHERAL_MAILBOX

#define XEC_MBOX0_NODE DT_NODELABEL(mbox0)

BUILD_ASSERT(DT_NODE_HAS_STATUS_OKAY(XEC_MBOX0_NODE), "XEC mbox0 DT node is disabled!");

#define XEC_MBOX_H2EC_OFS      0x100U
#define XEC_MBOX_EC2H_OFS      0x104U
#define XEC_MBOX_SMI_SRC_OFS   0x108U
#define XEC_MBOX_SMI_MSK_OFS   0x10cU
/* 32 mailbox data registers. 0 < n < 32 */
#define XEC_MBOX_NUM_8BIT_DATA 32U
#define XEC_MBOX_DATA_OFS(n)   (0x110u + (uint32_t)(n))

/* Mailbox data registers accessed as 32-bit. w is 0 through 7 */
#define XEC_MBOX_DATA32_OFS(w) (0x110u + ((uint32_t)(w) * 4U))

#define XEC_MBOX_SIRQ_EV_SLOT_VAL  CONFIG_ESPI_PERIPHERAL_MAILBOX_SIRQ_SLOT
#define XEC_MBOX_SIRQ_SMI_SLOT_VAL CONFIG_ESPI_PERIPHERAL_MAILBOX_SMI_SIRQ_SLOT

#define XEC_MBOX_GET_DATA 0
#define XEC_MBOX_SET_DATA 1U

struct xec_mbox_config {
	uintptr_t regbase;
	uint32_t ecia_info;
	uint32_t host_mem_addr;
	uint16_t host_io_addr;
};

static const struct xec_mbox_config xec_mbox0_cfg = {
	.regbase = (uintptr_t)DT_REG_ADDR(XEC_MBOX0_NODE),
	.ecia_info = (uint32_t)DT_PROP_BY_IDX(XEC_MBOX0_NODE, girqs, 0),
	.host_io_addr = (uint32_t)DT_PROP_OR(XEC_MBOX0_NODE, host_io, UINT16_MAX),
	.host_mem_addr = (uint32_t)DT_PROP_OR(XEC_MBOX0_NODE, host_mem, UINT32_MAX),
};

/* dev is a pointer to espi0 (parent) device
 * Interrupt on Host writing a byte to Host-to-EC mailbox register.
 * EC clears interrupt line by writing 0xFF to Host-to-EC mailbox register.
 * This also clears Host-to-EC to 0. Host can poll Host-to-EC to know
 * EC has processed command. EC may generate an interrupt to the Host
 * by writing a non-zero value to the EC-to-Host mailbox register.
 * EC may choose to enable EC-to-Host Serial-IRQ and/or EC-to-Host SMI delivered
 * by Serial-IRQ.
 * We do not write 0xFF to the Host-to-EC register to clear the interrupt status because
 * the Host may interpret claring of Host-to-EC as the EC has completed the requested
 * operation. Instead, we disable the mailbox interrupt. The application can re-enable the
 * interrupt after writing a value to Host-to-EC via our side-band API.
 */
static void mbox0_isr(const struct device *dev)
{
	struct espi_xec_data *const data = dev->data;
	mm_reg_t mregbase = xec_mbox0_cfg.regbase;
	struct espi_event evt = {
		ESPI_BUS_PERIPHERAL_NOTIFICATION,
		ESPI_PERIPHERAL_MAILBOX,
		ESPI_PERIPHERAL_NODATA,
	};

	evt.evt_data = sys_read8(mregbase + XEC_MBOX_H2EC_OFS);

	xec_ecia_info_girq_ctrl(xec_mbox0_cfg.ecia_info, MCHP_MEC_ECIA_GIRQ_DIS);
	xec_ecia_info_girq_src_clear(xec_mbox0_cfg.ecia_info);

	espi_send_callbacks(&data->callbacks, dev, evt);
}

static int connect_irq_mbox0(const struct device *dev)
{
	xec_ecia_info_girq_src_clear(xec_mbox0_cfg.ecia_info);

	IRQ_CONNECT(DT_IRQN(XEC_MBOX0_NODE), DT_IRQ(XEC_MBOX0_NODE, priority), mbox0_isr,
		    DEVICE_DT_GET(XEC_ESPI0_NODE), 0);
	irq_enable(DT_IRQN(XEC_MBOX0_NODE));

	/* enable GIRQ source */
	xec_ecia_info_girq_ctrl(xec_mbox0_cfg.ecia_info, MCHP_MEC_ECIA_GIRQ_EN);

	return 0;
}

/* Called by eSPI Bus init, eSPI reset de-assertion, and eSPI Platform Reset
 * de-assertion.
 */
static int init_mbox0(const struct device *dev)
{
	const struct espi_xec_config *devcfg = dev->config;
	struct xec_espi_ioc_cfg_regs *regs =
		(struct xec_espi_ioc_cfg_regs *)(devcfg->ioc_base_addr + MCHP_ESPI_IO_CFG_OFS);
	uint32_t bar_val = 0;

#ifdef CONFIG_ESPI_PERIPHERAL_MAILBOX_PORT_NUM
	bar_val = MCHP_ESPI_IO_BAR_HOST_ADDR_SET(CONFIG_ESPI_PERIPHERAL_MAILBOX_PORT_NUM);

	regs->IOHBAR[IOB_MBOX] = bar_val | MCHP_ESPI_IO_BAR_HOST_VALID;
#else
	bar_val = xec_mbox0_cfg.host_io_addr;

	if (bar_val != UINT16_MAX) {
		bar_val = MCHP_ESPI_IO_BAR_HOST_ADDR_SET(bar_val);
		regs->IOHBAR[IOB_MBOX] = bar_val | MCHP_ESPI_IO_BAR_HOST_VALID;
	}

	bar_val = xec_mbox0_cfg.host_mem_addr;
	if (bar_val != UINT32_MAX) {
		xec_espi_mbar_host_set(devcfg->mc_base_addr, MEMB_MBOX, bar_val, true);
	}
#endif

	/* Serial IRQ */
	regs->SIRQ[MCHP_ESPI_SIRQ_MBOX_SIRQ] = XEC_MBOX_SIRQ_EV_SLOT_VAL;
	regs->SIRQ[MCHP_ESPI_SIRQ_MBOX_SMI] = XEC_MBOX_SIRQ_SMI_SLOT_VAL;

	return 0;
}

/* Write all 32 8-bit mailbox data register as 32-bit words. These registers were designed
 * to be accessible as 32-bit words.
 */
static int mbox0_data_set_all(const struct device *dev, const uint32_t *src)
{
	mm_reg_t mregbase = xec_mbox0_cfg.regbase;
	uint32_t n = 0;

	if (src == NULL) {
		return -EINVAL;
	}

	for (n = 0; n < (XEC_MBOX_NUM_8BIT_DATA / 4U); n++) {
		sys_write32(src[n], mregbase + XEC_MBOX_DATA32_OFS(n));
	}

	return 0;
}

/* Get all 32 8-bit mailbox data register as 32-bit words. These registers were designed
 * to be accessible as 32-bit words.
 */
static int mbox0_data_get_all(const struct device *dev, uint32_t *dest)
{
	mm_reg_t mregbase = xec_mbox0_cfg.regbase;
	uint32_t n = 0;

	if (dest == NULL) {
		return -EINVAL;
	}

	for (n = 0; n < (XEC_MBOX_NUM_8BIT_DATA / 4U); n++) {
		dest[n] = sys_read32(mregbase + XEC_MBOX_DATA32_OFS(n));
	}

	return 0;
}

int mchp_xec_espi_pc_mailbox_get(const struct device *dev, uint8_t mbox_idx, uint8_t num_mboxes,
				 uint8_t *dest)
{
	mm_reg_t mregbase = xec_mbox0_cfg.regbase;
	uint8_t nb = 0;

	if ((dev == NULL) || (dest == NULL) || (mbox_idx >= MCHP_XEC_MAX_MAILBOX_INDEX)) {
		return -EINVAL;
	}

	mregbase += XEC_MBOX_DATA_OFS(0);

	for (uint8_t idx = mbox_idx; idx < MCHP_XEC_MAX_MAILBOX_INDEX; idx++) {
		if (idx >= num_mboxes) {
			break;
		}

		dest[nb++] = sys_read8(mregbase + idx);
	}

	return 0;
}

int mchp_xec_espi_pc_mailbox_set(const struct device *dev, uint8_t mbox_idx, uint8_t num_mboxes,
				 const uint8_t *src)
{
	mm_reg_t mregbase = xec_mbox0_cfg.regbase;
	uint8_t nb = 0;

	if ((dev == NULL) || (src == NULL) || (mbox_idx >= MCHP_XEC_MAX_MAILBOX_INDEX)) {
		return -EINVAL;
	}

	mregbase += XEC_MBOX_DATA_OFS(0);

	for (uint8_t idx = mbox_idx; idx < MCHP_XEC_MAX_MAILBOX_INDEX; idx++) {
		if (idx >= num_mboxes) {
			break;
		}

		sys_write8(src[nb], mregbase + idx);
	}

	return 0;
}

int mchp_xec_espi_pc_mailbox_get_all(const struct device *dev, uint32_t *dest)
{
	if (dev == NULL) {
		return -EINVAL;
	}

	return mbox0_data_get_all(dev, dest);
}

int mchp_xec_espi_pc_mailbox_set_all(const struct device *dev, const uint32_t *src)
{
	if (dev == NULL) {
		return -EINVAL;
	}

	return mbox0_data_set_all(dev, src);
}

int mchp_xec_espi_pc_mailbox_cmd(const struct device *dev, enum mchp_xec_espi_pc_mbox_cmd_id cmd_id,
				 uint8_t cmd)
{
	mm_reg_t mregbase = xec_mbox0_cfg.regbase;
	uint32_t ofs = 0;

	if ((dev == NULL) || (cmd_id >= MCHP_XEC_ESPI_PC_MBOX_CMD_ID_MAX)) {
		return -EINVAL;
	}

	switch (cmd_id) {
	case MCHP_XEC_ESPI_PC_MBOX_CMD_ID_HOST_TO_EC:
		ofs = XEC_MBOX_H2EC_OFS;
		break;
	case MCHP_XEC_ESPI_PC_MBOX_CMD_ID_EC_TO_HOST:
		ofs = XEC_MBOX_EC2H_OFS;
		break;
	case MCHP_XEC_ESPI_PC_MBOX_CMD_ID_SMI_SRC:
		ofs = XEC_MBOX_SMI_SRC_OFS;
		break;
	case MCHP_XEC_ESPI_PC_MBOX_CMD_ID_SMI_MASK:
		ofs = XEC_MBOX_SMI_MSK_OFS;
		break;
	default:
		return -EINVAL;
	}

	sys_write8(cmd, mregbase + ofs);

	return 0;
}

/** @brief Write command to specified mailbox box command register
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param id enum mchp_xec_espi_pc_mbox_isrc specifying the interrupt (internal EC or serial IRQ)
 * @param enable a zero value is disable else enable
 *
 * @retval 0 success
 * @retval -EINVAL if dev is NULL or id is invalid
 */
int mchp_xec_espi_pc_mailbox_ictrl(const struct device *dev, enum mchp_xec_espi_pc_mbox_isrc id,
				   uint8_t enable)
{
	const struct espi_xec_config *espi_drv_cfg = dev->config;
	struct xec_espi_ioc_cfg_regs *regs =
		(struct xec_espi_ioc_cfg_regs *)(espi_drv_cfg->ioc_base_addr +
						 MCHP_ESPI_IO_CFG_OFS);
	uint8_t mbox_sirq_slot_val = MCHP_ESPI_IO_SIRQ_DIS;

	if (dev == NULL) {
		return -EINVAL;
	}

	switch (id) {
	case MCHP_XEC_ESPI_PC_MBOX_ISRC_EC:
		xec_ecia_info_girq_ctrl(xec_mbox0_cfg.ecia_info, enable);
		break;
	case MCHP_XEC_ESPI_PC_MBOX_ISRC_SIRQ:
		if (enable != 0) {
			mbox_sirq_slot_val = (uint8_t)(XEC_MBOX_SIRQ_EV_SLOT_VAL);
		}
		regs->SIRQ[MCHP_ESPI_SIRQ_MBOX_SIRQ] = mbox_sirq_slot_val;
		break;
	case MCHP_XEC_ESPI_PC_MBOX_ISRC_SIRQ_SMI:
		if (enable != 0) {
			mbox_sirq_slot_val = (uint8_t)(XEC_MBOX_SIRQ_SMI_SLOT_VAL);
		}
		regs->SIRQ[MCHP_ESPI_SIRQ_MBOX_SMI] = mbox_sirq_slot_val;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

#undef CONNECT_IRQ_MBOX0
#define CONNECT_IRQ_MBOX0 connect_irq_mbox0
#undef INIT_MBOX0
#define INIT_MBOX0 init_mbox0

#endif /* CONFIG_ESPI_PERIPHERAL_MAILBOX */

#ifdef CONFIG_ESPI_PERIPHERAL_8042_KBC

BUILD_ASSERT(DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(kbc0)), "XEC kbc0 DT node is disabled!");

/* Legacy personal computer default interrupt for 8042 keyboard controller and mouse */
#define XEC_KBC_KB_SIRQ_SLOT    1U
#define XEC_KBC_MOUSE_SIRQ_SLOT 12U

struct xec_kbc0_config {
	uintptr_t regbase;
	uintptr_t regbase_p92;
	uint32_t ibf_ecia_info;
	uint32_t obe_ecia_info;
};

static const struct xec_kbc0_config xec_kbc0_cfg = {
	.regbase = DT_REG_ADDR(DT_NODELABEL(kbc0)),
	.regbase_p92 = DT_REG_ADDR(DT_NODELABEL(port92)),
	.ibf_ecia_info = DT_PROP_BY_IDX(DT_NODELABEL(kbc0), girqs, 1),
	.obe_ecia_info = DT_PROP_BY_IDX(DT_NODELABEL(kbc0), girqs, 0),
};

static void kbc0_ibf_isr(const struct device *dev)
{
	struct espi_xec_data *const data = dev->data;
	struct kbc_regs *kbc_hw = (struct kbc_regs *)xec_kbc0_cfg.regbase;

#ifdef CONFIG_ESPI_PERIPHERAL_KBC_IBF_EVT_DATA
	/* Chrome solution */
	struct espi_event evt = {
		ESPI_BUS_PERIPHERAL_NOTIFICATION,
		ESPI_PERIPHERAL_8042_KBC,
		ESPI_PERIPHERAL_NODATA,
	};
	struct espi_evt_data_kbc *kbc_evt = (struct espi_evt_data_kbc *)&evt.evt_data;
	/*
	 * Indicates if the host sent a command or data.
	 * 0 = data
	 * 1 = Command.
	 */
	kbc_evt->type = kbc_hw->EC_KBC_STS & MCHP_KBC_STS_CD ? 1 : 0;
	/* The data in KBC Input Buffer */
	kbc_evt->data = kbc_hw->EC_DATA;
	/* KBC Input Buffer Full event */
	kbc_evt->evt = HOST_KBC_EVT_IBF;
#else
	/* Windows solution */
	/* The high byte contains information from the host,
	 * and the lower byte speficies if the host sent
	 * a command or data. 1 = Command.
	 */
	uint32_t isr_data = ((kbc_hw->EC_KBC_STS & MCHP_KBC_STS_CD) << E8042_ISR_CMD_DATA_POS);
	isr_data |= ((kbc_hw->EC_DATA & 0xFF) << E8042_ISR_DATA_POS);

	struct espi_event evt = {.evt_type = ESPI_BUS_PERIPHERAL_NOTIFICATION,
				 .evt_details = ESPI_PERIPHERAL_8042_KBC,
				 .evt_data = isr_data};
#endif
	xec_ecia_info_girq_src_clear(xec_kbc0_cfg.ibf_ecia_info);

	espi_send_callbacks(&data->callbacks, dev, evt);
}

static void kbc0_obe_isr(const struct device *dev)
{
#ifdef CONFIG_ESPI_PERIPHERAL_KBC_OBE_CBK
	/* Chrome solution */
	struct espi_xec_data *const data = dev->data;

	struct espi_event evt = {
		ESPI_BUS_PERIPHERAL_NOTIFICATION,
		ESPI_PERIPHERAL_8042_KBC,
		ESPI_PERIPHERAL_NODATA,
	};
	struct espi_evt_data_kbc *kbc_evt = (struct espi_evt_data_kbc *)&evt.evt_data;

	/* Disable KBC OBE interrupt first */
	xec_ecia_info_girq_ctrl(xec_kbc0_cfg.obe_ecia_info, MCHP_MEC_ECIA_GIRQ_DIS);

	/*
	 * Notify application that host already read out data. The application
	 * might need to clear status register via espi_api_lpc_write_request()
	 * with E8042_CLEAR_FLAG opcode in callback.
	 */
	kbc_evt->evt = HOST_KBC_EVT_OBE;
	kbc_evt->data = 0;
	kbc_evt->type = 0;

	espi_send_callbacks(&data->callbacks, dev, evt);
#else
	/* Windows solution
	 * disable and clear GIRQ interrupt and status
	 */
	xec_ecia_info_girq_ctrl(xec_kbc0_cfg.obe_ecia_info, MCHP_MEC_ECIA_GIRQ_DIS);
#endif
	xec_ecia_info_girq_src_clear(xec_kbc0_cfg.obe_ecia_info);
}

/* dev is a pointer to espi0 device */
static int kbc0_rd_req(const struct device *dev, enum lpc_peripheral_opcode op, uint32_t *data)
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
static int kbc0_wr_req(const struct device *dev, enum lpc_peripheral_opcode op, uint32_t *data)
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
			xec_ecia_info_girq_src_clear(xec_kbc0_cfg.ibf_ecia_info);
			xec_ecia_info_girq_ctrl(xec_kbc0_cfg.ibf_ecia_info, MCHP_MEC_ECIA_GIRQ_EN);
			break;
		case E8042_PAUSE_IRQ:
			xec_ecia_info_girq_ctrl(xec_kbc0_cfg.ibf_ecia_info, MCHP_MEC_ECIA_GIRQ_DIS);
			break;
		case E8042_CLEAR_OBF:
			dummy = kbc_hw->HOST_AUX_DATA;
			break;
		case E8042_SET_FLAG:
			/* FW shouldn't modify these flags directly */
			*data &= ~(MCHP_KBC_STS_OBF | MCHP_KBC_STS_IBF | MCHP_KBC_STS_AUXOBF);
			kbc_hw->EC_KBC_STS |= *data;
			break;
		case E8042_CLEAR_FLAG:
			/* FW shouldn't modify these flags directly */
			*data |= (MCHP_KBC_STS_OBF | MCHP_KBC_STS_IBF | MCHP_KBC_STS_AUXOBF);
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
	xec_ecia_info_girq_src_clear(xec_kbc0_cfg.ibf_ecia_info);
	xec_ecia_info_girq_src_clear(xec_kbc0_cfg.obe_ecia_info);

	IRQ_CONNECT(DT_IRQ_BY_NAME(DT_NODELABEL(kbc0), ibf, irq),
		    DT_IRQ_BY_NAME(DT_NODELABEL(kbc0), ibf, priority), kbc0_ibf_isr,
		    DEVICE_DT_GET(DT_NODELABEL(espi0)), 0);
	irq_enable(DT_IRQ_BY_NAME(DT_NODELABEL(kbc0), ibf, irq));

	IRQ_CONNECT(DT_IRQ_BY_NAME(DT_NODELABEL(kbc0), obe, irq),
		    DT_IRQ_BY_NAME(DT_NODELABEL(kbc0), obe, priority), kbc0_obe_isr,
		    DEVICE_DT_GET(DT_NODELABEL(espi0)), 0);
	irq_enable(DT_IRQ_BY_NAME(DT_NODELABEL(kbc0), obe, irq));

	/* enable IBF GIRQ only. Application send command or turn on OBE
	 * after it writes to EC-to-Host data register.
	 */
	xec_ecia_info_girq_ctrl(xec_kbc0_cfg.ibf_ecia_info, MCHP_MEC_ECIA_GIRQ_EN);

	return 0;
}

static int init_kbc0(const struct device *dev)
{
	const struct espi_xec_config *devcfg = dev->config;
	struct xec_espi_ioc_cfg_regs *regs =
		(struct xec_espi_ioc_cfg_regs *)(devcfg->ioc_base_addr + MCHP_ESPI_IO_CFG_OFS);
	struct kbc_regs *kbc_hw = (struct kbc_regs *)xec_kbc0_cfg.regbase;
#ifdef CONFIG_ESPI_PERIPHERAL_8042_FAST_KBC_PORT92
	struct port92_regs *p92_hw = (struct port92_regs *)xec_kbc0_cfg.regbase_p92;
#endif
	uint32_t iobar_val = MCHP_ESPI_IO_BAR_HOST_ADDR_SET(ESPI_XEC_KBC_HOST_ADDR);

	kbc_hw->KBC_CTRL |= MCHP_KBC_CTRL_AUXH;
	kbc_hw->KBC_CTRL |= MCHP_KBC_CTRL_OBFEN;
	/* This is the activate register, but the HAL has a funny name */
	kbc_hw->ACTV = MCHP_KBC_ACTV_EN;

	regs->IOHBAR[IOB_KBC] = iobar_val | MCHP_ESPI_IO_BAR_HOST_VALID;

#ifdef CONFIG_ESPI_PERIPHERAL_8042_FAST_KBC_PORT92
	iobar_val = MCHP_ESPI_IO_BAR_HOST_ADDR_SET(ESPI_XEC_FKBC_HOST_ADDR);
	p29_hw->ACTV = MCHP_PORT92_ACTV_ENABLE;
	regs->IOHBAR[IOB_PORT92] = iobar_val | MCHP_ESPI_IO_BAR_HOST_VALID;
#endif

	/* Serial IRQ */
	regs->SIRQ[SIRQ_KBC_KIRQ] = XEC_KBC_KB_SIRQ_SLOT;
	regs->SIRQ[SIRQ_KBC_MIRQ] = XEC_KBC_MOUSE_SIRQ_SLOT;

	return 0;
}

#undef CONNECT_IRQ_KBC0
#define CONNECT_IRQ_KBC0 connect_irq_kbc0
#undef INIT_KBC0
#define INIT_KBC0 init_kbc0

#endif /* CONFIG_ESPI_PERIPHERAL_8042_KBC */

#ifdef CONFIG_ESPI_PERIPHERAL_HOST_IO

#define XEC_AEC0_NODE DT_NODELABEL(acpi_ec0)

static const struct xec_acpi_ec_config xec_acpi_ec0_cfg = {
	.regbase = DT_REG_ADDR(XEC_AEC0_NODE),
	.ibf_ecia_info = DT_PROP_BY_IDX(XEC_AEC0_NODE, girqs, 0),
	.obe_ecia_info = DT_PROP_BY_IDX(XEC_AEC0_NODE, girqs, 1),
	.host_mem_addr = DT_PROP_OR(XEC_AEC0_NODE, host_mem, UINT32_MAX),
	.host_io_addr = DT_PROP_OR(XEC_AEC0_NODE, host_io, ESPI_XEC_ACPI_EC0_HOST_ADDR),
	.obf_sirq_slot_val = MCHP_ESPI_IO_SIRQ_DIS,
};

static void acpi_ec0_ibf_isr(const struct device *dev)
{
	struct espi_xec_data *const data = dev->data;
	struct espi_event evt = {ESPI_BUS_PERIPHERAL_NOTIFICATION, ESPI_PERIPHERAL_HOST_IO,
				 ESPI_PERIPHERAL_NODATA};

#ifdef CONFIG_ESPI_PERIPHERAL_ACPI_EC_IBF_EVT_DATA
	struct acpi_ec_regs *acpi_ec0_hw = (struct acpi_ec_regs *)xec_acpi_ec0_cfg.regbase;

	/* Updates to fit Chrome shim layer design */
	struct espi_evt_data_acpi *acpi_evt = (struct espi_evt_data_acpi *)&evt.evt_data;

	/* Host put data on input buffer of ACPI EC0 channel */
	if (acpi_ec0_hw->EC_STS & MCHP_ACPI_EC_STS_IBF) {
		/* Set processing flag before reading command byte */
		acpi_ec0_hw->EC_STS |= MCHP_ACPI_EC_STS_UD1A;
		/*
		 * Indicates if the host sent a command or data.
		 * 0 = data
		 * 1 = Command.
		 */
		acpi_evt->type = acpi_ec0_hw->EC_STS & MCHP_ACPI_EC_STS_CMD ? 1 : 0;
		acpi_evt->data = acpi_ec0_hw->OS2EC_DATA;
	}
#endif /* CONFIG_ESPI_PERIPHERAL_ACPI_EC_IBF_EVT_DATA */

	espi_send_callbacks(&data->callbacks, dev, evt);

	/* clear GIRQ status */
	xec_ecia_info_girq_src_clear(xec_acpi_ec0_cfg.ibf_ecia_info);
}

static void acpi_ec0_obe_isr(const struct device *dev)
{
	struct espi_xec_data *const data = dev->data;
	struct espi_event evt = {
		ESPI_BUS_PERIPHERAL_NOTIFICATION,
		ESPI_PERIPHERAL_HOST_IO,
		ESPI_PERIPHERAL_NODATA,
	};
	struct espi_evt_data_acpi *acpi_evt = (struct espi_evt_data_acpi *)&evt.evt_data;

	/* ISSUE: 0=Host wrote data, 1=Host wrote cmd, 2=Host read EC-to-Host */
	acpi_evt->type = 2u;
	acpi_evt->data = 0;

	/* disable and clear GIRQ status */
	xec_ecia_info_girq_ctrl(xec_acpi_ec0_cfg.obe_ecia_info, MCHP_MEC_ECIA_GIRQ_DIS);
	xec_ecia_info_girq_src_clear(xec_acpi_ec0_cfg.obe_ecia_info);

	espi_send_callbacks(&data->callbacks, dev, evt);
}

static int eacpi_rd_req(const struct device *dev, enum lpc_peripheral_opcode op, uint32_t *data)
{
	struct acpi_ec_regs *acpi_ec0_hw = (struct acpi_ec_regs *)xec_acpi_ec0_cfg.regbase;

	ARG_UNUSED(dev);

	switch (op) {
	case EACPI_OBF_HAS_CHAR:
		/* EC has written data back to host. OBF is
		 * automatically cleared after host reads
		 * the data
		 */
		*data = acpi_ec0_hw->EC_STS & MCHP_ACPI_EC_STS_OBF ? 1 : 0;
		break;
	case EACPI_IBF_HAS_CHAR:
		*data = acpi_ec0_hw->EC_STS & MCHP_ACPI_EC_STS_IBF ? 1 : 0;
		break;
	case EACPI_READ_STS:
		*data = acpi_ec0_hw->EC_STS;
		break;
#if defined(CONFIG_ESPI_PERIPHERAL_ACPI_SHM_REGION)
	case EACPI_GET_SHARED_MEMORY:
		*data = (uint32_t)ec_host_cmd_sram + CONFIG_ESPI_XEC_PERIPHERAL_HOST_CMD_PARAM_SIZE;
		break;
#endif /* CONFIG_ESPI_PERIPHERAL_ACPI_SHM_REGION */
	default:
		return -EINVAL;
	}

	return 0;
}

static int eacpi_wr_req(const struct device *dev, enum lpc_peripheral_opcode op, uint32_t *data)
{
	struct acpi_ec_regs *acpi_ec0_hw = (struct acpi_ec_regs *)xec_acpi_ec0_cfg.regbase;

	ARG_UNUSED(dev);

	switch (op) {
	case EACPI_WRITE_CHAR:
		acpi_ec0_hw->EC2OS_DATA = (*data & 0xff);
		break;
	case EACPI_WRITE_STS:
		acpi_ec0_hw->EC_STS = (*data & 0xff);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int connect_irq_acpi_ec0(const struct device *dev)
{
	xec_ecia_info_girq_src_clear(xec_acpi_ec0_cfg.ibf_ecia_info);
	xec_ecia_info_girq_src_clear(xec_acpi_ec0_cfg.obe_ecia_info);

	IRQ_CONNECT(DT_IRQ_BY_NAME(DT_NODELABEL(acpi_ec0), ibf, irq),
		    DT_IRQ_BY_NAME(DT_NODELABEL(acpi_ec0), ibf, priority), acpi_ec0_ibf_isr,
		    DEVICE_DT_GET(DT_NODELABEL(espi0)), 0);
	irq_enable(DT_IRQ_BY_NAME(DT_NODELABEL(acpi_ec0), ibf, irq));

	IRQ_CONNECT(DT_IRQ_BY_NAME(DT_NODELABEL(acpi_ec0), obe, irq),
		    DT_IRQ_BY_NAME(DT_NODELABEL(acpi_ec0), obe, priority), acpi_ec0_obe_isr,
		    DEVICE_DT_GET(DT_NODELABEL(espi0)), 0);
	irq_enable(DT_IRQ_BY_NAME(DT_NODELABEL(acpi_ec0), obe, irq));

	/* Only enable IBF. OBE will trigger immediately. App requests when this is enabled */
	xec_ecia_info_girq_ctrl(xec_acpi_ec0_cfg.ibf_ecia_info, MCHP_MEC_ECIA_GIRQ_EN);

	return 0;
}

static int init_acpi_ec0(const struct device *dev)
{
	const struct espi_xec_config *devcfg = dev->config;
	struct xec_espi_ioc_cfg_regs *regs =
		(struct xec_espi_ioc_cfg_regs *)(devcfg->ioc_base_addr + MCHP_ESPI_IO_CFG_OFS);
	uint32_t iobar_val = MCHP_ESPI_IO_BAR_HOST_ADDR_SET(xec_acpi_ec0_cfg.host_io_addr);

	regs->IOHBAR[IOB_ACPI_EC0] = iobar_val | MCHP_ESPI_IO_BAR_HOST_VALID;

	/* Serial IRQ */
	regs->SIRQ[SIRQ_ACPI_EC0_OBF] = xec_acpi_ec0_cfg.obf_sirq_slot_val;

	return 0;
}

#undef CONNECT_IRQ_ACPI_EC0
#define CONNECT_IRQ_ACPI_EC0 connect_irq_acpi_ec0
#undef INIT_ACPI_EC0
#define INIT_ACPI_EC0 init_acpi_ec0

#endif /* CONFIG_ESPI_PERIPHERAL_HOST_IO */

#if defined(CONFIG_ESPI_PERIPHERAL_EC_HOST_CMD) || defined(CONFIG_ESPI_PERIPHERAL_HOST_IO_PVT)

#define XEC_AEC1_NODE DT_NODELABEL(acpi_ec1)

static const struct xec_acpi_ec_config xec_acpi_ec1_cfg = {
	.regbase = DT_REG_ADDR(XEC_AEC1_NODE),
	.ibf_ecia_info = DT_PROP_BY_IDX(XEC_AEC1_NODE, girqs, 0),
	.obe_ecia_info = DT_PROP_BY_IDX(XEC_AEC1_NODE, girqs, 1),
	.host_mem_addr = DT_PROP_OR(XEC_AEC1_NODE, host_mem, UINT32_MAX),
	.host_io_addr = DT_PROP_OR(XEC_AEC1_NODE, host_io, UINT16_MAX),
	.obf_sirq_slot_val = MCHP_ESPI_IO_SIRQ_DIS,
};

static void acpi_ec1_ibf_isr(const struct device *dev)
{
	struct espi_xec_data *const data = dev->data;
	struct espi_event evt = {.evt_type = ESPI_BUS_PERIPHERAL_NOTIFICATION,
#ifdef CONFIG_ESPI_PERIPHERAL_EC_HOST_CMD
				 .evt_details = ESPI_PERIPHERAL_EC_HOST_CMD,
#else
				 .evt_details = ESPI_PERIPHERAL_HOST_IO_PVT,
#endif
				 .evt_data = ESPI_PERIPHERAL_NODATA};
#ifdef CONFIG_ESPI_PERIPHERAL_ACPI_EC_IBF_EVT_DATA
	struct acpi_ec_regs *acpi_ec1_hw = (struct acpi_ec_regs *)xec_acpi_ec1_cfg.regbase;

	/* Updates to fit Chrome shim layer design.
	 * Host put data on input buffer of ACPI EC1 channel.
	 */
	if (acpi_ec1_hw->EC_STS & MCHP_ACPI_EC_STS_IBF) {
		/* Set processing flag before reading command byte */
		acpi_ec1_hw->EC_STS |= MCHP_ACPI_EC_STS_UD1A;
		/* Read out input data and clear IBF pending bit */
		evt.evt_data = acpi_ec1_hw->OS2EC_DATA;
	}

	/* clear GIRQ status */
	xec_ecia_info_girq_src_clear(xec_acpi_ec1_cfg.ibf_ecia_info);

	espi_send_callbacks(&data->callbacks, dev, evt);
#else
	espi_send_callbacks(&data->callbacks, dev, evt);

	/* clear GIRQ status */
	xec_ecia_info_girq_src_clear(xec_acpi_ec1_cfg.ibf_ecia_info);
#endif /* CONFIG_ESPI_PERIPHERAL_ACPI_EC_IBF_EVT_DATA */
}

static void acpi_ec1_obe_isr(const struct device *dev)
{
	struct espi_xec_data *const data = dev->data;
	struct espi_event evt = {
		ESPI_BUS_PERIPHERAL_NOTIFICATION,
		ESPI_PERIPHERAL_HOST_IO,
		ESPI_PERIPHERAL_NODATA,
	};
	struct espi_evt_data_acpi *acpi_evt = (struct espi_evt_data_acpi *)&evt.evt_data;

	acpi_evt->type = ESPI_EVENT_DATA_ACPI_TYPE_HOST_RD_EC_TO_HOST;
	acpi_evt->data = 0;

	/* disable and clear GIRQ status */
	xec_ecia_info_girq_ctrl(xec_acpi_ec1_cfg.obe_ecia_info, 0);
	xec_ecia_info_girq_src_clear(xec_acpi_ec1_cfg.obe_ecia_info);

	espi_send_callbacks(&data->callbacks, dev, evt);
}

static int connect_irq_acpi_ec1(const struct device *dev)
{
	xec_ecia_info_girq_src_clear(xec_acpi_ec1_cfg.ibf_ecia_info);
	xec_ecia_info_girq_src_clear(xec_acpi_ec1_cfg.obe_ecia_info);

	IRQ_CONNECT(DT_IRQ_BY_NAME(DT_NODELABEL(acpi_ec1), ibf, irq),
		    DT_IRQ_BY_NAME(DT_NODELABEL(acpi_ec1), ibf, priority), acpi_ec1_ibf_isr,
		    DEVICE_DT_GET(DT_NODELABEL(espi0)), 0);
	irq_enable(DT_IRQ_BY_NAME(DT_NODELABEL(acpi_ec1), ibf, irq));

	IRQ_CONNECT(DT_IRQ_BY_NAME(DT_NODELABEL(acpi_ec1), obe, irq),
		    DT_IRQ_BY_NAME(DT_NODELABEL(acpi_ec1), obe, priority), acpi_ec1_obe_isr,
		    DEVICE_DT_GET(DT_NODELABEL(espi0)), 0);
	irq_enable(DT_IRQ_BY_NAME(DT_NODELABEL(acpi_ec1), obe, irq));

	xec_ecia_info_girq_ctrl(xec_acpi_ec1_cfg.ibf_ecia_info, MCHP_MEC_ECIA_GIRQ_EN);

	return 0;
}

static int init_acpi_ec1(const struct device *dev)
{
	const struct espi_xec_config *devcfg = dev->config;
	struct xec_espi_ioc_cfg_regs *regs =
		(struct xec_espi_ioc_cfg_regs *)(devcfg->ioc_base_addr + MCHP_ESPI_IO_CFG_OFS);
	uint32_t bar_val = 0;

#ifdef CONFIG_ESPI_PERIPHERAL_EC_HOST_CMD
	bar_val = MCHP_ESPI_IO_BAR_HOST_ADDR_SET(CONFIG_ESPI_PERIPHERAL_HOST_CMD_DATA_PORT_NUM);
#elif CONFIG_ESPI_PERIPHERAL_HOST_IO_PVT
	bar_val = MCHP_ESPI_IO_BAR_HOST_ADDR_SET(CONFIG_ESPI_PERIPHERAL_HOST_IO_PVT_PORT_NUM);
#else
	if (xec_acpi_ec1_cfg.host_mem_addr != UINT32_MAX) {
		xec_espi_mbar_host_set(devcfg->mc_base_addr, MEMB_ACPI_EC1,
				       xec_acpi_ec1_cfg.host_mem_addr, true);
	}

	if (xec_acpi_ec2_cfg.host_io_addr != UINT16_MAX) {
		bar_val = MCHP_ESPI_IO_BAR_HOST_ADDR_SET((uint32_t)xec_acpi_ec1_cfg.host_io_addr);
	}
#endif

	regs->IOHBAR[IOB_ACPI_EC1] = bar_val | MCHP_ESPI_IO_BAR_HOST_VALID;

	/* Serial IRQ */
	regs->SIRQ[SIRQ_ACPI_EC1_OBF] = xec_acpi_ec1_cfg.obf_sirq_slot_val;

	return 0;
}

#undef CONNECT_IRQ_ACPI_EC1
#define CONNECT_IRQ_ACPI_EC1 connect_irq_acpi_ec1
#undef INIT_ACPI_EC1
#define INIT_ACPI_EC1 init_acpi_ec1

#endif /* CONFIG_ESPI_PERIPHERAL_EC_HOST_CMD || CONFIG_ESPI_PERIPHERAL_HOST_IO_PVT */

#if defined(CONFIG_ESPI_PERIPHERAL_HOST_IO_PVT2) || defined(CONFIG_ESPI_PERIPHERAL_HOST_IO_PVT3)

static void acpi_ec_pvt_ibf_handler(const struct device *dev,
				    const struct xec_acpi_ec_config *acpi_ec_cfg,
				    enum espi_virtual_peripheral periph)
{
	struct espi_xec_data *const data = dev->data;
	struct acpi_ec_regs *regs = (struct acpi_ec_regs *)acpi_ec_cfg->regbase;
	struct espi_event evt = {.evt_type = ESPI_BUS_PERIPHERAL_NOTIFICATION,
				 .evt_details = 0,
				 .evt_data = ESPI_PERIPHERAL_NODATA};
	struct espi_evt_data_pvt *pvt_evt = (struct espi_evt_data_pvt *)&evt.evt_data;

	evt.evt_details = periph;
	if (regs->EC_STS & MCHP_ACPI_EC_STS_IBF) {
		/* Set processing flag before reading command byte */
		regs->EC_STS |= MCHP_ACPI_EC_STS_UD1A;

		pvt_evt->type = ESPI_EVENT_DATA_PVT_TYPE_HOST_TO_EC_DATA;
		if ((regs->EC_STS & MCHP_ACPI_EC_STS_CMD) != 0) {
			pvt_evt->type = ESPI_EVENT_DATA_PVT_TYPE_HOST_TO_EC_CMD;
		}

		/* Read of input data clears IBF pending bit */
		pvt_evt->data = regs->OS2EC_DATA;
	}

	/* clear GIRQ status */
	xec_ecia_info_girq_src_clear(acpi_ec_cfg->ibf_ecia_info);

	espi_send_callbacks(&data->callbacks, dev, evt);
}

static void acpi_ec_pvt_obe_handler(const struct device *dev,
				    const struct xec_acpi_ec_config *acpi_ec_cfg,
				    enum espi_virtual_peripheral periph)
{
	struct espi_xec_data *const data = dev->data;
	struct espi_event evt = {
		ESPI_BUS_PERIPHERAL_NOTIFICATION,
		0,
		ESPI_PERIPHERAL_NODATA,
	};
	struct espi_evt_data_pvt *pvt_evt = (struct espi_evt_data_pvt *)&evt.evt_data;

	evt.evt_details = periph;
	pvt_evt->type = ESPI_EVENT_DATA_PVT_TYPE_HOST_RD_EC_TO_HOST;
	pvt_evt->data = 0;

	/* disable and clear GIRQ status */
	xec_ecia_info_girq_ctrl(acpi_ec_cfg->obe_ecia_info, MCHP_MEC_ECIA_GIRQ_DIS);
	xec_ecia_info_girq_src_clear(acpi_ec_cfg->obe_ecia_info);

	espi_send_callbacks(&data->callbacks, dev, evt);
}

static int init_acpi_ec_bars(const struct device *dev, const struct xec_acpi_ec_config *acpi_ec_cfg,
			     uint8_t io_bar_idx, uint8_t mem_bar_idx, uint8_t sirq_idx)
{
	const struct espi_xec_config *devcfg = dev->config;
	struct xec_espi_ioc_cfg_regs *regs =
		(struct xec_espi_ioc_cfg_regs *)(devcfg->ioc_base_addr + MCHP_ESPI_IO_CFG_OFS);
	uint32_t iob_value = 0;

	if (acpi_ec_cfg->host_io_addr != UINT16_MAX) {
		iob_value = acpi_ec_cfg->host_io_addr;
		iob_value = MCHP_ESPI_IO_BAR_HOST_ADDR_SET(iob_value);
		iob_value |= MCHP_ESPI_IO_BAR_HOST_VALID;
	}

	if (io_bar_idx == IOB_ACPI_EC2) {
		if (IS_ENABLED(CONFIG_ESPI_PERIPHERAL_HOST_IO_PVT2)) {
			iob_value = CONFIG_ESPI_PERIPHERAL_HOST_IO_PVT2_PORT_NUM;
			iob_value = MCHP_ESPI_IO_BAR_HOST_ADDR_SET(iob_value);
			iob_value |= MCHP_ESPI_IO_BAR_HOST_VALID;
		}
	} else if (io_bar_idx == IOB_ACPI_EC3) {
		if (IS_ENABLED(CONFIG_ESPI_PERIPHERAL_HOST_IO_PVT3)) {
			iob_value = CONFIG_ESPI_PERIPHERAL_HOST_IO_PVT3_PORT_NUM;
			iob_value = MCHP_ESPI_IO_BAR_HOST_ADDR_SET(iob_value);
			iob_value |= MCHP_ESPI_IO_BAR_HOST_VALID;
		}
	} else {
		return -EINVAL;
	}

	if (iob_value & MCHP_ESPI_IO_BAR_HOST_VALID) {
		regs->IOHBAR[io_bar_idx] = iob_value;
	}

	if (acpi_ec_cfg->host_mem_addr != UINT32_MAX) {
		xec_espi_mbar_host_set(devcfg->mc_base_addr, mem_bar_idx,
				       acpi_ec_cfg->host_mem_addr, true);
	}

	/* Serial IRQ */
	regs->SIRQ[sirq_idx] = acpi_ec_cfg->obf_sirq_slot_val;

	return 0;
}
#endif /* CONFIG_ESPI_PERIPHERAL_HOST_IO_PVT2 || CONFIG_ESPI_PERIPHERAL_HOST_IO_PVT3 */

#ifdef CONFIG_ESPI_PERIPHERAL_HOST_IO_PVT2

#define XEC_AEC2_NODE DT_NODELABEL(acpi_ec2)

static const struct xec_acpi_ec_config xec_acpi_ec2_cfg = {
	.regbase = DT_REG_ADDR(XEC_AEC2_NODE),
	.ibf_ecia_info = DT_PROP_BY_IDX(XEC_AEC2_NODE, girqs, 0),
	.obe_ecia_info = DT_PROP_BY_IDX(XEC_AEC2_NODE, girqs, 1),
	.host_mem_addr = DT_PROP_OR(XEC_AEC2_NODE, host_mem, UINT32_MAX),
	.host_io_addr = DT_PROP_OR(XEC_AEC2_NODE, host_io, UINT16_MAX),
	.obf_sirq_slot_val = MCHP_ESPI_IO_SIRQ_DIS,
};

static void acpi_ec2_ibf_isr(const struct device *dev)
{
	acpi_ec_pvt_ibf_handler(dev, &xec_acpi_ec2_cfg, ESPI_PERIPHERAL_HOST_IO_PVT2);
}

static void acpi_ec2_obe_isr(const struct device *dev)
{
	acpi_ec_pvt_obe_handler(dev, &xec_acpi_ec2_cfg, ESPI_PERIPHERAL_HOST_IO_PVT2);
}

static int connect_irq_acpi_ec2(const struct device *dev)
{
	xec_ecia_info_girq_src_clear(xec_acpi_ec2_cfg.ibf_ecia_info);
	xec_ecia_info_girq_src_clear(xec_acpi_ec2_cfg.obe_ecia_info);

	IRQ_CONNECT(DT_IRQ_BY_NAME(XEC_AEC2_NODE, ibf, irq),
		    DT_IRQ_BY_NAME(XEC_AEC2_NODE, ibf, priority), acpi_ec2_ibf_isr,
		    DEVICE_DT_GET(DT_NODELABEL(espi0)), 0);
	irq_enable(DT_IRQ_BY_NAME(XEC_AEC2_NODE, ibf, irq));

	IRQ_CONNECT(DT_IRQ_BY_NAME(XEC_AEC2_NODE, obe, irq),
		    DT_IRQ_BY_NAME(XEC_AEC2_NODE, obe, priority), acpi_ec2_obe_isr,
		    DEVICE_DT_GET(DT_NODELABEL(espi0)), 0);
	irq_enable(DT_IRQ_BY_NAME(XEC_AEC2_NODE, obe, irq));

	xec_ecia_info_girq_ctrl(xec_acpi_ec2_cfg.ibf_ecia_info, MCHP_MEC_ECIA_GIRQ_EN);

	return 0;
}

static int init_acpi_ec2(const struct device *dev)
{
	return init_acpi_ec_bars(dev, &xec_acpi_ec2_cfg, IOB_ACPI_EC2, MEMB_ACPI_EC2,
				 SIRQ_ACPI_EC2_OBF);
}

#undef CONNECT_IRQ_ACPI_EC2
#define CONNECT_IRQ_ACPI_EC2 connect_irq_acpi_ec2
#undef INIT_ACPI_EC2
#define INIT_ACPI_EC2 init_acpi_ec2

#endif /* CONFIG_ESPI_PERIPHERAL_HOST_IO_PVT2 */

#ifdef CONFIG_ESPI_PERIPHERAL_HOST_IO_PVT3

#define XEC_AEC3_NODE DT_NODELABEL(acpi_ec3)

static const struct xec_acpi_ec_config xec_acpi_ec3_cfg = {
	.regbase = DT_REG_ADDR(XEC_AEC3_NODE),
	.ibf_ecia_info = DT_PROP_BY_IDX(XEC_AEC3_NODE, girqs, 0),
	.obe_ecia_info = DT_PROP_BY_IDX(XEC_AEC3_NODE, girqs, 1),
	.host_mem_addr = DT_PROP_OR(XEC_AEC3_NODE, host_mem, UINT32_MAX),
	.host_io_addr = DT_PROP_OR(XEC_AEC3_NODE, host_io, UINT16_MAX),
	.obf_sirq_slot_val = MCHP_ESPI_IO_SIRQ_DIS,
};

static void acpi_ec3_ibf_isr(const struct device *dev)
{
	acpi_ec_pvt_ibf_handler(dev, &xec_acpi_ec3_cfg, ESPI_PERIPHERAL_HOST_IO_PVT3);
}

static void acpi_ec3_obe_isr(const struct device *dev)
{
	acpi_ec_pvt_obe_handler(dev, &xec_acpi_ec3_cfg, ESPI_PERIPHERAL_HOST_IO_PVT3);
}

static int connect_irq_acpi_ec3(const struct device *dev)
{
	xec_ecia_info_girq_src_clear(xec_acpi_ec3_cfg.ibf_ecia_info);
	xec_ecia_info_girq_src_clear(xec_acpi_ec3_cfg.obe_ecia_info);

	IRQ_CONNECT(DT_IRQ_BY_NAME(XEC_AEC3_NODE, ibf, irq),
		    DT_IRQ_BY_NAME(XEC_AEC3_NODE, ibf, priority), acpi_ec3_ibf_isr,
		    DEVICE_DT_GET(DT_NODELABEL(espi0)), 0);
	irq_enable(DT_IRQ_BY_NAME(XEC_AEC3_NODE, ibf, irq));

	IRQ_CONNECT(DT_IRQ_BY_NAME(XEC_AEC3_NODE, obe, irq),
		    DT_IRQ_BY_NAME(XEC_AEC3_NODE, obe, priority), acpi_ec3_obe_isr,
		    DEVICE_DT_GET(DT_NODELABEL(espi0)), 0);
	irq_enable(DT_IRQ_BY_NAME(XEC_AEC3_NODE, obe, irq));

	xec_ecia_info_girq_ctrl(xec_acpi_ec3_cfg.ibf_ecia_info, MCHP_MEC_ECIA_GIRQ_EN);

	return 0;
}

static int init_acpi_ec3(const struct device *dev)
{
	return init_acpi_ec_bars(dev, &xec_acpi_ec3_cfg, IOB_ACPI_EC3, MEMB_ACPI_EC3,
				 SIRQ_ACPI_EC3_OBF);
}

#undef CONNECT_IRQ_ACPI_EC3
#define CONNECT_IRQ_ACPI_EC3 connect_irq_acpi_ec3
#undef INIT_ACPI_EC3
#define INIT_ACPI_EC3 init_acpi_ec3

#endif /* CONFIG_ESPI_PERIPHERAL_HOST_IO_PVT3 */

#ifdef CONFIG_ESPI_PERIPHERAL_EC_HOST_CMD

BUILD_ASSERT(DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(emi0)), "XEC EMI0 DT node is disabled!");

/* EC only access */
#define XEC_EMI_H2EC_MBOX_OFS 0x100U
#define XEC_EMI_EC2H_MBOX_OFS 0x101U
#define XEC_EMI_MBASE0_OFS    0x104U
#define XEC_EMI_RWLIM0_OFS    0x108U
#define XEC_EMI_MBASE1_OFS    0x10CU
#define XEC_EMI_RWLIM1_OFS    0x110U
#define XEC_EMI_INTR_SET_OFS  0x114U /* r/ws */
#define XEC_EMI_HCLR_EN_OFS   0x116U
/* 8 32-bit registers. n is 0 through 7 */
#define XEC_EMI_APP_ID_OFS(n) 0x120U

#define XEC_EMI_MBASE_MSK           GENMASK(31, 2)
#define XEC_EMI_RWLIM_RD_MSK        GENMASK(14, 2)
#define XEC_EMI_RWLIM_WR_MSK        GENMASK(30, 18)
#define XEC_EMI_RWLIM_RD_SET(rdlim) ((uint32_t)(rdlim) & XEC_EMI_RWLIM_RD_MSK)
#define XEC_EMI_RWLIM_RD_GET(r)     ((uint32_t)(r) & XEC_EMI_RWLIM_RD_MSK)
#define XEC_EMI_RWLIM_WR_SET(wrlim) (((uint32_t)(wrlim) << 16) & XEC_EMI_RWLIM_WR_MSK)
#define XEC_EMI_RWLIM_WR_GET(r)     (((uint32_t)(r) & XEC_EMI_RWLIM_WR_MSK) >> 16)

struct xec_emi_config {
	uintptr_t regbase;
	uint32_t ecia_info;
	uint8_t sirq_slot_hev;
	uint8_t sirq_slot_e2h;
};

static const struct xec_emi_config xec_emi0_cfg = {
	.regbase = DT_REG_ADDR(DT_NODELABEL(emi0)),
	.ecia_info = DT_PROP_BY_IDX(DT_NODELABEL(emi0), girqs, 0),
	.sirq_slot_hev = MCHP_ESPI_IO_SIRQ_DIS,
	.sirq_slot_e2h = MCHP_ESPI_IO_SIRQ_DIS,
};

static int init_emi0(const struct device *dev)
{
	const struct espi_xec_config *cfg = dev->config;
	struct xec_espi_ioc_cfg_regs *regs =
		(struct xec_espi_ioc_cfg_regs *)(cfg->ioc_base_addr + MCHP_ESPI_IO_CFG_OFS);
	mm_reg_t emib = xec_emi0_cfg.regbase;
	uint32_t temp = 0, val = 0;

	sys_write8(0, emib + XEC_EMI_H2EC_MBOX_OFS); /* clear mailbox */

	xec_ecia_info_girq_ctrl(xec_emi0_cfg.ecia_info, 0);
	xec_ecia_info_girq_src_clear(xec_emi0_cfg.ecia_info);

	sys_write32((uint32_t)ec_host_cmd_sram, emib + XEC_EMI_MBASE0_OFS);

#ifdef CONFIG_ESPI_PERIPHERAL_ACPI_SHM_REGION
	temp = (uint32_t)(CONFIG_ESPI_XEC_PERIPHERAL_HOST_CMD_PARAM_SIZE +
			  CONFIG_ESPI_XEC_PERIPHERAL_ACPI_SHD_MEM_SIZE);
#else
	temp = (uint32_t)(CONFIG_ESPI_XEC_PERIPHERAL_HOST_CMD_PARAM_SIZE);
#endif
	val = XEC_EMI_RWLIM_RD_SET(temp);
	val |= XEC_EMI_RWLIM_WR_SET((uint32_t)(CONFIG_ESPI_XEC_PERIPHERAL_HOST_CMD_PARAM_SIZE));

	sys_write32(val, emib + XEC_EMI_RWLIM0_OFS);

	regs->IOHBAR[IOB_EMI0] = ((CONFIG_ESPI_PERIPHERAL_HOST_CMD_PARAM_PORT_NUM << 16) |
				  MCHP_ESPI_IO_BAR_HOST_VALID);

	/* Serial IRQ */
	regs->SIRQ[SIRQ_EMI0_HEV] = xec_emi0_cfg.sirq_slot_hev;
	regs->SIRQ[SIRQ_EMI0_E2H] = xec_emi0_cfg.sirq_slot_e2h;

	return 0;
}

#undef INIT_EMI0
#define INIT_EMI0 init_emi0

#endif /* CONFIG_ESPI_PERIPHERAL_EC_HOST_CMD */

#ifdef CONFIG_ESPI_PERIPHERAL_CUSTOM_OPCODE

static void host_cust_opcode_enable_interrupts(void);
static void host_cust_opcode_disable_interrupts(void);

static int ecust_rd_req(const struct device *dev, enum lpc_peripheral_opcode op, uint32_t *data)
{
	ARG_UNUSED(dev);

	switch (op) {
#ifdef CONFIG_ESPI_PERIPHERAL_EC_HOST_CMD
	case ECUSTOM_HOST_CMD_GET_PARAM_MEMORY:
		*data = (uint32_t)ec_host_cmd_sram;
		break;
	case ECUSTOM_HOST_CMD_GET_PARAM_MEMORY_SIZE:
		*data = CONFIG_ESPI_XEC_PERIPHERAL_HOST_CMD_PARAM_SIZE;
		break;
#endif
#ifdef CONFIG_ESPI_PERIPHERAL_MAILBOX
	/* data must point to a buffer of at least 32 bytes in size */
	case ECUSTOM_HOST_CMD_GET_MAILBOX_DATA:
		return mbox0_data_get_all(dev, data);
	case ECUSTOM_HOST_CMD_SET_MAILBOX_DATA:
		return mbox0_data_set_all(dev, (const uint32_t *)data);
#endif
	default:
		return -EINVAL;
	}

	return 0;
}

static int ecust_wr_req(const struct device *dev, enum lpc_peripheral_opcode op, uint32_t *data)
{
	struct acpi_ec_regs *acpi_ec1_hw = (struct acpi_ec_regs *)xec_acpi_ec1_cfg.regbase;

	ARG_UNUSED(dev);

	switch (op) {
	case ECUSTOM_HOST_SUBS_INTERRUPT_EN:
		if (*data != 0) {
			host_cust_opcode_enable_interrupts();
		} else {
			host_cust_opcode_disable_interrupts();
		}
		break;
	case ECUSTOM_HOST_CMD_SEND_RESULT:
		/*
		 * Write result to the data byte.  This sets the OBF
		 * status bit.
		 */
		acpi_ec1_hw->EC2OS_DATA = (*data & 0xff);
		/* Clear processing flag */
		acpi_ec1_hw->EC_STS &= ~MCHP_ACPI_EC_STS_UD1A;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

#endif /* CONFIG_ESPI_PERIPHERAL_CUSTOM_OPCODE */

#if defined(CONFIG_ESPI_PERIPHERAL_EC_HOST_CMD) && defined(CONFIG_ESPI_PERIPHERAL_ACPI_SHM_REGION)

static int eacpi_shm_rd_req(const struct device *dev, enum lpc_peripheral_opcode op, uint32_t *data)
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

static int eacpi_shm_wr_req(const struct device *dev, enum lpc_peripheral_opcode op, uint32_t *data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(op);
	ARG_UNUSED(data);

	return -EINVAL;
}

#endif /* CONFIG_ESPI_PERIPHERAL_ACPI_SHM_REGION */

#ifdef CONFIG_ESPI_PERIPHERAL_DEBUG_PORT_80

#define XEC_PBD_EC_DA_OFS           0x100u
#define XEC_PBD_EC_DA_VAL_POS       0
#define XEC_PBD_EC_DA_VAL_MSK       GENMASK(7, 0)
#define XEC_PBD_EC_DA_VAL_GET(r)    FIELD_GET(XEC_PBD_EC_DA_VAL_MSK, (r))
#define XEC_PBD_EC_DA_LANE_POS      8
#define XEC_PBD_EC_DA_LANE_MSK      GENMASK(9, 8)
#define XEC_PBD_EC_DA_LANE_0        0
#define XEC_PBD_EC_DA_LANE_1        1u
#define XEC_PBD_EC_DA_LANE_2        2u
#define XEC_PBD_EC_DA_LANE_3        3u
#define XEC_PBD_EC_DA_LANE_GET(r)   FIELD_GET(XEC_PBD_EC_DA_LANE_MSK, (r))
#define XEC_PBD_EC_DA_LEN_POS       10
#define XEC_PBD_EC_DA_LEN_MSK       GENMASK(11, 10)
#define XEC_PBD_EC_DA_LEN_1_OR_CONT 0
#define XEC_PBD_EC_DA_LEN_1_OF_2    1u
#define XEC_PBD_EC_DA_LEN_1_OF_4    2u
#define XEC_PBD_EC_DA_LEN_ORPHAN    3u
#define XEC_PBD_EC_DA_LEN_GET(r)    FIELD_GET(XEC_PBD_EC_DA_LEN_MSK, (r))
#define XEC_PBD_EC_DA_NE_POS        12
#define XEC_PBD_EC_DA_OVERRUN_POS   13
#define XEC_PBD_EC_DA_FTHR_POS      14

#define XEC_PBD_CFG_OFS         0x104u
#define XEC_PBD_SR_OFS          0x108u
#define XEC_PBD_SR_NE_POS       0
#define XEC_PBD_SR_OVERRUN_POS  1u
#define XEC_PBD_SR_FTHR_POS     2u
#define XEC_PBD_IER_OFS         0x109u
#define XEC_PBD_IER_FTHR_POS    0
#define XEC_PBD_SNAP_SHOT_OFS   0x10cu
#define XEC_PBD_LD_CFG_OFS      0x330u
#define XEC_PBD_LD_CFG_ACTV_POS 0

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
#ifdef CONFIG_ESPI_XEC_P80_MULTIBYTE
/* ev_data = 8, 16, or 32-bit capture I/O data
 * ev_details bits[15:0] = ESPI_PERIPHERAL_DEBUG_PORT80
 * ev_details bits[19:16] = byte lane bit map.
 * ev_details bits[23:20] = size of I/O write: 1, 2, or 4 bytes
 */
static void p80bd0_isr(const struct device *dev)
{
	struct espi_xec_data *const data = dev->data;
	mm_reg_t p80rb = xec_p80bd0_cfg.regbase;
	struct espi_event evt = {ESPI_BUS_PERIPHERAL_NOTIFICATION, 0, ESPI_PERIPHERAL_NODATA};
	int count = MCHP_P80_FIFO_READ_COUNT; /* limit ISR to 8 bytes */
	uint32_t dattr = 0, temp = 0;
	uint8_t ioflags = 0, iosize = 0, iowidth = 0, blane = 0;
	uint8_t iodata[4] = {0};

	dattr = sys_read32(p80rb + XEC_PBD_EC_DA_OFS);

	/* b[7:0]=8-bit value written, b[15:8]=attributes */
	while (((dattr & BIT(XEC_PBD_EC_DA_NE_POS)) != 0) && (count != 0)) { /* Not empty? */
		evt.evt_data = 0;

		iosize = XEC_PBD_EC_DA_LEN_GET(dattr);
		blane = XEC_PBD_EC_DA_LANE_GET(dattr);

		if (iosize == XEC_PBD_EC_DA_LEN_1_OR_CONT) {
			iodata[blane] = dattr & 0xffU;
			ioflags |= BIT(blane);
			if (iowidth == 0) {
				iowidth = 1U;
				break;
			} else if (iowidth == 2U) {
				break;
			} else if ((iowidth == 4U) && (blane == 3U)) {
				break;
			}
		} else if (iosize == XEC_PBD_EC_DA_LEN_1_OF_2) {
			iowidth = 2U; /* first byte of 16-bit Host I/O write */
			iodata[blane] = dattr & 0xffU;
			ioflags |= BIT(blane);
		} else if (iosize == XEC_PBD_EC_DA_LEN_1_OF_4) {
			iowidth = 4U; /* first byte of 32-bit Host I/O write */
			iodata[blane] = dattr & 0xffU;
			ioflags |= BIT(blane);
		} else { /* invalid and discard */
			ioflags &= (uint8_t)~BIT(blane);
		}

		dattr = sys_read32(p80rb + XEC_PBD_EC_DA_OFS);

		count--;
	}

	for (uint8_t n = 0; n < 4U; n++) {
		temp <<= 8;
		temp |= iodata[3U - n];
	}

	evt.evt_details = ESPI_PERIPHERAL_DEBUG_PORT80 | ((uint32_t)iowidth << 16);
	evt.evt_details |= ((uint32_t)(ioflags & 0xfU) << 16);
	evt.evt_details |= ((uint32_t)(iowidth & 0xfU) << 20);
	evt.evt_data = temp;
	espi_send_callbacks(&data->callbacks, dev, evt);

	/* clear GIRQ status */
	xec_ecia_info_girq_src_clear(xec_p80bd0_cfg.ecia_info);
}
#else
static void p80bd0_isr(const struct device *dev)
{
	struct espi_xec_data *const data = (struct espi_xec_data *)dev->data;
	mm_reg_t p80rb = xec_p80bd0_cfg.regbase;
	struct espi_event evt = {ESPI_BUS_PERIPHERAL_NOTIFICATION, 0, ESPI_PERIPHERAL_NODATA};
	int count = MCHP_P80_FIFO_READ_COUNT; /* limit ISR to 8 bytes */
	uint32_t dattr = sys_read32(p80rb + XEC_PBD_EC_DA_OFS);
	uint8_t byte_lane = 0;

	/* b[7:0]=8-bit value written, b[15:8]=attributes */
	while ((dattr & BIT(XEC_PBD_EC_DA_NE_POS)) && (count--)) { /* Not empty? */
		/* espi_event protocol No Data value is 0 so pick a bit and
		 * set it. This depends on the application.
		 */
		evt.evt_data = XEC_PBD_EC_DA_VAL_GET(dattr) | BIT(16);

		byte_lane = XEC_PBD_EC_DA_LANE_GET(dattr);

		switch (byte_lane) {
		case XEC_PBD_EC_DA_LANE_0:
			evt.evt_details |=
				((ESPI_PERIPHERAL_INDEX_0 << 16) | ESPI_PERIPHERAL_DEBUG_PORT80);
			break;
		case XEC_PBD_EC_DA_LANE_1:
			evt.evt_details |=
				((ESPI_PERIPHERAL_INDEX_1 << 16) | ESPI_PERIPHERAL_DEBUG_PORT80);
			break;
		case XEC_PBD_EC_DA_LANE_2:
			break;
		case XEC_PBD_EC_DA_LANE_3:
			break;
		default:
			break;
		}

		if (evt.evt_details) {
			espi_send_callbacks(&data->callbacks, dev, evt);
			evt.evt_details = 0;
		}

		dattr = sys_read32(p80rb + XEC_PBD_EC_DA_OFS);
	}

	/* clear GIRQ status */
	xec_ecia_info_girq_src_clear(xec_p80bd0_cfg.ecia_info);
}
#endif

static int connect_irq_p80bd0(const struct device *dev)
{
	xec_ecia_info_girq_src_clear(xec_p80bd0_cfg.ecia_info);

	IRQ_CONNECT(DT_IRQN(DT_NODELABEL(p80bd0)), DT_IRQ(DT_NODELABEL(acpi_ec1), priority),
		    p80bd0_isr, DEVICE_DT_GET(DT_NODELABEL(espi0)), 0);
	irq_enable(DT_IRQN(DT_NODELABEL(p80bd0)));

	xec_ecia_info_girq_ctrl(xec_p80bd0_cfg.ecia_info, 1u);

	return 0;
}

static int init_p80bd0(const struct device *dev)
{
	const struct espi_xec_config *cfg = dev->config;
	struct xec_espi_ioc_cfg_regs *cfgregs =
		(struct xec_espi_ioc_cfg_regs *)(cfg->ioc_base_addr + MCHP_ESPI_IO_CFG_OFS);
	mm_reg_t p80rb = xec_p80bd0_cfg.regbase;
	uint32_t bar_val = MCHP_ESPI_IO_BAR_HOST_ADDR_SET(ESPI_XEC_PORT80_HOST_ADDR);

	cfgregs->IOHBAR[IOB_P80BD] = (bar_val | MCHP_ESPI_IO_BAR_HOST_VALID);

	soc_set_bit8(p80rb + XEC_PBD_LD_CFG_OFS, XEC_PBD_LD_CFG_ACTV_POS);
	soc_set_bit8(p80rb + XEC_PBD_IER_OFS, XEC_PBD_IER_FTHR_POS);

	return 0;
}

#undef CONNECT_IRQ_P80BD0
#define CONNECT_IRQ_P80BD0 connect_irq_p80bd0
#undef INIT_P80BD0
#define INIT_P80BD0 init_p80bd0

#endif /* CONFIG_ESPI_PERIPHERAL_DEBUG_PORT_80 */

#ifdef CONFIG_ESPI_PERIPHERAL_UART

struct xec_uart_config {
	uintptr_t regbase;
	uint16_t host_io_addr;
};

#if CONFIG_ESPI_PERIPHERAL_UART_SOC_MAPPING == 1
#define XEC_UART_REG_BASE     DT_REG_ADDR(DT_NODELABEL(uart1))
#define XEC_UART_HOST_IO_ADDR DT_PROP_OR(DT_NODELABEL(uart1), host_io, ESPI_XEC_UART_HOST_ADDR)
#define XEC_UART_BAR_IDX      IOB_UART1
#define XEC_UART_SIRQ_IDX     SIRQ_UART1
#elif CONFIG_ESPI_PERIPHERAL_UART_SOC_MAPPING == 2
#define XEC_UART_REG_BASE     DT_REG_ADDR(DT_NODELABEL(uart2))
#define XEC_UART_HOST_IO_ADDR DT_PROP_OR(DT_NODELABEL(uart2), host_io, ESPI_XEC_UART_HOST_ADDR)
#define XEC_UART_BAR_IDX      IOB_UART2
#define XEC_UART_SIRQ_IDX     SIRQ_UART2
#elif CONFIG_ESPI_PERIPHERAL_UART_SOC_MAPPING == 3
#define XEC_UART_REG_BASE     DT_REG_ADDR(DT_NODELABEL(uart3))
#define XEC_UART_HOST_IO_ADDR DT_PROP_OR(DT_NODELABEL(uart3), host_io, ESPI_XEC_UART_HOST_ADDR)
#define XEC_UART_BAR_IDX      IOB_UART3
#define XEC_UART_SIRQ_IDX     SIRQ_UART3
#else
/* default to UART0 */
#define XEC_UART_REG_BASE     DT_REG_ADDR(DT_NODELABEL(uart0))
#define XEC_UART_HOST_IO_ADDR DT_PROP_OR(DT_NODELABEL(uart0), host_io, ESPI_XEC_UART_HOST_ADDR)
#define XEC_UART_BAR_IDX      IOB_UART0
#define XEC_UART_SIRQ_IDX     SIRQ_UART0
#endif

const struct xec_uart_config xec_uart_cfg = {
	.regbase = (uintptr_t)XEC_UART_REG_BASE,
	.host_io_addr = (uint16_t)XEC_UART_HOST_IO_ADDR,
};

int init_uart(const struct device *dev)
{
	const struct espi_xec_config *cfg = dev->config;
	struct xec_espi_ioc_cfg_regs *regs =
		(struct xec_espi_ioc_cfg_regs *)(cfg->ioc_base_addr + MCHP_ESPI_IO_CFG_OFS);
	uint32_t iobar_val = MCHP_ESPI_IO_BAR_HOST_ADDR_SET(xec_uart_cfg.host_io_addr);

	regs->IOHBAR[XEC_UART_BAR_IDX] = iobar_val | MCHP_ESPI_IO_BAR_HOST_VALID;

	regs->SIRQ[XEC_UART_SIRQ_IDX] = UART_DEFAULT_IRQ;

	return 0;
}

#undef INIT_UART
#define INIT_UART init_uart

#endif /* CONFIG_ESPI_PERIPHERAL_UART */

typedef int (*host_dev_irq_connect)(const struct device *dev);

static const host_dev_irq_connect hdic_tbl[] = {
	CONNECT_IRQ_MBOX0,    CONNECT_IRQ_KBC0,     CONNECT_IRQ_ACPI_EC0, CONNECT_IRQ_ACPI_EC1,
	CONNECT_IRQ_ACPI_EC2, CONNECT_IRQ_ACPI_EC3, CONNECT_IRQ_ACPI_EC4, CONNECT_IRQ_ACPI_PM1,
	CONNECT_IRQ_EMI0,     CONNECT_IRQ_EMI1,     CONNECT_IRQ_EMI2,     CONNECT_IRQ_RTC0,
	CONNECT_IRQ_P80BD0,
};

typedef int (*host_dev_init)(const struct device *dev);

static const host_dev_init hd_init_tbl[] = {
	INIT_MBOX0,    INIT_KBC0,     INIT_ACPI_EC0, INIT_ACPI_EC1, INIT_ACPI_EC2,
	INIT_ACPI_EC3, INIT_ACPI_EC4, INIT_ACPI_PM1, INIT_EMI0,     INIT_EMI1,
	INIT_EMI2,     INIT_RTC0,     INIT_P80BD0,   INIT_UART,
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

/* Configure peripheral channel devices' I/O and memory BARs
 * SRAM BARs and PC device memory BARs host address bits[47:32] are specified by the
 * SRAM BAR extended address register and Host device memory extended address registers.
 * Loop over table of host device initialization functions.
 */
int xec_host_dev_init(const struct device *dev)
{
	const struct espi_xec_config *devcfg = dev->config;
	struct xec_espi_mc_cfg_regs *const mc_cfg_regs =
		(struct xec_espi_mc_cfg_regs *)(devcfg->mc_base_addr + MCHP_ESPI_MC_CFG_OFS);
	int ret = 0;

	mc_cfg_regs->HBAR_EXT = XEC_PC_DEV_MBAR_HOST_ADDR_HIGH;
	mc_cfg_regs->SRAM_EXT = XEC_PC_SRAM_BAR_HOST_ADDR_HIGH;

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

typedef int (*xec_lpc_req)(const struct device *, enum lpc_peripheral_opcode, uint32_t *);

struct espi_lpc_req {
	uint16_t opcode_start;
	uint16_t opcode_max;
	xec_lpc_req rd_req;
	xec_lpc_req wr_req;
};

static const struct espi_lpc_req espi_lpc_req_tbl[] = {
#ifdef CONFIG_ESPI_PERIPHERAL_8042_KBC
	{E8042_START_OPCODE, E8042_MAX_OPCODE, kbc0_rd_req, kbc0_wr_req},
#endif
#ifdef CONFIG_ESPI_PERIPHERAL_HOST_IO
	{EACPI_START_OPCODE, EACPI_MAX_OPCODE, eacpi_rd_req, eacpi_wr_req},
#endif
#if defined(CONFIG_ESPI_PERIPHERAL_EC_HOST_CMD) && defined(CONFIG_ESPI_PERIPHERAL_ACPI_SHM_REGION)
	{EACPI_GET_SHARED_MEMORY, EACPI_GET_SHARED_MEMORY, eacpi_shm_rd_req, eacpi_shm_wr_req},
#endif
#ifdef CONFIG_ESPI_PERIPHERAL_CUSTOM_OPCODE
	{ECUSTOM_START_OPCODE, ECUSTOM_MAX_OPCODE, ecust_rd_req, ecust_wr_req},
#endif
};

static int espi_xec_lpc_req(const struct device *dev, enum lpc_peripheral_opcode op, uint32_t *data,
			    uint8_t write)
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
int espi_xec_read_lpc_request(const struct device *dev, enum lpc_peripheral_opcode op,
			      uint32_t *data)
{
	return espi_xec_lpc_req(dev, op, data, 0);
}

int espi_xec_write_lpc_request(const struct device *dev, enum lpc_peripheral_opcode op,
			       uint32_t *data)
{
	return espi_xec_lpc_req(dev, op, data, 1);
}
#else
int espi_xec_write_lpc_request(const struct device *dev, enum lpc_peripheral_opcode op,
			       uint32_t *data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(op);
	ARG_UNUSED(data);

	return -ENOTSUP;
}

int espi_xec_read_lpc_request(const struct device *dev, enum lpc_peripheral_opcode op,
			      uint32_t *data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(op);
	ARG_UNUSED(data);

	return -ENOTSUP;
}
#endif /* CONFIG_ESPI_PERIPHERAL_CHANNEL */

#if defined(CONFIG_ESPI_PERIPHERAL_CUSTOM_OPCODE)
static void host_cust_opcode_enable_interrupts(void)
{
	/* Enable host KBC sub-device interrupt */
	if (IS_ENABLED(CONFIG_ESPI_PERIPHERAL_8042_KBC)) {
		xec_ecia_info_girq_ctrl(xec_kbc0_cfg.ibf_ecia_info, 1u);
	}

	/* Enable host ACPI EC0 (Host IO) and
	 * ACPI EC1 (Host CMD) sub-device interrupt
	 */
	if (IS_ENABLED(CONFIG_ESPI_PERIPHERAL_HOST_IO) ||
	    IS_ENABLED(CONFIG_ESPI_PERIPHERAL_EC_HOST_CMD)) {
		xec_ecia_info_girq_ctrl(xec_acpi_ec0_cfg.ibf_ecia_info, 1u);
		xec_ecia_info_girq_ctrl(xec_acpi_ec1_cfg.ibf_ecia_info, 1u);
	}

	/* Enable host Port80 sub-device interrupt installation */
	if (IS_ENABLED(CONFIG_ESPI_PERIPHERAL_DEBUG_PORT_80)) {
		xec_ecia_info_girq_ctrl(xec_p80bd0_cfg.ecia_info, 1u);
	}
}

static void host_cust_opcode_disable_interrupts(void)
{
	/* Disable host KBC sub-device interrupt */
	if (IS_ENABLED(CONFIG_ESPI_PERIPHERAL_8042_KBC)) {
		xec_ecia_info_girq_ctrl(xec_kbc0_cfg.ibf_ecia_info, 0);
		xec_ecia_info_girq_ctrl(xec_kbc0_cfg.obe_ecia_info, 0);
	}

	/* Disable host ACPI EC0 (Host IO) and
	 * ACPI EC1 (Host CMD) sub-device interrupt
	 */
	if (IS_ENABLED(CONFIG_ESPI_PERIPHERAL_HOST_IO) ||
	    IS_ENABLED(CONFIG_ESPI_PERIPHERAL_EC_HOST_CMD)) {
		xec_ecia_info_girq_ctrl(xec_acpi_ec0_cfg.ibf_ecia_info, 0);
		xec_ecia_info_girq_ctrl(xec_acpi_ec0_cfg.obe_ecia_info, 0);
		xec_ecia_info_girq_ctrl(xec_acpi_ec1_cfg.ibf_ecia_info, 0);
	}

	/* Disable host Port80 sub-device interrupt installation */
	if (IS_ENABLED(CONFIG_ESPI_PERIPHERAL_DEBUG_PORT_80)) {
		xec_ecia_info_girq_ctrl(xec_p80bd0_cfg.ecia_info, 0);
	}
}
#endif /* CONFIG_ESPI_PERIPHERAL_CUSTOM_OPCODE */
