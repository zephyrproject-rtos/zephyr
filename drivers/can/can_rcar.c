/*
 * Copyright (c) 2021 IoT.bzh
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_rcar_can

#include <zephyr/kernel.h>
#include <errno.h>
#include <zephyr/drivers/can.h>
#include <zephyr/drivers/can/transceiver.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/renesas_cpg_mssr.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>

LOG_MODULE_REGISTER(can_rcar, CONFIG_CAN_LOG_LEVEL);

/* Control Register */
#define RCAR_CAN_CTLR             0x0840
/* Control Register bits */
#define RCAR_CAN_CTLR_BOM         (3 << 11)     /* Bus-Off Recovery Mode Bits */
#define RCAR_CAN_CTLR_BOM_ENT     BIT(11)       /* Automatic halt mode entry at bus-off entry */
#define RCAR_CAN_CTLR_SLPM        BIT(10)
#define RCAR_CAN_CTLR_CANM_HALT   BIT(9)
#define RCAR_CAN_CTLR_CANM_RESET  BIT(8)
#define RCAR_CAN_CTLR_CANM_MASK   (3 << 8)
#define RCAR_CAN_CTLR_MLM         BIT(3)        /* Message Lost Mode Select */
#define RCAR_CAN_CTLR_IDFM        (3 << 1)      /* ID Format Mode Select Bits */
#define RCAR_CAN_CTLR_IDFM_MIXED  BIT(2)        /* Mixed ID mode */
#define RCAR_CAN_CTLR_MBM         BIT(0)        /* Mailbox Mode select */

/* Mask Register */
#define RCAR_CAN_MKR0             0x0430
#define RCAR_CAN_MKR1             0x0434
#define RCAR_CAN_MKR2             0x0400
#define RCAR_CAN_MKR3             0x0404
#define RCAR_CAN_MKR4             0x0408
#define RCAR_CAN_MKR5             0x040C
#define RCAR_CAN_MKR6             0x0410
#define RCAR_CAN_MKR7             0x0414
#define RCAR_CAN_MKR8             0x0418
#define RCAR_CAN_MKR9             0x041C

/* FIFO Received ID Compare Register 0 */
#define RCAR_CAN_FIDCR0           0x0420

/* FIFO Received ID Compare Register 1 */
#define RCAR_CAN_FIDCR1           0x0424

/* FIFO Received ID Compare Registers 0 and 1 bits */
#define RCAR_CAN_FIDCR_IDE        BIT(31)       /* ID Extension Bit */
#define RCAR_CAN_FIDCR_RTR        BIT(30)       /* RTR Bit */

/* Mask Invalid Register 0 */
#define RCAR_CAN_MKIVLR0          0x0438
/* Mask Invalid Register 1 */
#define RCAR_CAN_MKIVLR1          0x0428
/* Mailbox Interrupt Enable Registers*/
#define RCAR_CAN_MIER0            0x043C
#define RCAR_CAN_MIER1            0x042C
#define RCAR_CAN_MIER1_RXFIE      BIT(28)       /* Rx FIFO Interrupt Enable */
#define RCAR_CAN_MIER1_TXFIE      BIT(24)       /* Tx FIFO Interrupt Enable */

#define RCAR_CAN_STR              0x0842        /* Status Register */
#define RCAR_CAN_STR_RSTST        BIT(8)        /* Reset Status Bit */
#define RCAR_CAN_STR_HLTST        BIT(9)        /* Halt Status Bit */
#define RCAR_CAN_STR_SLPST        BIT(10)       /* Sleep Status Bit */
#define MAX_STR_READS             0x100

/* Bit Configuration Register */
#define RCAR_CAN_BCR              0x0844

/* Clock Select Register */
#define RCAR_CAN_CLKR             0x0847
#define RCAR_CAN_CLKR_EXT_CLOCK   0x3         /* External input clock */
#define RCAR_CAN_CLKR_CLKP2       0x1
#define RCAR_CAN_CLKR_CLKP1       0x0

/* Error Interrupt Enable Register */
#define RCAR_CAN_EIER             0x084C

/* Interrupt Enable Register */
#define RCAR_CAN_IER              0x0860
#define RCAR_CAN_IER_ERSIE        BIT(5)        /* Error Interrupt Enable Bit */
#define RCAR_CAN_IER_RXFIE        BIT(4)        /* Rx FIFO Interrupt Enable Bit */
#define RCAR_CAN_IER_TXFIE        BIT(3)        /* Tx FIFO Interrupt Enable Bit */

/* Interrupt Status Register */
#define RCAR_CAN_ISR              0x0861
#define RCAR_CAN_ISR_ERSF         BIT(5)        /* Error (ERS) Interrupt */
#define RCAR_CAN_ISR_RXFF         BIT(4)        /* Reception FIFO Interrupt */
#define RCAR_CAN_ISR_TXFF         BIT(3)        /* Transmission FIFO Interrupt */

/* Receive FIFO Control Register */
#define RCAR_CAN_RFCR             0x0848
#define RCAR_CAN_RFCR_RFE         BIT(0)        /* Receive FIFO Enable */
#define RCAR_CAN_RFCR_RFEST       BIT(7)        /* Receive FIFO Empty Flag */

/* Receive FIFO Pointer Control Register */
#define RCAR_CAN_RFPCR            0x0849

/* Transmit FIFO Control Register */
#define RCAR_CAN_TFCR             0x084A
#define RCAR_CAN_TFCR_TFE         BIT(0)        /* Transmit FIFO Enable */
#define RCAR_CAN_TFCR_TFUST       (7 << 1)      /* Transmit FIFO Unsent Msg Number Status Bits */
#define RCAR_CAN_TFCR_TFUST_SHIFT 1             /* Offset of Tx FIFO Unsent */

/* Transmit FIFO Pointer Control Register */
#define RCAR_CAN_TFPCR            0x084B

/* Error Code Store Register*/
#define RCAR_CAN_ECSR             0x0850        /* Error Code Store Register */
#define RCAR_CAN_ECSR_EDPM        BIT(7)        /* Error Display Mode Select */
#define RCAR_CAN_ECSR_ADEF        BIT(6)        /* ACK Delimiter Error Flag */
#define RCAR_CAN_ECSR_BE0F        BIT(5)        /* Bit Error (dominant) Flag */
#define RCAR_CAN_ECSR_BE1F        BIT(4)        /* Bit Error (recessive) Flag */
#define RCAR_CAN_ECSR_CEF         BIT(3)        /* CRC Error Flag */
#define RCAR_CAN_ECSR_AEF         BIT(2)        /* ACK Error Flag */
#define RCAR_CAN_ECSR_FEF         BIT(1)        /* Form Error Flag */
#define RCAR_CAN_ECSR_SEF         BIT(0)        /* Stuff Error Flag */

/* Test Control Register */
#define RCAR_CAN_TCR              0x0858
#define RCAR_CAN_TCR_TSTE         BIT(0)        /* Test Mode Enable Bit*/
#define RCAR_CAN_TCR_LISTEN_ONLY  BIT(1)
#define RCAR_CAN_TCR_INT_LOOP     (3 << 1)      /* Internal loopback*/

/* Error Interrupt Factor Judge Register bits */
#define RCAR_CAN_EIFR             0x084D
#define RCAR_CAN_EIFR_BLIF        BIT(7)        /* Bus Lock Detect Flag */
#define RCAR_CAN_EIFR_OLIF        BIT(6)        /* Overload Frame Transmission */
#define RCAR_CAN_EIFR_ORIF        BIT(5)        /* Receive Overrun Detect Flag */
#define RCAR_CAN_EIFR_BORIF       BIT(4)        /* Bus-Off Recovery Detect Flag */
#define RCAR_CAN_EIFR_BOEIF       BIT(3)        /* Bus-Off Entry Detect Flag */
#define RCAR_CAN_EIFR_EPIF        BIT(2)        /* Error Passive Detect Flag */
#define RCAR_CAN_EIFR_EWIF        BIT(1)        /* Error Warning Detect Flag */
#define RCAR_CAN_EIFR_BEIF        BIT(0)        /* Bus Error Detect Flag */

/* Receive Error Count Register */
#define RCAR_CAN_RECR             0x084D
/* Transmit Error Count Register */
#define RCAR_CAN_TECR             0x084F

/* Mailbox configuration:
 * mailbox 60 - 63 - Rx FIFO mailboxes
 * mailbox 56 - 59 - Tx FIFO mailboxes
 * non-FIFO mailboxes are not used
 */
#define RCAR_CAN_MB_56            0x0380
#define RCAR_CAN_MB_60            0x03C0
/* DLC must be accessed as a 16 bit register */
#define RCAR_CAN_MB_DLC_OFFSET    0x4           /* Data length code */
#define RCAR_CAN_MB_DATA_OFFSET   0x6           /* Data section */
#define RCAR_CAN_MB_TSH_OFFSET    0x14          /* Timestamp upper byte */
#define RCAR_CAN_MB_TSL_OFFSET    0x15          /* Timestamp lower byte */
#define RCAR_CAN_FIFO_DEPTH       4
#define RCAR_CAN_MB_SID_SHIFT     18
#define RCAR_CAN_MB_RTR           BIT(30)
#define RCAR_CAN_MB_IDE           BIT(31)
#define RCAR_CAN_MB_SID_MASK      0x1FFC0000
#define RCAR_CAN_MB_EID_MASK      0x1FFFFFFF

typedef void (*init_func_t)(const struct device *dev);

struct can_rcar_cfg {
	uint32_t reg_addr;
	int reg_size;
	init_func_t init_func;
	const struct device *clock_dev;
	struct rcar_cpg_clk mod_clk;
	struct rcar_cpg_clk bus_clk;
	uint32_t bus_speed;
	uint8_t sjw;
	uint8_t prop_seg;
	uint8_t phase_seg1;
	uint8_t phase_seg2;
	uint16_t sample_point;
	const struct pinctrl_dev_config *pcfg;
	const struct device *phy;
	uint32_t max_bitrate;
};

struct can_rcar_tx_cb {
	can_tx_callback_t cb;
	void *cb_arg;
};

struct can_rcar_data {
	struct k_mutex inst_mutex;
	struct k_sem tx_sem;
	struct can_rcar_tx_cb tx_cb[RCAR_CAN_FIFO_DEPTH];
	uint8_t tx_head;
	uint8_t tx_tail;
	uint8_t tx_unsent;
	struct k_mutex rx_mutex;
	can_rx_callback_t rx_callback[CONFIG_CAN_RCAR_MAX_FILTER];
	void *rx_callback_arg[CONFIG_CAN_RCAR_MAX_FILTER];
	struct can_filter filter[CONFIG_CAN_RCAR_MAX_FILTER];
	can_state_change_callback_t state_change_cb;
	void *state_change_cb_data;
	enum can_state state;
	bool started;
};

static inline uint16_t can_rcar_read16(const struct can_rcar_cfg *config,
				       uint32_t offs)
{
	return sys_read16(config->reg_addr + offs);
}

static inline void can_rcar_write16(const struct can_rcar_cfg *config,
				    uint32_t offs, uint16_t value)
{
	sys_write16(value, config->reg_addr + offs);
}

static void can_rcar_tx_done(const struct device *dev)
{
	struct can_rcar_data *data = dev->data;
	struct can_rcar_tx_cb *tx_cb;

	tx_cb = &data->tx_cb[data->tx_tail];
	data->tx_tail++;
	if (data->tx_tail >= RCAR_CAN_FIFO_DEPTH) {
		data->tx_tail = 0;
	}

	data->tx_unsent--;
	tx_cb->cb(dev, 0, tx_cb->cb_arg);
	k_sem_give(&data->tx_sem);
}

static void can_rcar_get_error_count(const struct can_rcar_cfg *config,
				     struct can_bus_err_cnt *err_cnt)
{
	err_cnt->tx_err_cnt = sys_read8(config->reg_addr + RCAR_CAN_TECR);
	err_cnt->rx_err_cnt = sys_read8(config->reg_addr + RCAR_CAN_RECR);
}

static void can_rcar_state_change(const struct device *dev, uint32_t newstate)
{
	const struct can_rcar_cfg *config = dev->config;
	struct can_rcar_data *data = dev->data;
	const can_state_change_callback_t cb = data->state_change_cb;
	void *state_change_cb_data = data->state_change_cb_data;
	struct can_bus_err_cnt err_cnt;

	if (data->state == newstate) {
		return;
	}

	LOG_DBG("Can state change new: %u old:%u\n", newstate, data->state);

	data->state = newstate;

	if (cb == NULL) {
		return;
	}
	can_rcar_get_error_count(config, &err_cnt);
	cb(dev, newstate, err_cnt, state_change_cb_data);
}

static void can_rcar_error(const struct device *dev)
{
	const struct can_rcar_cfg *config = dev->config;
	uint8_t eifr, ecsr;

	eifr = sys_read8(config->reg_addr + RCAR_CAN_EIFR);

	if (eifr & RCAR_CAN_EIFR_BEIF) {

		ecsr = sys_read8(config->reg_addr + RCAR_CAN_ECSR);
		if (ecsr & RCAR_CAN_ECSR_ADEF) {
			CAN_STATS_ACK_ERROR_INC(dev);
			sys_write8((uint8_t)~RCAR_CAN_ECSR_ADEF,
				   config->reg_addr + RCAR_CAN_ECSR);
		}
		if (ecsr & RCAR_CAN_ECSR_BE0F) {
			CAN_STATS_BIT0_ERROR_INC(dev);
			sys_write8((uint8_t)~RCAR_CAN_ECSR_BE0F,
				   config->reg_addr + RCAR_CAN_ECSR);
		}
		if (ecsr & RCAR_CAN_ECSR_BE1F) {
			CAN_STATS_BIT1_ERROR_INC(dev);
			sys_write8((uint8_t)~RCAR_CAN_ECSR_BE1F,
				   config->reg_addr + RCAR_CAN_ECSR);
		}
		if (ecsr & RCAR_CAN_ECSR_CEF) {
			CAN_STATS_CRC_ERROR_INC(dev);
			sys_write8((uint8_t)~RCAR_CAN_ECSR_CEF,
				   config->reg_addr + RCAR_CAN_ECSR);
		}
		if (ecsr & RCAR_CAN_ECSR_AEF) {
			CAN_STATS_ACK_ERROR_INC(dev);
			sys_write8((uint8_t)~RCAR_CAN_ECSR_AEF,
				   config->reg_addr + RCAR_CAN_ECSR);
		}
		if (ecsr & RCAR_CAN_ECSR_FEF) {
			CAN_STATS_FORM_ERROR_INC(dev);
			sys_write8((uint8_t)~RCAR_CAN_ECSR_FEF,
				   config->reg_addr + RCAR_CAN_ECSR);
		}
		if (ecsr & RCAR_CAN_ECSR_SEF) {
			CAN_STATS_STUFF_ERROR_INC(dev);
			sys_write8((uint8_t)~RCAR_CAN_ECSR_SEF,
				   config->reg_addr + RCAR_CAN_ECSR);
		}

		sys_write8((uint8_t)~RCAR_CAN_EIFR_BEIF,
			   config->reg_addr + RCAR_CAN_EIFR);
	}
	if (eifr & RCAR_CAN_EIFR_EWIF) {
		LOG_DBG("Error warning interrupt\n");
		/* Clear interrupt condition */
		sys_write8((uint8_t)~RCAR_CAN_EIFR_EWIF,
			   config->reg_addr + RCAR_CAN_EIFR);
		can_rcar_state_change(dev, CAN_STATE_ERROR_WARNING);
	}
	if (eifr & RCAR_CAN_EIFR_EPIF) {
		LOG_DBG("Error passive interrupt\n");
		/* Clear interrupt condition */
		sys_write8((uint8_t)~RCAR_CAN_EIFR_EPIF,
			   config->reg_addr + RCAR_CAN_EIFR);
		can_rcar_state_change(dev, CAN_STATE_ERROR_PASSIVE);
	}
	if (eifr & RCAR_CAN_EIFR_BORIF) {
		LOG_DBG("Bus-off recovery interrupt\n");
		sys_write8(RCAR_CAN_IER_ERSIE, config->reg_addr + RCAR_CAN_IER);
		/* Clear interrupt condition */
		sys_write8((uint8_t)~RCAR_CAN_EIFR_BORIF,
			   config->reg_addr + RCAR_CAN_EIFR);
		can_rcar_state_change(dev, CAN_STATE_BUS_OFF);
	}
	if (eifr & RCAR_CAN_EIFR_BOEIF) {
		LOG_DBG("Bus-off entry interrupt\n");
		sys_write8(RCAR_CAN_IER_ERSIE, config->reg_addr + RCAR_CAN_IER);
		/* Clear interrupt condition */
		sys_write8((uint8_t)~RCAR_CAN_EIFR_BOEIF,
			   config->reg_addr + RCAR_CAN_EIFR);
		can_rcar_state_change(dev, CAN_STATE_BUS_OFF);
	}
	if (eifr & RCAR_CAN_EIFR_ORIF) {
		LOG_DBG("Receive overrun error interrupt\n");
		CAN_STATS_RX_OVERRUN_INC(dev);
		sys_write8((uint8_t)~RCAR_CAN_EIFR_ORIF,
			   config->reg_addr + RCAR_CAN_EIFR);
	}
	if (eifr & RCAR_CAN_EIFR_OLIF) {
		LOG_DBG("Overload Frame Transmission error interrupt\n");
		sys_write8((uint8_t)~RCAR_CAN_EIFR_OLIF,
			   config->reg_addr + RCAR_CAN_EIFR);
	}
	if (eifr & RCAR_CAN_EIFR_BLIF) {
		LOG_DBG("Bus lock detected interrupt\n");
		sys_write8((uint8_t)~RCAR_CAN_EIFR_BLIF,
			   config->reg_addr + RCAR_CAN_EIFR);
	}
}

static void can_rcar_rx_filter_isr(const struct device *dev,
				   struct can_rcar_data *data,
				   const struct can_frame *frame)
{
	struct can_frame tmp_frame;
	uint8_t i;

	for (i = 0; i < CONFIG_CAN_RCAR_MAX_FILTER; i++) {
		if (data->rx_callback[i] == NULL) {
			continue;
		}

		if (!can_frame_matches_filter(frame, &data->filter[i])) {
			continue; /* filter did not match */
		}
		/* Make a temporary copy in case the user
		 * modifies the message.
		 */
		tmp_frame = *frame;
		data->rx_callback[i](dev, &tmp_frame, data->rx_callback_arg[i]);
	}
}

static void can_rcar_rx_isr(const struct device *dev)
{
	const struct can_rcar_cfg *config = dev->config;
	struct can_rcar_data *data = dev->data;
	struct can_frame frame = {0};
	uint32_t val;
	int i;

	val = sys_read32(config->reg_addr + RCAR_CAN_MB_60);
	if (val & RCAR_CAN_MB_IDE) {
		frame.flags |= CAN_FRAME_IDE;
		frame.id = val & RCAR_CAN_MB_EID_MASK;
	} else {
		frame.id = (val & RCAR_CAN_MB_SID_MASK) >> RCAR_CAN_MB_SID_SHIFT;
	}

	if (val & RCAR_CAN_MB_RTR) {
		frame.flags |= CAN_FRAME_RTR;
	}

	frame.dlc = sys_read16(config->reg_addr +
			       RCAR_CAN_MB_60 + RCAR_CAN_MB_DLC_OFFSET) & 0xF;

	/* Be paranoid doc states that any value greater than 8
	 * should be considered as 8 bytes.
	 */
	if (frame.dlc > CAN_MAX_DLC) {
		frame.dlc = CAN_MAX_DLC;
	}

	for (i = 0; i < frame.dlc; i++) {
		frame.data[i] = sys_read8(config->reg_addr +
					  RCAR_CAN_MB_60 + RCAR_CAN_MB_DATA_OFFSET + i);
	}
#if defined(CONFIG_CAN_RX_TIMESTAMP)
	/* read upper byte */
	frame.timestamp = sys_read8(config->reg_addr +
				    RCAR_CAN_MB_60 + RCAR_CAN_MB_TSH_OFFSET) << 8;
	/* and then read lower byte */
	frame.timestamp |= sys_read8(config->reg_addr +
				     RCAR_CAN_MB_60 + RCAR_CAN_MB_TSL_OFFSET);
#endif
	/* Increment CPU side pointer */
	sys_write8(0xff, config->reg_addr + RCAR_CAN_RFPCR);

	can_rcar_rx_filter_isr(dev, data, &frame);
}

static void can_rcar_isr(const struct device *dev)
{
	const struct can_rcar_cfg *config = dev->config;
	struct can_rcar_data *data = dev->data;
	uint8_t isr, unsent;

	isr = sys_read8(config->reg_addr + RCAR_CAN_ISR);
	if (isr & RCAR_CAN_ISR_ERSF) {
		/* Clear the Error interrupt */
		isr &= ~RCAR_CAN_ISR_ERSF;
		sys_write8(isr, config->reg_addr + RCAR_CAN_ISR);
		can_rcar_error(dev);
	}
	if (isr & RCAR_CAN_ISR_TXFF) {
		/* Check for sent messages */
		while (1) {
			unsent = sys_read8(config->reg_addr + RCAR_CAN_TFCR);
			unsent = (unsent & RCAR_CAN_TFCR_TFUST) >>
				 RCAR_CAN_TFCR_TFUST_SHIFT;
			if (data->tx_unsent <= unsent) {
				break;
			}
			can_rcar_tx_done(dev);
		}

		/* Clear the Tx interrupt */
		isr = sys_read8(config->reg_addr + RCAR_CAN_ISR);
		isr &= ~RCAR_CAN_ISR_TXFF;
		sys_write8(isr, config->reg_addr + RCAR_CAN_ISR);
	}
	if (isr & RCAR_CAN_ISR_RXFF) {
		/* while there is unread messages */
		while (!(sys_read8(config->reg_addr + RCAR_CAN_RFCR)
			 & RCAR_CAN_RFCR_RFEST)) {
			can_rcar_rx_isr(dev);
		}
		/* Clear the Rx interrupt */
		isr = sys_read8(config->reg_addr + RCAR_CAN_ISR);
		isr &= ~RCAR_CAN_ISR_RXFF;
		sys_write8(isr, config->reg_addr + RCAR_CAN_ISR);
	}
}

static int can_rcar_leave_sleep_mode(const struct can_rcar_cfg *config)
{
	uint16_t ctlr, str;
	int i;

	ctlr = can_rcar_read16(config, RCAR_CAN_CTLR);
	ctlr &= ~RCAR_CAN_CTLR_SLPM;
	can_rcar_write16(config, RCAR_CAN_CTLR, ctlr);
	for (i = 0; i < MAX_STR_READS; i++) {
		str = can_rcar_read16(config, RCAR_CAN_STR);
		if (!(str & RCAR_CAN_STR_SLPST)) {
			return 0;
		}
	}
	return -EAGAIN;
}

static int can_rcar_enter_reset_mode(const struct can_rcar_cfg *config, bool force)
{
	uint16_t ctlr;
	int i;

	ctlr = can_rcar_read16(config, RCAR_CAN_CTLR);
	ctlr &= ~RCAR_CAN_CTLR_CANM_MASK;
	ctlr |= RCAR_CAN_CTLR_CANM_RESET;
	if (force) {
		ctlr |= RCAR_CAN_CTLR_CANM_HALT;
	}
	can_rcar_write16(config, RCAR_CAN_CTLR, ctlr);
	for (i = 0; i < MAX_STR_READS; i++) {
		if (can_rcar_read16(config, RCAR_CAN_STR) & RCAR_CAN_STR_RSTST) {
			return 0;
		}
	}
	return -EAGAIN;
}

static int can_rcar_enter_halt_mode(const struct can_rcar_cfg *config)
{
	uint16_t ctlr;
	int i;

	ctlr = can_rcar_read16(config, RCAR_CAN_CTLR);
	ctlr &= ~RCAR_CAN_CTLR_CANM_MASK;
	ctlr |= RCAR_CAN_CTLR_CANM_HALT;
	can_rcar_write16(config, RCAR_CAN_CTLR, ctlr);

	/* Wait for controller to apply high bit timing settings */
	k_usleep(1);

	for (i = 0; i < MAX_STR_READS; i++) {
		if (can_rcar_read16(config, RCAR_CAN_STR) & RCAR_CAN_STR_HLTST) {
			return 0;
		}
	}

	return -EAGAIN;
}

static int can_rcar_enter_operation_mode(const struct can_rcar_cfg *config)
{
	uint16_t ctlr, str;
	int i;

	ctlr = can_rcar_read16(config, RCAR_CAN_CTLR);
	ctlr &= ~RCAR_CAN_CTLR_CANM_MASK;
	can_rcar_write16(config, RCAR_CAN_CTLR, ctlr);

	/* Wait for controller to apply high bit timing settings */
	k_usleep(1);

	for (i = 0; i < MAX_STR_READS; i++) {
		str = can_rcar_read16(config, RCAR_CAN_STR);
		if (!(str & RCAR_CAN_CTLR_CANM_MASK)) {
			break;
		}
	}

	if (i == MAX_STR_READS) {
		return -EAGAIN;
	}

	/* Enable Rx and Tx FIFO */
	sys_write8(RCAR_CAN_RFCR_RFE, config->reg_addr + RCAR_CAN_RFCR);
	sys_write8(RCAR_CAN_TFCR_TFE, config->reg_addr + RCAR_CAN_TFCR);

	return 0;
}

static int can_rcar_get_capabilities(const struct device *dev, can_mode_t *cap)
{
	ARG_UNUSED(dev);

	*cap = CAN_MODE_NORMAL | CAN_MODE_LOOPBACK | CAN_MODE_LISTENONLY;

	return 0;
}

static int can_rcar_start(const struct device *dev)
{
	const struct can_rcar_cfg *config = dev->config;
	struct can_rcar_data *data = dev->data;
	int ret;

	if (data->started) {
		return -EALREADY;
	}

	if (config->phy != NULL) {
		ret = can_transceiver_enable(config->phy);
		if (ret != 0) {
			LOG_ERR("failed to enable CAN transceiver (err %d)", ret);
			return ret;
		}
	}

	k_mutex_lock(&data->inst_mutex, K_FOREVER);

	CAN_STATS_RESET(dev);

	ret = can_rcar_enter_operation_mode(config);
	if (ret != 0) {
		LOG_ERR("failed to enter operation mode (err %d)", ret);

		if (config->phy != NULL) {
			/* Attempt to disable the CAN transceiver in case of error */
			(void)can_transceiver_disable(config->phy);
		}
	} else {
		data->started = true;
	}

	k_mutex_unlock(&data->inst_mutex);

	return ret;
}

static int can_rcar_stop(const struct device *dev)
{
	const struct can_rcar_cfg *config = dev->config;
	struct can_rcar_data *data = dev->data;
	int ret;

	if (!data->started) {
		return -EALREADY;
	}

	k_mutex_lock(&data->inst_mutex, K_FOREVER);

	ret = can_rcar_enter_halt_mode(config);
	if (ret != 0) {
		LOG_ERR("failed to enter halt mode (err %d)", ret);
		k_mutex_unlock(&data->inst_mutex);
		return ret;
	}

	data->started = false;

	k_mutex_unlock(&data->inst_mutex);

	if (config->phy != NULL) {
		ret = can_transceiver_disable(config->phy);
		if (ret != 0) {
			LOG_ERR("failed to disable CAN transceiver (err %d)", ret);
			return ret;
		}
	}

	return 0;
}

static int can_rcar_set_mode(const struct device *dev, can_mode_t mode)
{
	const struct can_rcar_cfg *config = dev->config;
	struct can_rcar_data *data = dev->data;
	uint8_t tcr = 0;
	int ret = 0;

	if ((mode & ~(CAN_MODE_LOOPBACK | CAN_MODE_LISTENONLY)) != 0) {
		LOG_ERR("Unsupported mode: 0x%08x", mode);
		return -ENOTSUP;
	}

	if (data->started) {
		return -EBUSY;
	}

	k_mutex_lock(&data->inst_mutex, K_FOREVER);

	if ((mode & (CAN_MODE_LOOPBACK | CAN_MODE_LISTENONLY)) ==
	    (CAN_MODE_LOOPBACK | CAN_MODE_LISTENONLY)) {
		LOG_ERR("Combination of loopback and listenonly modes not supported");
		ret = -ENOTSUP;
		goto unlock;
	} else if ((mode & CAN_MODE_LOOPBACK) != 0) {
		/* Loopback mode */
		tcr = RCAR_CAN_TCR_INT_LOOP | RCAR_CAN_TCR_TSTE;
	} else if ((mode & CAN_MODE_LISTENONLY) != 0) {
		/* Listen-only mode */
		tcr = RCAR_CAN_TCR_LISTEN_ONLY | RCAR_CAN_TCR_TSTE;
	} else {
		/* Normal mode */
		tcr = 0;
	}

	sys_write8(tcr, config->reg_addr + RCAR_CAN_TCR);

unlock:
	k_mutex_unlock(&data->inst_mutex);

	return ret;
}

/* Bit Configuration Register settings */
#define RCAR_CAN_BCR_TSEG1(x)   (((x) & 0x0f) << 20)
#define RCAR_CAN_BCR_BPR(x)     (((x) & 0x3ff) << 8)
#define RCAR_CAN_BCR_SJW(x)     (((x) & 0x3) << 4)
#define RCAR_CAN_BCR_TSEG2(x)   ((x) & 0x07)

static void can_rcar_set_bittiming(const struct can_rcar_cfg *config,
				   const struct can_timing *timing)
{
	uint32_t bcr;

	bcr = RCAR_CAN_BCR_TSEG1(timing->phase_seg1 + timing->prop_seg - 1) |
	      RCAR_CAN_BCR_BPR(timing->prescaler - 1) |
	      RCAR_CAN_BCR_SJW(timing->sjw - 1) |
	      RCAR_CAN_BCR_TSEG2(timing->phase_seg2 - 1);

	/* Don't overwrite CLKR with 32-bit BCR access; CLKR has 8-bit access.
	 * All the registers are big-endian but they get byte-swapped on 32-bit
	 * read/write (but not on 8-bit, contrary to the manuals)...
	 */
	sys_write32((bcr << 8) | RCAR_CAN_CLKR_CLKP2,
		    config->reg_addr + RCAR_CAN_BCR);
}

static int can_rcar_set_timing(const struct device *dev,
			       const struct can_timing *timing)
{
	const struct can_rcar_cfg *config = dev->config;
	struct can_rcar_data *data = dev->data;
	int ret = 0;

	struct reg_backup {
		uint32_t address;
		uint8_t value;
	};

	struct reg_backup regs[3] = { { RCAR_CAN_TCR, 0 }, { RCAR_CAN_TFCR, 0 }
				      , { RCAR_CAN_RFCR, 0 } };

	if (data->started) {
		return -EBUSY;
	}

	k_mutex_lock(&data->inst_mutex, K_FOREVER);

	/* Changing bittiming should be done in reset mode.
	 * Switching to reset mode is resetting loopback mode (TCR),
	 * transmit and receive FIFOs (TFCR and RFCR).
	 * Storing these reg values to restore them once back in halt mode.
	 */
	for (int i = 0; i < 3; i++) {
		regs[i].value = sys_read8(config->reg_addr + regs[i].address);
	}

	/* Switching to reset mode */
	ret = can_rcar_enter_reset_mode(config, true);
	if (ret != 0) {
		goto unlock;
	}

	/* Setting bit timing */
	can_rcar_set_bittiming(config, timing);

	/* Restoring registers must be done in halt mode */
	ret = can_rcar_enter_halt_mode(config);
	if (ret) {
		goto unlock;
	}

	/* Restoring registers */
	for (int i = 0; i < 3; i++) {
		sys_write8(regs[i].value, config->reg_addr + regs[i].address);
	}

unlock:
	k_mutex_unlock(&data->inst_mutex);
	return ret;
}

static void can_rcar_set_state_change_callback(const struct device *dev,
					       can_state_change_callback_t cb,
					       void *user_data)
{
	struct can_rcar_data *data = dev->data;

	data->state_change_cb = cb;
	data->state_change_cb_data = user_data;
}

static int can_rcar_get_state(const struct device *dev, enum can_state *state,
			      struct can_bus_err_cnt *err_cnt)
{
	const struct can_rcar_cfg *config = dev->config;
	struct can_rcar_data *data = dev->data;

	if (state != NULL) {
		if (!data->started) {
			*state = CAN_STATE_STOPPED;
		} else {
			*state = data->state;
		}
	}

	if (err_cnt != NULL) {
		can_rcar_get_error_count(config, err_cnt);
	}

	return 0;
}

#ifndef CONFIG_CAN_AUTO_BUS_OFF_RECOVERY
static int can_rcar_recover(const struct device *dev, k_timeout_t timeout)
{
	const struct can_rcar_cfg *config = dev->config;
	struct can_rcar_data *data = dev->data;
	int64_t start_time;
	int ret;

	if (!data->started) {
		return -ENETDOWN;
	}

	if (data->state != CAN_STATE_BUS_OFF) {
		return 0;
	}

	if (k_mutex_lock(&data->inst_mutex, K_FOREVER)) {
		return -EAGAIN;
	}

	start_time = k_uptime_ticks();
	while (data->state == CAN_STATE_BUS_OFF) {
		ret = can_rcar_enter_operation_mode(config);
		if (ret != 0) {
			goto done;
		}

		if (!K_TIMEOUT_EQ(timeout, K_FOREVER) &&
		    k_uptime_ticks() - start_time >= timeout.ticks) {
			ret = -EAGAIN;
			goto done;
		}
	}

done:
	k_mutex_unlock(&data->inst_mutex);
	return ret;
}
#endif /* CONFIG_CAN_AUTO_BUS_OFF_RECOVERY */

static int can_rcar_send(const struct device *dev, const struct can_frame *frame,
			 k_timeout_t timeout, can_tx_callback_t callback,
			 void *user_data)
{
	const struct can_rcar_cfg *config = dev->config;
	struct can_rcar_data *data = dev->data;
	struct can_rcar_tx_cb *tx_cb;
	uint32_t identifier;
	int i;

	LOG_DBG("Sending %d bytes on %s. "
		"Id: 0x%x, "
		"ID type: %s, "
		"Remote Frame: %s"
		, frame->dlc, dev->name
		, frame->id
		, (frame->flags & CAN_FRAME_IDE) != 0 ?
		"extended" : "standard"
		, (frame->flags & CAN_FRAME_RTR) != 0 ? "yes" : "no");

	__ASSERT_NO_MSG(callback != NULL);
	__ASSERT(frame->dlc == 0U || frame->data != NULL, "Dataptr is null");

	if (frame->dlc > CAN_MAX_DLC) {
		LOG_ERR("DLC of %d exceeds maximum (%d)",
			frame->dlc, CAN_MAX_DLC);
		return -EINVAL;
	}

	if ((frame->flags & ~(CAN_FRAME_IDE | CAN_FRAME_RTR)) != 0) {
		LOG_ERR("unsupported CAN frame flags 0x%02x", frame->flags);
		return -ENOTSUP;
	}

	if (!data->started) {
		return -ENETDOWN;
	}

	/* Wait for a slot into the tx FIFO */
	if (k_sem_take(&data->tx_sem, timeout) != 0) {
		return -EAGAIN;
	}

	k_mutex_lock(&data->inst_mutex, K_FOREVER);
	tx_cb = &data->tx_cb[data->tx_head];
	tx_cb->cb = callback;
	tx_cb->cb_arg = user_data;

	data->tx_head++;
	if (data->tx_head >= RCAR_CAN_FIFO_DEPTH) {
		data->tx_head = 0;
	}

	if ((frame->flags & CAN_FRAME_IDE) != 0) {
		identifier = frame->id | RCAR_CAN_MB_IDE;
	} else {
		identifier = frame->id << RCAR_CAN_MB_SID_SHIFT;
	}

	if ((frame->flags & CAN_FRAME_RTR) != 0) {
		identifier |= RCAR_CAN_MB_RTR;
	}

	sys_write32(identifier, config->reg_addr + RCAR_CAN_MB_56);

	sys_write16(frame->dlc, config->reg_addr
		    + RCAR_CAN_MB_56 + RCAR_CAN_MB_DLC_OFFSET);

	for (i = 0; i < frame->dlc; i++) {
		sys_write8(frame->data[i], config->reg_addr
			   + RCAR_CAN_MB_56 + RCAR_CAN_MB_DATA_OFFSET + i);
	}

	compiler_barrier();
	data->tx_unsent++;
	/* Start Tx: increment the CPU-side pointer for the transmit FIFO
	 * to the next mailbox location
	 */
	sys_write8(0xff, config->reg_addr + RCAR_CAN_TFPCR);

	k_mutex_unlock(&data->inst_mutex);

	return 0;
}

static inline int can_rcar_add_rx_filter_unlocked(const struct device *dev,
						  can_rx_callback_t cb,
						  void *cb_arg,
						  const struct can_filter *filter)
{
	struct can_rcar_data *data = dev->data;
	int i;

	for (i = 0; i < CONFIG_CAN_RCAR_MAX_FILTER; i++) {
		if (data->rx_callback[i] == NULL) {
			data->rx_callback_arg[i] = cb_arg;
			data->filter[i] = *filter;
			compiler_barrier();
			data->rx_callback[i] = cb;
			return i;
		}
	}

	return -ENOSPC;
}

static int can_rcar_add_rx_filter(const struct device *dev, can_rx_callback_t cb,
				  void *cb_arg, const struct can_filter *filter)
{
	struct can_rcar_data *data = dev->data;
	int filter_id;

	if ((filter->flags & ~(CAN_FILTER_IDE | CAN_FILTER_DATA)) != 0) {
		LOG_ERR("unsupported CAN filter flags 0x%02x", filter->flags);
		return -ENOTSUP;
	}

	k_mutex_lock(&data->rx_mutex, K_FOREVER);
	filter_id = can_rcar_add_rx_filter_unlocked(dev, cb, cb_arg, filter);
	k_mutex_unlock(&data->rx_mutex);
	return filter_id;
}

static void can_rcar_remove_rx_filter(const struct device *dev, int filter_id)
{
	struct can_rcar_data *data = dev->data;

	if (filter_id >= CONFIG_CAN_RCAR_MAX_FILTER) {
		return;
	}

	k_mutex_lock(&data->rx_mutex, K_FOREVER);
	compiler_barrier();
	data->rx_callback[filter_id] = NULL;
	k_mutex_unlock(&data->rx_mutex);
}

static int can_rcar_init(const struct device *dev)
{
	const struct can_rcar_cfg *config = dev->config;
	struct can_rcar_data *data = dev->data;
	struct can_timing timing;
	int ret;
	uint16_t ctlr;

	k_mutex_init(&data->inst_mutex);
	k_mutex_init(&data->rx_mutex);
	k_sem_init(&data->tx_sem, RCAR_CAN_FIFO_DEPTH, RCAR_CAN_FIFO_DEPTH);

	data->tx_head = 0;
	data->tx_tail = 0;
	data->tx_unsent = 0;

	memset(data->rx_callback, 0, sizeof(data->rx_callback));
	data->state = CAN_STATE_ERROR_ACTIVE;
	data->state_change_cb = NULL;
	data->state_change_cb_data = NULL;

	if (config->phy != NULL) {
		if (!device_is_ready(config->phy)) {
			LOG_ERR("CAN transceiver not ready");
			return -ENODEV;
		}
	}

	if (!device_is_ready(config->clock_dev)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	/* Configure dt provided device signals when available */
	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	/* reset the registers */
	ret = clock_control_off(config->clock_dev,
				(clock_control_subsys_t)&config->mod_clk);
	if (ret < 0) {
		return ret;
	}

	ret = clock_control_on(config->clock_dev,
			       (clock_control_subsys_t)&config->mod_clk);
	if (ret < 0) {
		return ret;
	}

	ret = clock_control_on(config->clock_dev,
			       (clock_control_subsys_t)&config->bus_clk);
	if (ret < 0) {
		return ret;
	}

	ret = can_rcar_enter_reset_mode(config, false);
	__ASSERT(!ret, "Fail to set CAN controller to reset mode");
	if (ret) {
		return ret;
	}

	ret = can_rcar_leave_sleep_mode(config);
	__ASSERT(!ret, "Fail to leave CAN controller from sleep mode");
	if (ret) {
		return ret;
	}

	timing.sjw = config->sjw;
	if (config->sample_point) {
		ret = can_calc_timing(dev, &timing, config->bus_speed,
				      config->sample_point);
		if (ret == -EINVAL) {
			LOG_ERR("Can't find timing for given param");
			return -EIO;
		}
		LOG_DBG("Presc: %d, TS1: %d, TS2: %d",
			timing.prescaler, timing.phase_seg1, timing.phase_seg2);
		LOG_DBG("Sample-point err : %d", ret);
	} else {
		timing.prop_seg = config->prop_seg;
		timing.phase_seg1 = config->phase_seg1;
		timing.phase_seg2 = config->phase_seg2;
		ret = can_calc_prescaler(dev, &timing, config->bus_speed);
		if (ret) {
			LOG_WRN("Bitrate error: %d", ret);
		}
	}

	ret = can_rcar_set_timing(dev, &timing);
	if (ret) {
		return ret;
	}

	ret = can_rcar_set_mode(dev, CAN_MODE_NORMAL);
	if (ret) {
		return ret;
	}

	ctlr = can_rcar_read16(config, RCAR_CAN_CTLR);
	ctlr |= RCAR_CAN_CTLR_IDFM_MIXED;       /* Select mixed ID mode */
#ifndef CONFIG_CAN_AUTO_BUS_OFF_RECOVERY
	ctlr |= RCAR_CAN_CTLR_BOM_ENT;          /* Entry to halt mode automatically at bus-off */
#endif
	ctlr |= RCAR_CAN_CTLR_MBM;              /* Select FIFO mailbox mode */
	ctlr |= RCAR_CAN_CTLR_MLM;              /* Overrun mode */
	ctlr &= ~RCAR_CAN_CTLR_SLPM;            /* Clear CAN Sleep mode */
	can_rcar_write16(config, RCAR_CAN_CTLR, ctlr);

	/* Accept all SID and EID */
	sys_write32(0, config->reg_addr + RCAR_CAN_MKR8);
	sys_write32(0, config->reg_addr + RCAR_CAN_MKR9);
	/* In FIFO mailbox mode, write "0" to bits 24 to 31 */
	sys_write32(0, config->reg_addr + RCAR_CAN_MKIVLR0);
	sys_write32(0, config->reg_addr + RCAR_CAN_MKIVLR1);
	/* Accept standard and extended ID frames, but not
	 * remote frame.
	 */
	sys_write32(0, config->reg_addr + RCAR_CAN_FIDCR0);
	sys_write32(RCAR_CAN_FIDCR_IDE,
		    config->reg_addr + RCAR_CAN_FIDCR1);

	/* Enable and configure FIFO mailbox interrupts Rx and Tx */
	sys_write32(RCAR_CAN_MIER1_RXFIE | RCAR_CAN_MIER1_TXFIE,
		    config->reg_addr + RCAR_CAN_MIER1);

	sys_write8(RCAR_CAN_IER_ERSIE | RCAR_CAN_IER_RXFIE | RCAR_CAN_IER_TXFIE,
		   config->reg_addr + RCAR_CAN_IER);

	/* Accumulate error codes */
	sys_write8(RCAR_CAN_ECSR_EDPM, config->reg_addr + RCAR_CAN_ECSR);

	/* Enable interrupts for all type of errors */
	sys_write8(0xFF, config->reg_addr + RCAR_CAN_EIER);

	config->init_func(dev);

	return 0;
}

static int can_rcar_get_core_clock(const struct device *dev, uint32_t *rate)
{
	const struct can_rcar_cfg *config = dev->config;

	*rate = config->bus_clk.rate;
	return 0;
}

static int can_rcar_get_max_filters(const struct device *dev, bool ide)
{
	ARG_UNUSED(ide);

	return CONFIG_CAN_RCAR_MAX_FILTER;
}

static int can_rcar_get_max_bitrate(const struct device *dev, uint32_t *max_bitrate)
{
	const struct can_rcar_cfg *config = dev->config;

	*max_bitrate = config->max_bitrate;

	return 0;
}

static const struct can_driver_api can_rcar_driver_api = {
	.get_capabilities = can_rcar_get_capabilities,
	.start = can_rcar_start,
	.stop = can_rcar_stop,
	.set_mode = can_rcar_set_mode,
	.set_timing = can_rcar_set_timing,
	.send = can_rcar_send,
	.add_rx_filter = can_rcar_add_rx_filter,
	.remove_rx_filter = can_rcar_remove_rx_filter,
	.get_state = can_rcar_get_state,
#ifndef CONFIG_CAN_AUTO_BUS_OFF_RECOVERY
	.recover = can_rcar_recover,
#endif
	.set_state_change_callback = can_rcar_set_state_change_callback,
	.get_core_clock = can_rcar_get_core_clock,
	.get_max_filters = can_rcar_get_max_filters,
	.get_max_bitrate = can_rcar_get_max_bitrate,
	.timing_min = {
		.sjw = 0x1,
		.prop_seg = 0x00,
		.phase_seg1 = 0x04,
		.phase_seg2 = 0x02,
		.prescaler = 0x01
	},
	.timing_max = {
		.sjw = 0x4,
		.prop_seg = 0x00,
		.phase_seg1 = 0x10,
		.phase_seg2 = 0x08,
		.prescaler = 0x400
	}
};

/* Device Instantiation */
#define CAN_RCAR_INIT(n)							\
	PINCTRL_DT_INST_DEFINE(n);						\
	static void can_rcar_##n##_init(const struct device *dev);		\
	static const struct can_rcar_cfg can_rcar_cfg_##n = {			\
		.reg_addr = DT_INST_REG_ADDR(n),				\
		.reg_size = DT_INST_REG_SIZE(n),				\
		.init_func = can_rcar_##n##_init,				\
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),		\
		.mod_clk.module =						\
			DT_INST_CLOCKS_CELL_BY_IDX(n, 0, module),		\
		.mod_clk.domain =						\
			DT_INST_CLOCKS_CELL_BY_IDX(n, 0, domain),		\
		.bus_clk.module =						\
			DT_INST_CLOCKS_CELL_BY_IDX(n, 1, module),		\
		.bus_clk.domain =						\
			DT_INST_CLOCKS_CELL_BY_IDX(n, 1, domain),		\
		.bus_clk.rate = 40000000,					\
		.bus_speed = DT_INST_PROP(n, bus_speed),			\
		.sjw = DT_INST_PROP(n, sjw),					\
		.prop_seg = DT_INST_PROP_OR(n, prop_seg, 0),			\
		.phase_seg1 = DT_INST_PROP_OR(n, phase_seg1, 0),		\
		.phase_seg2 = DT_INST_PROP_OR(n, phase_seg2, 0),		\
		.sample_point = DT_INST_PROP_OR(n, sample_point, 0),		\
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),			\
		.phy = DEVICE_DT_GET_OR_NULL(DT_INST_PHANDLE(n, phys)),		\
		.max_bitrate = DT_INST_CAN_TRANSCEIVER_MAX_BITRATE(n, 1000000),	\
	};									\
	static struct can_rcar_data can_rcar_data_##n;				\
										\
	CAN_DEVICE_DT_INST_DEFINE(n, can_rcar_init,				\
				  NULL,						\
				  &can_rcar_data_##n,				\
				  &can_rcar_cfg_##n,				\
				  POST_KERNEL,					\
				  CONFIG_CAN_INIT_PRIORITY,			\
				  &can_rcar_driver_api				\
				  );						\
	static void can_rcar_##n##_init(const struct device *dev)		\
	{									\
		IRQ_CONNECT(DT_INST_IRQN(n),					\
			    0,							\
			    can_rcar_isr,					\
			    DEVICE_DT_INST_GET(n), 0);				\
										\
		irq_enable(DT_INST_IRQN(n));					\
	}

DT_INST_FOREACH_STATUS_OKAY(CAN_RCAR_INIT)
