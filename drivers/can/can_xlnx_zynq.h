/*
 * Xilinx Processor System CAN controller driver
 *
 * Driver private data declarations
 * All data regarding register offsets, bit positions/masks etc. was obtained from:
 * Zynq-7000 SoC Technical Reference Manual (TRM), Xilinx document ID UG585, rev. 1.13
 *
 * Copyright (c) 2024, Immo Birnbaum
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _ZEPHYR_DRIVERS_CAN_CAN_XLNX_ZYNQ_H_
#define _ZEPHYR_DRIVERS_CAN_CAN_XLNX_ZYNQ_H_

#include <stdint.h>
#include <zephyr/kernel.h>

/*
 * Register offsets within the respective CAN controller's address space:
 * comp. TRM appendix B.5, p. 796 ff.
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
 */
#define CAN_XLNX_ZYNQ_SRR_OFFSET 0x00000000
#define CAN_XLNX_ZYNQ_MSR_OFFSET 0x00000004
#define CAN_XLNX_ZYNQ_BRPR_OFFSET 0x00000008
#define CAN_XLNX_ZYNQ_BTR_OFFSET 0x0000000C
#define CAN_XLNX_ZYNQ_ECR_OFFSET 0x00000010
#define CAN_XLNX_ZYNQ_ESR_OFFSET 0x00000014
#define CAN_XLNX_ZYNQ_SR_OFFSET 0x00000018
#define CAN_XLNX_ZYNQ_ISR_OFFSET 0x0000001C
#define CAN_XLNX_ZYNQ_IER_OFFSET 0x00000020
#define CAN_XLNX_ZYNQ_ICR_OFFSET 0x00000024
#define CAN_XLNX_ZYNQ_TCR_OFFSET 0x00000028
#define CAN_XLNX_ZYNQ_WIR_OFFSET 0x0000002C
#define CAN_XLNX_ZYNQ_TXFIFO_ID_OFFSET 0x00000030
#define CAN_XLNX_ZYNQ_TXFIFO_DLC_OFFSET 0x00000034
#define CAN_XLNX_ZYNQ_TXFIFO_DATA1_OFFSET 0x00000038
#define CAN_XLNX_ZYNQ_TXFIFO_DATA2_OFFSET 0x0000003C
#define CAN_XLNX_ZYNQ_TXHPB_ID_OFFSET 0x00000040
#define CAN_XLNX_ZYNQ_TXHPB_DLC_OFFSET 0x00000044
#define CAN_XLNX_ZYNQ_TXHPB_DATA1_OFFSET 0x00000048
#define CAN_XLNX_ZYNQ_TXHPB_DATA2_OFFSET 0x0000004C
#define CAN_XLNX_ZYNQ_RXFIFO_ID_OFFSET 0x00000050
#define CAN_XLNX_ZYNQ_RXFIFO_DLC_OFFSET 0x00000054
#define CAN_XLNX_ZYNQ_RXFIFO_DATA1_OFFSET 0x00000058
#define CAN_XLNX_ZYNQ_RXFIFO_DATA2_OFFSET 0x0000005C
#define CAN_XLNX_ZYNQ_AFR_OFFSET 0x00000060

/* Software Reset register bits: TRM appendix B.5, p. 797 f. */
#define CAN_XLNX_ZYNQ_SRR_CAN_ENABLE BIT(1)
#define CAN_XLNX_ZYNQ_SRR_SOFTWARE_RESET BIT(0)

/* Mode Select register bits: TRM appendix B.5, p. 798 f. */
#define CAN_XLNX_ZYNQ_MSR_SNOOP BIT(2)
#define CAN_XLNX_ZYNQ_MSR_LOOPBACK BIT(1)
#define CAN_XLNX_ZYNQ_MSR_SLEEP BIT(0)

/* Baudrate Prescaler register mask: TRM appendix B.5, p. 799 f. */
#define CAN_XLNX_ZYNQ_BRPR_PRESCALER_MASK 0x000000FF
#define CAN_XLNX_ZYNQ_BRPR_MIN_PRESCALER 1
#define CAN_XLNX_ZYNQ_BRPR_MAX_PRESCALER 256

/* Bit Timing register offsets and masks: TRM appendix B.5, p. 800 */
#define CAN_XLNX_ZYNQ_BTR_SJW_OFFSET 7
#define CAN_XLNX_ZYNQ_BTR_SJW_MASK 0x3
#define CAN_XLNX_ZYNQ_BTR_TS2_OFFSET 4
#define CAN_XLNX_ZYNQ_BTR_TS2_MASK 0x7
#define CAN_XLNX_ZYNQ_BTR_TS1_OFFSET 0
#define CAN_XLNX_ZYNQ_BTR_TS1_MASK 0xF

/* Error Counter register offsets and masks: TRM appendix B.5, p. 801 */
#define CAN_XLNX_ZYNQ_ECR_RX_ERRORS_OFFSET 8
#define CAN_XLNX_ZYNQ_ECR_RX_ERRORS_MASK 0xFF
#define CAN_XLNX_ZYNQ_ECR_TX_ERRORS_OFFSET 0
#define CAN_XLNX_ZYNQ_ECR_TX_ERRORS_MASK 0xFF

/* Error Status register bits: TRM appendix B.5, p. 801 ff. */
#define CAN_XLNX_ZYNQ_ESR_ACK_ERROR BIT(4)
#define CAN_XLNX_ZYNQ_ESR_BIT_ERROR BIT(3)
#define CAN_XLNX_ZYNQ_ESR_STUFF_ERROR BIT(2)
#define CAN_XLNX_ZYNQ_ESR_FORM_ERROR BIT(1)
#define CAN_XLNX_ZYNQ_ESR_CRC_ERROR BIT(0)
#define CAN_XLNX_ZYNQ_ESR_CLEAR_ALL_MASK 0x1F

/* Status register bits, offsets and masks: TRM appendix B.5, p. 803 ff. */
#define CAN_XLNX_ZYNQ_SR_SNOOP_MODE BIT(12)
#define CAN_XLNX_ZYNQ_SR_ACC_FLTR_BUSY BIT(11)
#define CAN_XLNX_ZYNQ_SR_TX_FIFO_FULL BIT(10)
#define CAN_XLNX_ZYNQ_SR_TX_HIGH_PRIO_FULL BIT(9)
#define CAN_XLNX_ZYNQ_SR_ERROR_STATUS_OFFSET 7
#define CAN_XLNX_ZYNQ_SR_ERROR_STATUS_MASK 0x3
#define CAN_XLNX_ZYNQ_SR_ESTAT_CONFIG_MODE 0
#define CAN_XLNX_ZYNQ_SR_ESTAT_ERR_ACTIVE 1
#define CAN_XLNX_ZYNQ_SR_ESTAT_BUS_OFF 2
#define CAN_XLNX_ZYNQ_SR_ESTAT_ERR_PASSIVE 3
#define CAN_XLNX_ZYNQ_SR_ERROR_WARNING BIT(6)
#define CAN_XLNX_ZYNQ_SR_BUS_BUSY BIT(5)
#define CAN_XLNX_ZYNQ_SR_BUS_IDLE BIT(4)
#define CAN_XLNX_ZYNQ_SR_NORMAL_MODE BIT(3)
#define CAN_XLNX_ZYNQ_SR_SLEEP_MODE BIT(2)
#define CAN_XLNX_ZYNQ_SR_LOOPBACK_MODE BIT(1)
#define CAN_XLNX_ZYNQ_SR_CONFIG_MODE BIT(0)

/* Interrupt status / enable / clear bits: TRM appendix B.5, p. 805 ff. */
#define CAN_XLNX_ZYNQ_IRQ_TX_EMPTY BIT(14)
#define CAN_XLNX_ZYNQ_IRQ_TX_WATERMARK BIT(13)
#define CAN_XLNX_ZYNQ_IRQ_RX_WATERMARK BIT(12)
#define CAN_XLNX_ZYNQ_IRQ_SLEEP_MODE_EXIT BIT(11)
#define CAN_XLNX_ZYNQ_IRQ_SLEEP_MODE_ENTER BIT(10)
#define CAN_XLNX_ZYNQ_IRQ_BUS_OFF BIT(9)
#define CAN_XLNX_ZYNQ_IRQ_MESSAGE_ERROR BIT(8)
#define CAN_XLNX_ZYNQ_IRQ_RX_NOT_EMPTY BIT(7)
#define CAN_XLNX_ZYNQ_IRQ_RX_OVERFLOW BIT(6)
#define CAN_XLNX_ZYNQ_IRQ_RX_UNDERFLOW BIT(5)
#define CAN_XLNX_ZYNQ_IRQ_MESSAGE_RX BIT(4)
#define CAN_XLNX_ZYNQ_IRQ_TXHPB_FULL BIT(3)
#define CAN_XLNX_ZYNQ_IRQ_TX_FULL BIT(2)
#define CAN_XLNX_ZYNQ_IRQ_MESSAGE_TX BIT(1)
#define CAN_XLNX_ZYNQ_IRQ_ARBITRATION_LOST BIT(0)

/* Timestamp Control register bits: TRM appendix B.5, p. 812 f. */
#define CAN_XLNX_ZYNQ_CTR_CLEAR_TIMESTAMP BIT(0)

/* Watermark Interrupt register offsets and masks: TRM appendix B.5, p. 813 f. */
#define CAN_XLNX_ZYNQ_WIR_TX_EMPTY_OFFSET 8
#define CAN_XLNX_ZYNQ_WIR_TX_EMPTY_MASK	0xFF
#define CAN_XLNX_ZYNQ_WIR_RX_FULL_OFFSET 0
#define CAN_XLNX_ZYNQ_WIR_RX_FULL_MASK 0xFF

/* FIFO registers offsets and masks: TRM appendix B.5, p. 814 ff. */
#define CAN_XLNX_ZYNQ_FIFO_IDR_IDH_OFFSET 21
#define CAN_XLNX_ZYNQ_FIFO_IDR_IDH_MASK 0x7FF
#define CAN_XLNX_ZYNQ_FIFO_IDR_SRRRTR BIT(20)
#define CAN_XLNX_ZYNQ_FIFO_IDR_IDE BIT(19)
#define CAN_XLNX_ZYNQ_FIFO_IDR_IDL_OFFSET 1
#define CAN_XLNX_ZYNQ_FIFO_IDR_IDL_MASK 0x3FFFF
#define CAN_XLNX_ZYNQ_FIFO_IDR_RTR BIT(0)
#define CAN_XLNX_ZYNQ_FIFO_DLCR_DLC_OFFSET 28
#define CAN_XLNX_ZYNQ_FIFO_DLCR_DLC_MASK 0xF
#define CAN_XLNX_ZYNQ_FIFO_DLCR_RXT_MASK 0xFFFFF

/* I/O pin control macros, depending on this feature being enabled */
#ifdef CONFIG_PINCTRL
#define CAN_XLNX_ZYNQ_DEV_PINCTRL_DEFINE(inst) PINCTRL_DT_INST_DEFINE(inst);
#define CAN_XLNX_ZYNQ_DEV_PINCTRL_INIT(inst) .pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),
#else
#define CAN_XLNX_ZYNQ_DEV_PINCTRL_DEFINE(inst)
#define CAN_XLNX_ZYNQ_DEV_PINCTRL_INIT(inst)
#endif /* CONFIG_PINCTRL */

/* Mode/state transition validation retry count */
#define CAN_XLNX_ZYNQ_MODE_STATE_CHANGE_RETRIES 8192

/* Per-instance device initialization macros */
#define CAN_XLNX_ZYNQ_DEV_DATA(inst)\
static struct can_xlnx_zynq_dev_data can_xlnx_zynq_##inst##_data = {\
	.common = {0},\
	.base = 0x0,\
	.state = CAN_STATE_STOPPED,\
	.tx_errors = 0,\
	.rx_errors = 0,\
	.tx_callback = NULL,\
	.tx_user_data = NULL,\
	.timing = {0} \
};

#define CAN_XLNX_ZYNQ_DEV_CONFIG(inst)\
static const struct can_xlnx_zynq_dev_cfg can_xlnx_zynq_##inst##_cfg = {\
	.common = CAN_DT_DRIVER_CONFIG_INST_GET(inst, 0, 1000000),\
	DEVICE_MMIO_NAMED_ROM_INIT(reg_base, DT_DRV_INST(inst)),\
	.irq_config_func = can_xlnx_zynq_##inst##_irq_config,\
	.irq = DT_INST_IRQN(inst),\
	CAN_XLNX_ZYNQ_DEV_PINCTRL_INIT(inst)\
	.clock_frequency = DT_INST_PROP_OR(inst, clock_frequency, 0),\
};

#define CAN_XLNX_ZYNQ_DEV_CONFIG_IRQ_FUNC(inst)\
static void can_xlnx_zynq_##inst##_irq_config(const struct device *dev)\
{\
	ARG_UNUSED(dev);\
	IRQ_CONNECT(DT_INST_IRQN(inst), DT_INST_IRQ(inst, priority),\
		    can_xlnx_zynq_isr, DEVICE_DT_INST_GET(inst), 0);\
	irq_enable(DT_INST_IRQN(inst));\
}

#define CAN_XLNX_ZYNQ_DEV_DEFINE(inst)\
CAN_DEVICE_DT_INST_DEFINE(inst, can_xlnx_zynq_init, NULL,\
			  &can_xlnx_zynq_##inst##_data,\
			  &can_xlnx_zynq_##inst##_cfg,\
			  POST_KERNEL, CONFIG_CAN_INIT_PRIORITY,\
			  &can_xlnx_zynq_driver_api);

#define CAN_XLNX_ZYNQ_DEV_INITITALIZE(inst)\
CAN_XLNX_ZYNQ_DEV_PINCTRL_DEFINE(inst)\
CAN_XLNX_ZYNQ_DEV_CONFIG_IRQ_FUNC(inst)\
CAN_XLNX_ZYNQ_DEV_DATA(inst)\
CAN_XLNX_ZYNQ_DEV_CONFIG(inst)\
CAN_XLNX_ZYNQ_DEV_DEFINE(inst)

/* Type definitions */

typedef void (*can_xlnx_zynq_config_irq_t)(const struct device *dev);

struct can_xlnx_zynq_filter_data {
	struct can_filter filter;
	can_rx_callback_t callback;
	void *user_data;
};

struct can_xlnx_zynq_recovery_work {
	struct k_work_delayable work_item;
	const struct device *dev;
};

/* Run-time modifiable device data */
struct can_xlnx_zynq_dev_data {
	struct can_driver_data common;

	DEVICE_MMIO_NAMED_RAM(reg_base);
	mem_addr_t base;

	enum can_state state;
	uint8_t tx_errors;
	uint8_t rx_errors;

	ATOMIC_DEFINE(rx_filters_allocated, CONFIG_CAN_MAX_FILTER);
	struct can_xlnx_zynq_filter_data rx_filters[CONFIG_CAN_MAX_FILTER];

	can_tx_callback_t tx_callback;
	void *tx_user_data;
	struct k_sem tx_lock_sem;
	struct k_sem tx_done_sem;

	struct can_timing timing;
};

/* Constant device configuration data */
struct can_xlnx_zynq_dev_cfg {
	const struct can_driver_config common;

	DEVICE_MMIO_NAMED_ROM(reg_base);
	can_xlnx_zynq_config_irq_t irq_config_func;
	uint32_t irq;
#ifdef CONFIG_PINCTRL
	const struct pinctrl_dev_config *pin_config;
#endif

	uint32_t clock_frequency;
	uint16_t sjw;
	uint16_t phase_seg1;
	uint16_t phase_seg2;
};

#endif /* _ZEPHYR_DRIVERS_CAN_CAN_XLNX_ZYNQ_H_ */
