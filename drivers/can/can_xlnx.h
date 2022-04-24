/*
 * Xilinx Processor System CAN controller driver
 *
 * Driver private data declarations
 *
 * Copyright (c) 2022, Weidmueller Interface GmbH & Co. KG
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _ZEPHYR_DRIVERS_CAN_CAN_XLNX_H_
#define _ZEPHYR_DRIVERS_CAN_CAN_XLNX_H_

#include <stdint.h>

/* The Xilinx CAN controller always has 4 acceptance filters */
#define CAN_XLNX_NUM_ACCEPTANCE_FILTERS	4
/* The RX/TX FIFOs of the Xilinx CAN controller are always 64 entries deep */
#define CAN_XLNX_RX_TX_FIFO_DEPTH	64

/*
 * Register offsets within the respective CAN controller's
 * address space: comp. Zynq-7000 TRM, appendix B.5, p. 796 ff.
 * SRR          = Software Reset                        register
 * MSR          = Mode Select                           register
 * BRPR         = Baud Rate Prescaler                   register
 * BTR          = Bit Timing                            register
 * ECR          = Error Counter                         register
 * ESR          = Error Status                          register
 * SR           = Status                                register
 * ISR          = Interrupt Status                      register
 * IER          = Interrupt Enable                      register
 * ICR          = Interrupt Clear                       register
 * TCR          = Timestamp Control                     register
 * WIR          = Watermark Interrupt                   register
 * TXFIFO_ID    = Transmit Message FIFO identifier      register
 * TXFIFO_DLC   = Transmit Message FIFO DLC             register
 * TXFIFO_DATA1 = Transmit Message FIFO data word 1     register
 * TXFIFO_DATA2 = Transmit Message FIFO data word 2     register
 * TXHPB_ID     = Transmit High Prio identifier         register
 * TXHPB_DLC    = Transmit High Prio DLC                register
 * TXHPB_DATA1  = Transmit High Prio buffer data word 1 register
 * TXHPB_DATA2  = Transmit High Prio buffer data word 2 register
 * RXFIFO_ID    = Receive Message FIFO identifier       register
 * RXFIFO_DLC   = Receive Message FIFO DLC              register
 * RXFIFO_DATA1 = Receive Message FIFO data word 1      register
 * RXFIFO_DATA2 = Receive Message FIFO data word 2      register
 * AFR          = Acceptance Filter                     register
 * AFMR1        = Acceptance Filter Mask                register 1
 * AFIR1        = Acceptance Filter ID                  register 1
 * AFMR2        = Acceptance Filter Mask                register 2
 * AFIR2        = Acceptance Filter ID                  register 2
 * AFMR3        = Acceptance Filter Mask                register 3
 * AFIR3        = Acceptance Filter ID                  register 3
 * AFMR4        = Acceptance Filter Mask                register 4
 * AFIR4        = Acceptance Filter ID                  register 4
 */
#define CAN_XLNX_SRR_OFFSET		0x00000000
#define CAN_XLNX_MSR_OFFSET		0x00000004
#define CAN_XLNX_BRPR_OFFSET		0x00000008
#define CAN_XLNX_BTR_OFFSET		0x0000000C
#define CAN_XLNX_ECR_OFFSET		0x00000010
#define CAN_XLNX_ESR_OFFSET		0x00000014
#define CAN_XLNX_SR_OFFSET		0x00000018
#define CAN_XLNX_ISR_OFFSET		0x0000001C
#define CAN_XLNX_IER_OFFSET		0x00000020
#define CAN_XLNX_ICR_OFFSET		0x00000024
#define CAN_XLNX_TCR_OFFSET		0x00000028
#define CAN_XLNX_WIR_OFFSET		0x0000002C
#define CAN_XLNX_TXFIFO_ID_OFFSET	0x00000030
#define CAN_XLNX_TXFIFO_DLC_OFFSET	0x00000034
#define CAN_XLNX_TXFIFO_DATA1_OFFSET	0x00000038
#define CAN_XLNX_TXFIFO_DATA2_OFFSET	0x0000003C
#define CAN_XLNX_TXHPB_ID_OFFSET	0x00000040
#define CAN_XLNX_TXHPB_DLC_OFFSET	0x00000044
#define CAN_XLNX_TXHPB_DATA1_OFFSET	0x00000048
#define CAN_XLNX_TXHPB_DATA2_OFFSET	0x0000004C
#define CAN_XLNX_RXFIFO_ID_OFFSET	0x00000050
#define CAN_XLNX_RXFIFO_DLC_OFFSET	0x00000054
#define CAN_XLNX_RXFIFO_DATA1_OFFSET	0x00000058
#define CAN_XLNX_RXFIFO_DATA2_OFFSET	0x0000005C
#define CAN_XLNX_AFR_OFFSET		0x00000060
#define CAN_XLNX_AFMR1_OFFSET		0x00000064
#define CAN_XLNX_AFIR1_OFFSET		0x00000068
#define CAN_XLNX_AFMR2_OFFSET		0x0000006C
#define CAN_XLNX_AFIR2_OFFSET		0x00000070
#define CAN_XLNX_AFMR3_OFFSET		0x00000074
#define CAN_XLNX_AFIR3_OFFSET		0x00000078
#define CAN_XLNX_AFMR4_OFFSET		0x0000007C
#define CAN_XLNX_AFIR4_OFFSET		0x00000080

/*
 * Software Reset register bits
 * Bit assignments: comp. Zynq-7000 TRM, appendix B.5, p. 797 f.
 */
#define CAN_XLNX_SRR_CAN_ENABLE		BIT(1)
#define CAN_XLNX_SRR_SOFTWARE_RESET	BIT(0)

/*
 * Mode Select register bits
 * Bit assignments: comp. Zynq-7000 TRM, appendix B.5, p. 798 f.
 */
#define CAN_XLNX_MSR_SNOOP		BIT(2)
#define CAN_XLNX_MSR_LOOPBACK		BIT(1)
#define CAN_XLNX_MSR_SLEEP		BIT(0)

/*
 * Baudrate Prescaler register mask
 * Bit assignments: comp. Zynq-7000 TRM, appendix B.5, p. 799 f.
 */
#define CAN_XLNX_BRPR_PRESCALER_MASK	0x000000FF
#define CAN_XLNX_BRPR_MIN_PRESCALER	1
#define CAN_XLNX_BRPR_MAX_PRESCALER	256

/*
 * Bit Timing register offsets and masks
 * Bit assignments: comp. Zynq-7000 TRM, appendix B.5, p. 800
 */
#define CAN_XLNX_BTR_SJW_OFFSET		7
#define CAN_XLNX_BTR_SJW_MASK		0x3
#define CAN_XLNX_BTR_TS2_OFFSET		4
#define CAN_XLNX_BTR_TS2_MASK		0x7
#define CAN_XLNX_BTR_TS1_OFFSET		0
#define CAN_XLNX_BTR_TS1_MASK		0xF

/*
 * Error Counter register offsets and masks
 * Bit assignments: comp. Zynq-7000 TRM, appendix B.5, p. 801
 */
#define CAN_XLNX_ECR_RX_ERRORS_OFFSET	8
#define CAN_XLNX_ECR_RX_ERRORS_MASK	0xFF
#define CAN_XLNX_ECR_TX_ERRORS_OFFSET	0
#define CAN_XLNX_ECR_TX_ERRORS_MASK	0xFF

/*
 * Error Status register bits
 * Bit assignments: comp. Zynq-7000 TRM, appendix B.5, p. 801 ff.
 */
#define CAN_XLNX_ESR_ACK_ERROR		BIT(4)
#define CAN_XLNX_ESR_BIT_ERROR		BIT(3)
#define CAN_XLNX_ESR_STUFF_ERROR	BIT(2)
#define CAN_XLNX_ESR_FORM_ERROR		BIT(1)
#define CAN_XLNX_ESR_CRC_ERROR		BIT(0)
#define CAN_XLNX_ESR_CLEAR_ALL_MASK	0x1F

/*
 * Status register bits, offsets and masks
 * Bit assignments: comp. Zynq-7000 TRM, appendix B.5, p. 803 ff.
 */
#define CAN_XLNX_SR_SNOOP_MODE		BIT(12)
#define CAN_XLNX_SR_ACC_FLTR_BUSY	BIT(11)
#define CAN_XLNX_SR_TX_FIFO_FULL	BIT(10)
#define CAN_XLNX_SR_TX_HIGH_PRIO_FULL	BIT(9)
#define CAN_XLNX_SR_ERROR_STATUS_OFFSET	7
#define CAN_XLNX_SR_ERROR_STATUS_MASK	0x3
#define CAN_XLNX_SR_ESTAT_ERR_ACTIVE	0x1
#define CAN_XLNX_SR_ESTAT_BUS_OFF	0x2
#define CAN_XLNX_SR_ESTAT_ERR_PASSIVE	0x3
#define CAN_XLNX_SR_ERROR_WARNING	BIT(6)
#define CAN_XLNX_SR_BUS_BUSY		BIT(5)
#define CAN_XLNX_SR_BUS_IDLE		BIT(4)
#define CAN_XLNX_SR_NORMAL_MODE		BIT(3)
#define CAN_XLNX_SR_SLEEP_MODE		BIT(2)
#define CAN_XLNX_SR_LOOPBACK_MODE	BIT(1)
#define CAN_XLNX_SR_CONFIG_MODE		BIT(0)

/*
 * Interrupt status / enable / clear bits
 * Bit assignments: comp. Zynq-7000 TRM, appendix B.5, p. 805 ff.
 */
#define CAN_XLNX_IRQ_TX_EMPTY		BIT(14)
#define CAN_XLNX_IRQ_TX_WATERMARK	BIT(13)
#define CAN_XLNX_IRQ_RX_WATERMARK	BIT(12)
#define CAN_XLNX_IRQ_SLEEP_MODE_EXIT	BIT(11)
#define CAN_XLNX_IRQ_SLEEP_MODE_ENTER	BIT(10)
#define CAN_XLNX_IRQ_BUS_OFF		BIT(9)
#define CAN_XLNX_IRQ_MESSAGE_ERROR	BIT(8)
#define CAN_XLNX_IRQ_RX_NOT_EMPTY	BIT(7)
#define CAN_XLNX_IRQ_RX_OVERFLOW	BIT(6)
#define CAN_XLNX_IRQ_RX_UNDERFLOW	BIT(5)
#define CAN_XLNX_IRQ_MESSAGE_RX		BIT(4)
#define CAN_XLNX_IRQ_TXHPB_FULL		BIT(3)
#define CAN_XLNX_IRQ_TX_FULL		BIT(2)
#define CAN_XLNX_IRQ_MESSAGE_TX		BIT(1)
#define CAN_XLNX_IRQ_ARBITRATION_LOST	BIT(0)

/*
 * Timestamp Control register bits
 * Bit assignments: comp. Zynq-7000 TRM, appendix B.5, p. 812 f.
 */
#define CAN_XLNX_CTR_CLEAR_TIMESTAMP	BIT(0)

/*
 * Watermark Interrupt register offsets and masks
 * Bit assignments: comp. Zynq-7000 TRM, appendix B.5, p. 813 f.
 */
#define CAN_XLNX_WIR_TX_EMPTY_OFFSET	8
#define CAN_XLNX_WIR_TX_EMPTY_MASK	0xFF
#define CAN_XLNX_WIR_RX_FULL_OFFSET	0
#define CAN_XLNX_WIR_RX_FULL_MASK	0xFF

/*
 * FIFO registers offsets and masks
 * Bit assignments: comp. Zynq-7000 TRM, appendix B.5, p. 814 ff.
 */
#define CAN_XLNX_FIFO_IDR_IDH_OFFSET	21
#define CAN_XLNX_FIFO_IDR_IDH_MASK	0x7FF
#define CAN_XLNX_FIFO_IDR_SRRRTR	BIT(20)
#define CAN_XLNX_FIFO_IDR_IDE		BIT(19)
#define CAN_XLNX_FIFO_IDR_IDL_OFFSET	1
#define CAN_XLNX_FIFO_IDR_IDL_MASK	0x3FFFF
#define CAN_XLNX_FIFO_IDR_RTR		BIT(0)
#define CAN_XLNX_FIFO_DLCR_DLC_OFFSET	28
#define CAN_XLNX_FIFO_DLCR_DLC_MASK	0xF
#define CAN_XLNX_FIFO_DLCR_RXT_MASK	0xFFFFF

/*
 * Acceptance Filter register bits
 * Bit assignments: comp. Zynq-7000 TRM, appendix B.5, p. 823 f.
 */
#define CAN_XLNX_AFR_UAF4		BIT(3)
#define CAN_XLNX_AFR_UAF3		BIT(2)
#define CAN_XLNX_AFR_UAF2		BIT(1)
#define CAN_XLNX_AFR_UAF1		BIT(0)

/*
 * Acceptance Filter Mask and ID registers bits, offsets and masks
 * Bit assignments: comp. Zynq-7000 TRM, appendix B.5, p. 825 ff.
 */
#define CAN_XLNX_AFR_STD_ID_OFFSET	21
#define CAN_XLNX_AFR_STD_ID_MASK	0x7FF
#define CAN_XLNX_AFR_SRR_RTR		BIT(20)
#define CAN_XLNX_AFR_IDE		BIT(19)
#define CAN_XLNX_AFR_EXT_ID_OFFSET	1
#define CAN_XLNX_AFR_EXT_ID_MASK	0x3FFFF
#define CAN_XLNX_AFR_RTR		BIT(0)

/* I/O pin control macros, depending on this feature being enabled */
#if CONFIG_PINCTRL
#define CAN_XLNX_DEV_PINCTRL_DEFINE(idx) PINCTRL_DT_INST_DEFINE(idx);
#define CAN_XLNX_DEV_PINCTRL_INIT(idx) .pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(idx),
#else
#define CAN_XLNX_DEV_PINCTRL_DEFINE(idx)
#define CAN_XLNX_DEV_PINCTRL_INIT(idx)
#endif /* CONFIG_PINCTRL */

/* Device config & run-time data struct creation macros */
#define CAN_XLNX_DEV_DATA(idx)\
static struct can_xlnx_dev_data can_xlnx##idx##_data = {\
	.state = CAN_ERROR_ACTIVE,\
	.state_chg_cb = NULL,\
	.state_chg_cb_data = NULL,\
	.tx_done_handlers = {{0}},\
	.tx_done_wr_idx = 0,\
	.tx_done_rd_idx = 0,\
	.rx_filters = {{0}},\
	.filter_usage_mask = 0,\
	.bus_off = 0,\
	.tx_errors = 0,\
	.rx_errors = 0\
};

#define CAN_XLNX_DEV_CONFIG(idx)\
static const struct can_xlnx_dev_cfg can_xlnx##idx##_cfg = {\
	DEVICE_MMIO_ROM_INIT(DT_DRV_INST(idx)),\
	CAN_XLNX_DEV_PINCTRL_INIT(idx)\
	.clock_frequency = DT_INST_PROP(idx, clock_frequency),\
	.config_func = can_xlnx##idx##_irq_config,\
	.bus_speed = DT_INST_PROP(idx, bus_speed),\
	.max_bitrate = DT_CAN_TRANSCEIVER_MAX_BITRATE(node_id, 1000000),\
	.sample_point = DT_INST_PROP(idx, sample_point),\
	.sjw = DT_INST_PROP(idx, sjw)\
};

/* Macro used to generate each CAN device's IRQ attach function. */
#define CAN_XLNX_DEV_CONFIG_IRQ_FUNC(idx)\
static void can_xlnx##idx##_irq_config(const struct device *dev)\
{\
	ARG_UNUSED(dev);\
	IRQ_CONNECT(DT_INST_IRQN(idx), DT_INST_IRQ(idx, priority),\
		    can_xlnx_isr, DEVICE_DT_INST_GET(idx), 0);\
	irq_enable(DT_INST_IRQN(idx));\
}

/* Device definition macro */
#define CAN_XLNX_DEV_DEFINE(idx)\
DEVICE_DT_INST_DEFINE(idx, can_xlnx_init, NULL,\
	&can_xlnx##idx##_data, &can_xlnx##idx##_cfg,\
	POST_KERNEL, CONFIG_CAN_INIT_PRIORITY, &can_xlnx_apis);

/*
 * Top-level device initialization macro, executed for each CAN device
 * entry contained in the device tree which has status "okay".
 */
#define CAN_XLNX_DEV_INITITALIZE(idx)\
CAN_XLNX_DEV_PINCTRL_DEFINE(idx)\
CAN_XLNX_DEV_CONFIG_IRQ_FUNC(idx)\
CAN_XLNX_DEV_DATA(idx)\
CAN_XLNX_DEV_CONFIG(idx)\
CAN_XLNX_DEV_DEFINE(idx)

/* Type definitions */

/* IRQ handler function type */
typedef void (*can_xlnx_config_irq_t)(const struct device *dev);

/**
 * @brief RX filter data.
 *
 * This struct contains all data associated with each of the 4
 * RX filters than can be set up by the user.
 */
struct can_xlnx_filter_data {
	can_rx_callback_t cb;
	void *user_data;
	struct can_filter filter;
};

/**
 * @brief TX done handler information array
 *
 * This struct contains the information required to confirm to
 * the respective sending context that a CAN message has been
 * transmitted. This can either be indicated by giving to a
 * semaphore or calling a callback. Both this information is
 * contained herein.
 */
struct can_xlnx_tx_done_handler {
	can_tx_callback_t cb;
	void *user_data;
	struct k_sem *sem;
};

/**
 * @brief Run-time modifiable device data structure.
 *
 * This struct contains all data of the CAN controller which
 * is modifiable at run-time.
 */
struct can_xlnx_dev_data {
	DEVICE_MMIO_RAM;
	struct k_sem filter_prot_sem;
	struct k_sem tx_done_prot_sem;
	struct k_sem tx_done_sem; /* TODO: array with size [num_cpus] */
#ifndef CONFIG_CAN_AUTO_BUS_OFF_RECOVERY
	struct k_sem recovery_sem;
#endif

	enum can_state state;
	can_state_change_callback_t state_chg_cb;
	void *state_chg_user_data;

	struct can_xlnx_tx_done_handler tx_done_handlers[CAN_XLNX_RX_TX_FIFO_DEPTH];
	uint8_t tx_done_wr_idx;
	uint8_t tx_done_rd_idx;

	struct can_xlnx_filter_data rx_filters[CAN_XLNX_NUM_ACCEPTANCE_FILTERS];
	uint32_t filter_usage_mask;

	uint8_t bus_off;
	uint8_t tx_errors;
	uint8_t rx_errors;
};

/**
 * @brief Constant device configuration data structure.
 *
 * This struct contains all data of the CAN controller which
 * is required for proper operation (such as base memory
 * address etc.) which doesn't have to and therefore cannot
 * be modified at run-time.
 */
struct can_xlnx_dev_cfg {
	DEVICE_MMIO_ROM;
#ifdef CONFIG_PINCTRL
	const struct pinctrl_dev_config *pincfg;
#endif
	uint32_t clock_frequency;
	can_xlnx_config_irq_t config_func;
	uint32_t bus_speed;
	uint32_t max_bitrate;
	uint16_t sample_point;
	uint8_t sjw;
};

#endif /* _ZEPHYR_DRIVERS_CAN_CAN_XLNX_H_ */
