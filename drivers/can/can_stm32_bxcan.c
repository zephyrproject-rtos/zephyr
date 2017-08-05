/*
 * Copyright (c) 2017 Erwin Rol <erwin@erwinrol.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <board.h>
#include <device.h>
#include <kernel.h>
#include <init.h>
#include <can.h>
#include <can/frame_pool.h>

#include <clock_control/stm32_clock_control.h>

#define CAN_TIMEOUT_VALUE 10
#define CONFIG_CAN_STM32_BXCAN_IRQ_PRI 0	// TODO menu config

#define CAN_STM32_BXCAN_MAX_PORT_COUNT		2
#define CAN_STM32_BXCAN_MAX_FIFO_COUNT		2
#define CAN_STM32_BXCAN_FILTER_BANK_COUNT	28

static u8_t can_stm32_bxcan_cb_idx_table[CAN_STM32_BXCAN_FILTER_BANK_COUNT];

struct can_stm32_bxcan_config {
	int			port_nr;
	void			(*config_func)(void);
	struct stm32_pclken	pclken;
};

struct can_stm32_bxcan_data {
	CAN_TypeDef	*can;
	struct device	*clock;

	const struct can_recv_cb *cb_table;
	u32_t		 	  cb_table_size;

	u8_t *fifo_0_cb_idx_table;
	u8_t *fifo_1_cb_idx_table;

	void (*error_cb)(struct device *dev, u32_t error_hint);
};

#define DEV_CFG(dev) \
	((const struct can_stm32_bxcan_config * const)(dev)->config->config_info)
#define DEV_DATA(dev) \
	((struct can_stm32_bxcan_data * const)(dev)->driver_data)

static int can_stm32_bxcan_init(struct device *dev);
static int can_stm32_bxcan_set_mode(struct device *dev, enum can_mode mode);
static int can_stm32_bxcan_set_bitrate(struct device* dev,
		enum can_bitrate bitrate);
static int can_stm32_bxcan_get_error_counters(struct device *dev,
		struct can_error_counters* ec);
static int can_stm32_bxcan_send(struct device *dev, struct can_frame* f);
static int can_stm32_bxcan_register_recv_callbacks(struct device *dev,
		const struct can_recv_cb *cb_table, u32_t cb_table_size);
static int can_stm32_bxcan_register_error_callback(struct device *dev,
		void (*error_cb)(struct device *dev, u32_t error_hint));
static void can_stm32_bxcan_tx_irq(void *arg);
static void can_stm32_bxcan_rx0_irq(void *arg);
static void can_stm32_bxcan_rx1_irq(void *arg);
static void can_stm32_bxcan_sce_irq(void *arg);

static const struct can_driver_api can_stm32_bxcan_drv_api_funcs = {
	.set_mode			= can_stm32_bxcan_set_mode,
	.set_bitrate			= can_stm32_bxcan_set_bitrate,
	.get_error_counters		= can_stm32_bxcan_get_error_counters,
	.send				= can_stm32_bxcan_send,
	.register_recv_callbacks	= can_stm32_bxcan_register_recv_callbacks,
	.register_error_callback	= can_stm32_bxcan_register_error_callback,
};

#ifdef CONFIG_CAN_STM32_BXCAN_PORT_2

static struct device DEVICE_NAME_GET(can_stm32_bxcan_port_2);

static void can_stm32_bxcan_irq_config_port_2(void)
{
	IRQ_CONNECT(CAN2_TX_IRQn, CONFIG_CAN_STM32_BXCAN_IRQ_PRI, can_stm32_bxcan_tx_irq,
			DEVICE_GET(can_stm32_bxcan_port_2), 0);
	irq_enable(CAN2_TX_IRQn);

	IRQ_CONNECT(CAN2_RX0_IRQn, CONFIG_CAN_STM32_BXCAN_IRQ_PRI, can_stm32_bxcan_rx0_irq,
			DEVICE_GET(can_stm32_bxcan_port_2), 0);
	irq_enable(CAN2_RX0_IRQn);

	IRQ_CONNECT(CAN2_RX1_IRQn, CONFIG_CAN_STM32_BXCAN_IRQ_PRI, can_stm32_bxcan_rx1_irq,
			DEVICE_GET(can_stm32_bxcan_port_2), 0);
	irq_enable(CAN2_RX1_IRQn);

	IRQ_CONNECT(CAN2_SCE_IRQn, CONFIG_CAN_STM32_BXCAN_IRQ_PRI, can_stm32_bxcan_sce_irq,
			DEVICE_GET(can_stm32_bxcan_port_2), 0);
	irq_enable(CAN2_SCE_IRQn);
}

static struct can_stm32_bxcan_data can_stm32_bxcan_dev_data_port_2 = {
	.can = CAN2,
	.clock = NULL,
};

static const struct can_stm32_bxcan_config can_stm32_bxcan_dev_cfg_port_2 = {
	.port_nr = 2,
	.config_func = can_stm32_bxcan_irq_config_port_2,
	.pclken = {
		.bus = STM32_CLOCK_BUS_APB1,
		.enr = LL_APB1_GRP1_PERIPH_CAN2
	},
};

DEVICE_AND_API_INIT(can_stm32_bxcan_port_2, CONFIG_CAN_STM32_BXCAN_PORT_2_DEV_NAME,
		    can_stm32_bxcan_init,
		    &can_stm32_bxcan_dev_data_port_2, &can_stm32_bxcan_dev_cfg_port_2,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &can_stm32_bxcan_drv_api_funcs);

#endif /* CONFIG_CAN_STM32_BXCAN_PORT_2 */


#ifdef CONFIG_CAN_STM32_BXCAN_PORT_1

static struct device DEVICE_NAME_GET(can_stm32_bxcan_port_1);

static void can_stm32_bxcan_irq_config_port_1(void)
{
	IRQ_CONNECT(CAN1_TX_IRQn, CONFIG_CAN_STM32_BXCAN_IRQ_PRI, can_stm32_bxcan_tx_irq,
			DEVICE_GET(can_stm32_bxcan_port_1), 0);
	irq_enable(CAN1_TX_IRQn);

	IRQ_CONNECT(CAN1_RX0_IRQn, CONFIG_CAN_STM32_BXCAN_IRQ_PRI, can_stm32_bxcan_rx0_irq,
			DEVICE_GET(can_stm32_bxcan_port_1), 0);
	irq_enable(CAN1_RX0_IRQn);

	IRQ_CONNECT(CAN1_RX1_IRQn, CONFIG_CAN_STM32_BXCAN_IRQ_PRI, can_stm32_bxcan_rx1_irq,
			DEVICE_GET(can_stm32_bxcan_port_1), 0);
	irq_enable(CAN1_RX1_IRQn);

	IRQ_CONNECT(CAN1_SCE_IRQn, CONFIG_CAN_STM32_BXCAN_IRQ_PRI, can_stm32_bxcan_sce_irq,
			DEVICE_GET(can_stm32_bxcan_port_1), 0);
	irq_enable(CAN1_SCE_IRQn);
}

static struct can_stm32_bxcan_data can_stm32_bxcan_dev_data_port_1 = {
	.can = CAN1,
	.clock = NULL,
};

static const struct can_stm32_bxcan_config can_stm32_bxcan_dev_cfg_port_1 = {
	.port_nr = 1,
	.config_func = can_stm32_bxcan_irq_config_port_1,
	.pclken = {
		.bus = STM32_CLOCK_BUS_APB1,
		.enr = LL_APB1_GRP1_PERIPH_CAN1
	},
};

DEVICE_AND_API_INIT(can_stm32_bxcan_port_1, CONFIG_CAN_STM32_BXCAN_PORT_1_DEV_NAME,
		    can_stm32_bxcan_init,
		    &can_stm32_bxcan_dev_data_port_1, &can_stm32_bxcan_dev_cfg_port_1,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &can_stm32_bxcan_drv_api_funcs);


#endif /* CONFIG_CAN_STM32_BXCAN_PORT_1 */


static int __can_stm32_bxcan_set_bitrate(struct device *dev, enum can_bitrate bitrate)
{
	const struct can_stm32_bxcan_config *config = DEV_CFG(dev);
	struct can_stm32_bxcan_data *data = DEV_DATA(dev);

	u32_t prescaler,sjw,bs1,bs2,mode;

	switch(bitrate) {
#ifdef CONFIG_CAN_STM32_BXCAN_BITRATE_1MBIT
	case CAN_BITRATE_1_MBIT:	// 1Mbit/s
		prescaler = CONFIG_CAN_STM32_BXCAN_BITRATE_1MBIT_PRESCALER;
		sjw = CONFIG_CAN_STM32_BXCAN_BITRATE_1MBIT_SJW;
		bs1 = CONFIG_CAN_STM32_BXCAN_BITRATE_1MBIT_BS1;
		bs2 = CONFIG_CAN_STM32_BXCAN_BITRATE_1MBIT_BS2;
		break;
#endif
#ifdef CONFIG_CAN_STM32_BXCAN_BITRATE_800KBIT
	case CAN_BITRATE_800_KBIT:	// 800kbit/s
		prescaler = CONFIG_CAN_STM32_BXCAN_BITRATE_800KBIT_PRESCALER;
		sjw = CONFIG_CAN_STM32_BXCAN_BITRATE_800KBIT_SJW;
		bs1 = CONFIG_CAN_STM32_BXCAN_BITRATE_800KBIT_BS1;
		bs2 = CONFIG_CAN_STM32_BXCAN_BITRATE_800KBIT_BS2;
		break;
#endif
#ifdef CONFIG_CAN_STM32_BXCAN_BITRATE_500KBIT
	case CAN_BITRATE_500_KBIT:	// 500kbit/s
		prescaler = CONFIG_CAN_STM32_BXCAN_BITRATE_500KBIT_PRESCALER;
		sjw = CONFIG_CAN_STM32_BXCAN_BITRATE_500KBIT_SJW;
		bs1 = CONFIG_CAN_STM32_BXCAN_BITRATE_500KBIT_BS1;
		bs2 = CONFIG_CAN_STM32_BXCAN_BITRATE_500KBIT_BS2;
		break;
#endif
#ifdef CONFIG_CAN_STM32_BXCAN_BITRATE_250KBIT
	case CAN_BITRATE_250_KBIT:	// 250kbit/s
		prescaler = CONFIG_CAN_STM32_BXCAN_BITRATE_250KBIT_PRESCALER;
		sjw = CONFIG_CAN_STM32_BXCAN_BITRATE_250KBIT_SJW;
		bs1 = CONFIG_CAN_STM32_BXCAN_BITRATE_250KBIT_BS1;
		bs2 = CONFIG_CAN_STM32_BXCAN_BITRATE_250KBIT_BS2;
		break;
#endif
#ifdef CONFIG_CAN_STM32_BXCAN_BITRATE_125KBIT
	case CAN_BITRATE_125_KBIT:	// 125kbit/s
		prescaler = CONFIG_CAN_STM32_BXCAN_BITRATE_125KBIT_PRESCALER;
		sjw = CONFIG_CAN_STM32_BXCAN_BITRATE_125KBIT_SJW;
		bs1 = CONFIG_CAN_STM32_BXCAN_BITRATE_125KBIT_BS1;
		bs2 = CONFIG_CAN_STM32_BXCAN_BITRATE_125KBIT_BS2;
		break;
#endif
#ifdef CONFIG_CAN_STM32_BXCAN_BITRATE_50KBIT
	case CAN_BITRATE_50_KBIT:	// 50kbit/s
		prescaler = CONFIG_CAN_STM32_BXCAN_BITRATE_50KBIT_PRESCALER;
		sjw = CONFIG_CAN_STM32_BXCAN_BITRATE_50KBIT_SJW;
		bs1 = CONFIG_CAN_STM32_BXCAN_BITRATE_50KBIT_BS1;
		bs2 = CONFIG_CAN_STM32_BXCAN_BITRATE_50KBIT_BS2;
		break;
#endif
#ifdef CONFIG_CAN_STM32_BXCAN_BITRATE_20KBIT
	case CAN_BITRATE_20_KBIT:	// 20kbit/s
		prescaler = CONFIG_CAN_STM32_BXCAN_BITRATE_20KBIT_PRESCALER;
		sjw = CONFIG_CAN_STM32_BXCAN_BITRATE_20KBIT_SJW;
		bs1 = CONFIG_CAN_STM32_BXCAN_BITRATE_20KBIT_BS1;
		bs2 = CONFIG_CAN_STM32_BXCAN_BITRATE_20KBIT_BS2;
		break;
#endif
#ifdef CONFIG_CAN_STM32_BXCAN_BITRATE_10KBIT
	case CAN_BITRATE_10_KBIT:	// 10kbit/s
		prescaler = CONFIG_CAN_STM32_BXCAN_BITRATE_10KBIT_PRESCALER;
		sjw = CONFIG_CAN_STM32_BXCAN_BITRATE_10KBIT_SJW;
		bs1 = CONFIG_CAN_STM32_BXCAN_BITRATE_10KBIT_BS1;
		bs2 = CONFIG_CAN_STM32_BXCAN_BITRATE_10KBIT_BS2;
		break;
#endif
	default:
		printk("Unkwown bit rate %d\n", bitrate);
		return -EINVAL;
	}

	/* Set the bit timing register */
	u32_t btr = 0;

	//btr &= ~CAN_BTR_BRP_Msk;
	btr |= (((prescaler-1) <<  CAN_BTR_BRP_Pos) & CAN_BTR_BRP_Msk);

	//btr &= ~CAN_BTR_TS1_Msk;
	btr |= (((bs1-1) << CAN_BTR_TS1_Pos) & CAN_BTR_TS1_Msk);

	//btr &= ~CAN_BTR_TS2_Msk;
	btr |= (((bs2-1) << CAN_BTR_TS2_Pos) & CAN_BTR_TS2_Msk);

	//btr &= ~CAN_BTR_SJW_Msk;
	btr |= (((sjw-1) << CAN_BTR_SJW_Pos) & CAN_BTR_SJW_Msk);

	printk("__can_stm32_bxcan_set_bitrate %d 0x%08x\n", bitrate, btr);

	data->can->BTR = btr;

	return 0;
}

static void can_stm32_bxcan_tx_irq(void *arg)
{
	uint32_t tmp1 = 0U, tmp2 = 0U, tmp3 = 0U;
	struct device *dev;
	struct can_stm32_bxcan_data *data;

	__ASSERT_NO_MSG(arg != NULL);

	dev = (struct device *)arg;
	data = DEV_DATA(dev);

	__ASSERT_NO_MSG(data != NULL);

	u32_t ier = data->can->IER;

	/* Check End of transmission flag */
	if (ier & CAN_IER_TMEIE) {
		u32_t tsr = data->can->TSR;

		if (tsr & CAN_TXMAILBOX_0) {
			// MB0 empty
		}

		if (tsr & CAN_TXMAILBOX_1) {
			// MB0 empty
		}

		if (tsr & CAN_TXMAILBOX_2) {
			// MB0 empty
		}

		data->can->IER &= ~CAN_IER_TMEIE;
	}
}

static int can_stm32_bxcan_read_rx_fifo(CAN_FIFOMailBox_TypeDef *fifo_mailbox, struct can_frame *f)
{
	u32_t rir = fifo_mailbox->RIR;
	if (rir & 0x04) {
		f->id = ((rir >> 3) & CAN_EFF_MASK) | CAN_EFF_FLAG; // extended 29 bit ID
	} else {
		f->id = (rir >> 21) & CAN_SFF_MASK; // Standard 11 bit ID
	}

	if (rir & 0x02)
		f->id |= CAN_RTR_FLAG;

	u32_t rdtr = fifo_mailbox->RDTR;
	f->dlc = rdtr & 0xF; // DLC 4bit data len
	// u16_t TIME = (rdtr >> 16) & 0xFFFF; // Rx Timestamp

	u32_t rdlr = fifo_mailbox->RDLR;
	f->data[0] = rdlr & 0xFF;
	f->data[1] = (rdlr >> 8U) & 0xFF;
	f->data[2] = (rdlr >> 16U) & 0xFF;
	f->data[3] = (rdlr >> 24U) & 0xFF;

	u32_t rdhr = fifo_mailbox->RDHR;
	f->data[4] = rdhr & 0xFF;
	f->data[5] = (rdhr >> 8U)  & 0xFF;
	f->data[6] = (rdhr >> 16U) & 0xFF;
	f->data[7] = (rdhr >> 24U) & 0xFF;

	return 0;
}

static void can_stm32_bxcan_rx0_irq(void *arg)
{
	struct device *dev;
	struct can_stm32_bxcan_data *data;

	__ASSERT_NO_MSG(arg != NULL);

	dev = (struct device *)arg;
	data = DEV_DATA(dev);

	__ASSERT_NO_MSG(data != NULL);


	u32_t ier = data->can->IER;

	if (ier & CAN_IER_FMPIE0) {
		u32_t rf0r = data->can->RF0R;

		if (rf0r & CAN_RF0R_FOVR0) {
			// overrun FIFO 0
			printk("FIFO 0 Overflow\n");

			data->can->RF0R &= ~CAN_RF0R_FOVR0;
		}

		if (rf0r & CAN_RF0R_FULL0) {
			// FIFO 0 full
			printk("FIFO 0 Full\n");

			data->can->RF0R &= ~CAN_RF0R_FULL0;
		}

		if (rf0r & CAN_RF0R_FMP0) {

			CAN_FIFOMailBox_TypeDef *fifo_mailbox = &(data->can->sFIFOMailBox[0]);
			u32_t rdtr = fifo_mailbox->RDTR;
			u32_t filter_nr = ((rdtr >> 8) & 0xFF);

			int cb_nr = data->fifo_0_cb_idx_table[filter_nr];
			const struct can_recv_cb *cb = &(data->cb_table[cb_nr]);

			if (cb->cb != NULL) {
				struct can_frame *f = NULL;
				if (can_frame_alloc(&f, K_NO_WAIT) < 0) {
					// OOM
				} else {
					can_stm32_bxcan_read_rx_fifo(fifo_mailbox, f);
					cb->cb(dev, f);
				}
			}

			data->can->RF0R |= CAN_RF0R_RFOM0;
		}
	}
}

static void can_stm32_bxcan_rx1_irq(void *arg)
{
	struct device *dev;
	struct can_stm32_bxcan_data *data;

	__ASSERT_NO_MSG(arg != NULL);

	dev = (struct device *)arg;
	data = DEV_DATA(dev);

	__ASSERT_NO_MSG(data != NULL);

	u32_t ier = data->can->IER;

	if (ier & CAN_IER_FMPIE1) {
		u32_t rf1r = data->can->RF1R;

		if (rf1r & CAN_RF1R_FOVR1) {
				// overrun FIFO 1
			printk("FIFO 1 Overflow\n");

			data->can->RF1R &= ~CAN_RF1R_FOVR1;
		}

		if (rf1r & CAN_RF1R_FULL1) {
				// FIFO 1 full
			printk("FIFO 1 Full\n");

			data->can->RF1R &= ~CAN_RF1R_FULL1;
		}

		if (rf1r & CAN_RF1R_FMP1)
		{
			CAN_FIFOMailBox_TypeDef *fifo_mailbox = &(data->can->sFIFOMailBox[1]);
			u32_t rdtr = fifo_mailbox->RDTR;
			u32_t filter_nr = ((rdtr >> 8) & 0xFF);

			int cb_nr = data->fifo_1_cb_idx_table[filter_nr];
			const struct can_recv_cb *cb = &(data->cb_table[cb_nr]);

			if (cb->cb != NULL) {
				struct can_frame *f = NULL;
				if (can_frame_alloc(&f, K_NO_WAIT) < 0) {
					// OOM
				} else {
					can_stm32_bxcan_read_rx_fifo(fifo_mailbox, f);
					cb->cb(dev, f);
				}
			}

			data->can->RF1R |= CAN_RF1R_RFOM1;
		}
	}
}

static void can_stm32_bxcan_sce_irq(void *arg)
{
	uint32_t tmp1 = 0U, tmp2 = 0U, tmp3 = 0U;
	struct device *dev;
	struct can_stm32_bxcan_data *data;

	__ASSERT_NO_MSG(arg != NULL);

	dev = (struct device *)arg;
	data = DEV_DATA(dev);

	__ASSERT_NO_MSG(data != NULL);

	u32_t ier = data->can->IER;


	if (ier & CAN_IT_ERR) {
		u32_t error_hint = 0;
		u32_t esr = data->can->ESR;

		if ((ier & CAN_IT_EWG) && (esr & CAN_FLAG_EWG)) {
			// Error Warning
			error_hint |= CAN_ERROR_WARNING;
		}

		if ((ier & CAN_IT_EPV) && (esr & CAN_FLAG_EPV)) {
			// Error Pasive
			error_hint |= CAN_ERROR_PASSIVE;
		}


		if ((ier & CAN_IT_BOF) && (esr & CAN_FLAG_BOF)) {
			// BOF Error
			error_hint |= CAN_ERROR_BOF;
		}


		if ((ier & CAN_IT_LEC))
		{
			switch(((esr >> 4) & 7))
			{
			case 0: // 000: No Error
				break;
			case 1: // 001: Stuff Error
				error_hint |= CAN_ERROR_STUFF;
				break;
			case 2: // 010: Form Error
				error_hint |= CAN_ERROR_FORM;
				break;
			case 3: // 011: Acknowledgment Error
				error_hint |= CAN_ERROR_ACK;
				break;
			case 4: // 100: Bit recessive Error
				error_hint |= CAN_ERROR_BIT_REC;
				break;
			case 5: // 101: Bit dominant Error
				error_hint |= CAN_ERROR_BIT_DOM;
				break;
			case 6: // 110: CRC Error
				error_hint |= CAN_ERROR_CRC;
				break;
			case 7: // 111: Set by software
				break;
			default:
				break;
			}

			/* Clear Last error code Flag */
			data->can->ESR &= ~(CAN_ESR_LEC);
		}

		if (data->error_cb != NULL)
			data->error_cb(dev, error_hint);

		// Clear ERRI Flag
		data->can->MSR = CAN_MSR_ERRI;
	}
}

static int can_stm32_bxcan_tx_start(struct device *dev, const struct can_frame* f)
{
	const struct can_stm32_bxcan_config *config = DEV_CFG(dev);
	struct can_stm32_bxcan_data *data = DEV_DATA(dev);

	CAN_TxMailBox_TypeDef *tx_mailbox;

	u32_t tsr = data->can->TSR;

	/* Select one empty transmit mailbox */
	if((tsr & CAN_TSR_TME0) == CAN_TSR_TME0)
	{
		tx_mailbox = &(data->can->sTxMailBox[CAN_TXMAILBOX_0]);
	}
	else if((tsr & CAN_TSR_TME1) == CAN_TSR_TME1)
	{
		tx_mailbox = &(data->can->sTxMailBox[CAN_TXMAILBOX_1]);
	}
	else if((tsr & CAN_TSR_TME2) == CAN_TSR_TME2)
	{
		tx_mailbox = &(data->can->sTxMailBox[CAN_TXMAILBOX_2]);
	}
	else
	{
		return -ENOMEM;
	}

	if (f->id & CAN_EFF_FLAG) {
		tx_mailbox->TIR = ((f->id & CAN_EFF_MASK) << 3U) | CAN_TI0R_IDE |
				  ((f->id & CAN_RTR_FLAG) ? CAN_TI0R_RTR : 0);
	} else {
		tx_mailbox->TIR = ((f->id & CAN_SFF_MASK) << 21U) |
				  ((f->id & CAN_RTR_FLAG) ? CAN_TI0R_RTR : 0);
	}

	tx_mailbox->TDTR &= ~CAN_TDT0R_DLC_Msk;
	tx_mailbox->TDTR |= ((f->dlc << CAN_TDT0R_DLC_Pos) & CAN_TDT0R_DLC_Msk);

	tx_mailbox->TDHR = (f->data[7] << CAN_TDH0R_DATA7_Pos) |
			   (f->data[6] << CAN_TDH0R_DATA6_Pos) |
			   (f->data[5] << CAN_TDH0R_DATA5_Pos) |
			   (f->data[4] << CAN_TDH0R_DATA4_Pos);

	tx_mailbox->TDLR = (f->data[3] << CAN_TDL0R_DATA3_Pos) |
			   (f->data[2] << CAN_TDL0R_DATA2_Pos) |
			   (f->data[1] << CAN_TDL0R_DATA1_Pos) |
			   (f->data[0] << CAN_TDL0R_DATA0_Pos);

	data->can->IER |=  CAN_IER_EWGIE | CAN_IER_EPVIE | CAN_IER_BOFIE | CAN_IER_LECIE | CAN_IER_ERRIE | CAN_IER_TMEIE;

	/* Request transmission */
	tx_mailbox->TIR |= CAN_TI0R_TXRQ;

	return 0;
}

static int can_stm32_bxcan_reset_rx_filters(void)
{
	CAN_TypeDef* filter_dev = CAN1; // we must always use CAN1 for filter settings

	/* Initialisation mode for the filter */
	filter_dev->FMR |= CAN_FMR_FINIT;
	filter_dev->FMR &= ~CAN_FMR_CAN2SB;

	filter_dev->FA1R  = 0x00000000;	/* disable all filter banks */
	filter_dev->FM1R  = 0x00000000;	/* clear id/mask mode, default id-mode */
	filter_dev->FFA1R = 0x00000000;	/* clear filter FIFO assignment, default fifo 0*/
	filter_dev->FS1R  = 0x00000000; /* clear scale mode, default 16bit mode */

	for (int i=0; i < CAN_STM32_BXCAN_FILTER_BANK_COUNT;i++) {
		filter_dev->sFilterRegister[i].FR1 = 0;
		filter_dev->sFilterRegister[i].FR2 = 0;
	}

	// Leave the initialisation mode for the filter
	filter_dev->FMR &= ~CAN_FMR_FINIT;

	return 0;
}

static int can_stm32_update_rx_filters(void)
{
	CAN_TypeDef* filter_dev = CAN1; // we must always use CAN1 for filter settings

	struct can_stm32_bxcan_data *data[2];
	data[0] = &can_stm32_bxcan_dev_data_port_1;
	data[1] = &can_stm32_bxcan_dev_data_port_2;

	printk("can_stm32_update_rx_filters\n");

	/* Initialisation mode for the filter */
	filter_dev->FMR |= CAN_FMR_FINIT;

	filter_dev->FMR &= ~CAN_FMR_CAN2SB;

	filter_dev->FA1R  = 0x00000000;	/* disable all filter banks */
	filter_dev->FM1R  = 0x00000000;	/* clear id/mask mode, default mask-mode */
	filter_dev->FFA1R = 0x00000000;	/* clear filter FIFO assignment, default fifo 0*/
	filter_dev->FS1R  = 0x00000000; /* clear scale mode, default 16bit mode */

	int fifo_0_cnt[2];
	int fifo_1_cnt[2];

	int bank_nr = 0;
	for (int p = 0; p < 2; p++) {
		const struct can_recv_cb *cb_table;
		int cb_table_size;

		if (p == 1) {
			// set the switch point for port 2
			printk("port 2 bank start %d\n", bank_nr);
			filter_dev->FMR |= ((bank_nr & 0x1F) << 8);
		}

		fifo_0_cnt[p] = 0;
		fifo_1_cnt[p] = 0;

		for (int cb_nr = 0; cb_nr < data[p]->cb_table_size; cb_nr++) {
			const struct can_recv_cb *cb = &(data[p]->cb_table[cb_nr]);
			const struct can_filter *f = &(cb->filter);
			u32_t bank_bitpos = (1 << bank_nr);
			u32_t fr1 = 0;
			u32_t fr2 = 0;

			if (bank_nr >= CAN_STM32_BXCAN_FILTER_BANK_COUNT) {
				// not enough banks
				goto error_out;
			}

			printk("F P=%d cb_nr=%d bank_nr=%d bitpos=0x%08x\n", p, cb_nr, bank_nr, bank_bitpos);

			if (f->id & CAN_EFF_FLAG) {
				fr1 =  ((f->id & CAN_EFF_MASK) << 3) | 0x4;
				fr2 =  ((f->mask & CAN_EFF_MASK) << 3) | 0x4;
			} else {
				fr1 = (f->id & CAN_SFF_MASK) << 21;
				fr2 = ((f->mask & CAN_SFF_MASK) << 21) | 0x4;
			}

			if (f->id & CAN_RTR_FLAG)
				fr1 |= 0x2;

			if (f->mask & CAN_RTR_FLAG)
				fr2 |= 0x2;

			printk("fr1=0x%08x  fr2=0x%08x\n", fr1, fr2);

			filter_dev->sFilterRegister[bank_nr].FR1 = fr1;
			filter_dev->sFilterRegister[bank_nr].FR2 = fr2;

			filter_dev->FM1R &= ~bank_bitpos; // mask mode
			filter_dev->FS1R |= bank_bitpos; // 32bit scale mode

			// Filter FIFO assignment
			if (cb->fifo == 0) {
				filter_dev->FFA1R &= ~bank_bitpos;// use FIFO 0
				fifo_0_cnt[p]++;
			} else {
				filter_dev->FFA1R |= bank_bitpos;// FIFO 1
				fifo_1_cnt[p]++;
			}

			// Filter activation
			filter_dev->FA1R |= bank_bitpos;

			bank_nr++;
		}
	}

	int offset = 0;
	data[0]->fifo_0_cb_idx_table = &(can_stm32_bxcan_cb_idx_table[offset]);
	offset += fifo_0_cnt[0];
	data[0]->fifo_1_cb_idx_table = &(can_stm32_bxcan_cb_idx_table[offset]);
	offset += fifo_1_cnt[0];
	data[1]->fifo_0_cb_idx_table = &(can_stm32_bxcan_cb_idx_table[offset]);
	offset += fifo_0_cnt[1];
	data[1]->fifo_1_cb_idx_table = &(can_stm32_bxcan_cb_idx_table[offset]);

	for (int p = 0; p < 2; p++) {
		int fifo_0_offset = 0;
		int fifo_1_offset = 0;
		for (int cb_nr = 0; cb_nr < data[p]->cb_table_size; cb_nr++) {
			const struct can_recv_cb *cb = &(data[p]->cb_table[cb_nr]);

			if (cb->fifo == 0) {
				data[p]->fifo_0_cb_idx_table[fifo_0_offset++] = cb_nr;
			} else {
				data[p]->fifo_1_cb_idx_table[fifo_1_offset++] = cb_nr;
			}
		}
	}

	// Leave the initialisation mode for the filter
	filter_dev->FMR &= ~CAN_FMR_FINIT;
	return 0;
error_out:
	can_stm32_bxcan_reset_rx_filters();
	return -1;
}



static int can_stm32_bxcan_enter_sleep_mode(struct device *dev)
{
	const struct can_stm32_bxcan_config *config = DEV_CFG(dev);
	struct can_stm32_bxcan_data *data = DEV_DATA(dev);
	s64_t tickstart;

	data->can->MCR &= ~(CAN_MCR_INRQ | CAN_MCR_SLEEP);

	tickstart =  k_uptime_get();

	while ((data->can->MSR & (CAN_MSR_SLAK|CAN_MSR_INAK)) != CAN_MSR_SLAK) {
		if (k_uptime_delta_32(&tickstart) > CAN_TIMEOUT_VALUE) {
			return -ETIMEDOUT;
		}

		k_yield();
	}

	return 0;
}

static int can_stm32_bxcan_wakeup(struct device *dev)
{
	const struct can_stm32_bxcan_config *config = DEV_CFG(dev);
	struct can_stm32_bxcan_data *data = DEV_DATA(dev);
	s64_t tickstart;

	data->can->MCR &= ~CAN_MCR_SLEEP;

	tickstart =  k_uptime_get();

	while ((data->can->MSR & CAN_MSR_SLAK) != CAN_MSR_SLAK) {
		if (k_uptime_delta_32(&tickstart) > CAN_TIMEOUT_VALUE) {
			return -ETIMEDOUT;
		}

		k_yield();
	}

	return 0;
}

static int can_stm32_bxcan_enter_init_mode(struct device* dev)
{
	const struct can_stm32_bxcan_config *config = DEV_CFG(dev);
	struct can_stm32_bxcan_data *data = DEV_DATA(dev);
	s64_t tickstart;

	printk("can_stm32_bxcan_enter_init_mode\n");

	data->can->MCR &= ~CAN_MCR_SLEEP;

	data->can->MCR |= CAN_MCR_INRQ ;

	tickstart =  k_uptime_get();

	while ((data->can->MSR & CAN_MSR_INAK) != CAN_MSR_INAK) {
		if (k_uptime_delta_32(&tickstart) > CAN_TIMEOUT_VALUE) {
			return -ETIMEDOUT;
		}

		k_yield();
	}

	return 0;
}

static int can_stm32_bxcan_enter_normal_mode(struct device* dev)
{
	const struct can_stm32_bxcan_config *config = DEV_CFG(dev);
	struct can_stm32_bxcan_data *data = DEV_DATA(dev);
	s64_t tickstart;

	printk("can_stm32_bxcan_enter_normal_mode\n");

	data->can->MCR &= ~CAN_MCR_INRQ;

	tickstart =  k_uptime_get();

	while ((data->can->MSR & CAN_MSR_INAK) == CAN_MSR_INAK) {
		if (k_uptime_delta_32(&tickstart) > CAN_TIMEOUT_VALUE) {
			return -ETIMEDOUT;
		}

		k_yield();
	}

	return 0;
}

static int can_stm32_bxcan_init(struct device *dev)
{
	const struct can_stm32_bxcan_config *config = DEV_CFG(dev);
	struct can_stm32_bxcan_data *data = DEV_DATA(dev);
	int res;

	printk("can_stm32_init start %d\n", config->port_nr);

	/* enable clock */
	data->clock = device_get_binding(STM32_CLOCK_CONTROL_NAME);
	__ASSERT_NO_MSG(data->clock);
	clock_control_on(data->clock, (clock_control_subsys_t *)&config->pclken);

	res = can_stm32_bxcan_enter_init_mode(dev);
	if (res < 0) {
		return res;
	}

	res = can_stm32_bxcan_reset_rx_filters();
	if (res < 0) {
		return res;
	}

	// data->can->MCR |= CAN_MCR_TTCM;
	data->can->MCR &= ~CAN_MCR_TTCM;

	/* Set the automatic bus-off management */
	// data->can->MCR |= CAN_MCR_ABOM;
	data->can->MCR &= ~CAN_MCR_ABOM;

	/* Set the automatic wake-up mode */
	//data->can->MCR |= CAN_MCR_AWUM;
	data->can->MCR &= ~CAN_MCR_AWUM;

	/* Set the no automatic retransmission */
	// data->can->MCR |= CAN_MCR_NART;
	data->can->MCR &= ~CAN_MCR_NART;

	/* Set the receive FIFO locked mode */
	// data->can->MCR |= CAN_MCR_RFLM;
	data->can->MCR &= ~CAN_MCR_RFLM;

	/* Set the transmit FIFO priority */
	//data->can->MCR |= CAN_MCR_TXFP;
	data->can->MCR &= ~CAN_MCR_TXFP;


	__ASSERT_NO_MSG(config->config_func != NULL);
	config->config_func();

	data->can->IER |= CAN_IER_EWGIE | CAN_IER_EPVIE | CAN_IER_BOFIE | CAN_IER_LECIE | CAN_IER_ERRIE |
			  CAN_IER_FFIE0 | CAN_IER_FMPIE0 | CAN_IER_FOVIE0 |
			  CAN_IER_FFIE1 | CAN_IER_FMPIE1 | CAN_IER_FOVIE1;

	printk("can_stm32_init done\n");

	return 0;
}

static int can_stm32_bxcan_register_recv_callbacks(struct device *dev,
		const struct can_recv_cb *cb_table, u32_t cb_table_size)
{
	const struct can_stm32_bxcan_config *config = DEV_CFG(dev);
	struct can_stm32_bxcan_data *data = DEV_DATA(dev);

	data->cb_table = cb_table;
	data->cb_table_size = cb_table_size;

	return can_stm32_update_rx_filters();
}

static int can_stm32_bxcan_register_error_callback(struct device *dev,
		void (*error_cb)(struct device *dev, u32_t error_hint))
{
	const struct can_stm32_bxcan_config *config = DEV_CFG(dev);
	struct can_stm32_bxcan_data *data = DEV_DATA(dev);

	data->error_cb = error_cb;

	return 0;
}

static int can_stm32_bxcan_set_mode(struct device *dev, enum can_mode mode)
{
	switch(mode) {
	case CAN_INIT_MODE:
		return can_stm32_bxcan_enter_init_mode(dev);
	case CAN_NORMAL_MODE:
		return can_stm32_bxcan_enter_normal_mode(dev);
	case CAN_SLEEP_MODE:
		return can_stm32_bxcan_enter_sleep_mode(dev);

	case CAN_INT_LOOP_BACK_MODE:
	case CAN_EXT_LOOP_BACK_MODE:
	case CAN_BUS_MONITOR_MODE:
		return -ENOTSUP;
	}

	return -EINVAL;
}

static int can_stm32_bxcan_get_error_counters(struct device *dev, struct can_error_counters* ec)
{
	const struct can_stm32_bxcan_config *config = DEV_CFG(dev);
	struct can_stm32_bxcan_data *data = DEV_DATA(dev);

	u32_t esr = data->can->ESR;

	ec->tec = (esr >> 16) & 0xFF;
	ec->rec = (esr >> 24) & 0xFF;

	return 0;
}

static int can_stm32_bxcan_send(struct device *dev, struct can_frame* f)
{
	int res = can_stm32_bxcan_tx_start(dev, f);
	if (res < 0) {
		// send error
	} else {
		// send OK, release frame
		can_frame_free(f);
	}

	return res;
}

static int can_stm32_bxcan_set_bitrate(struct device* dev, enum can_bitrate bitrate)
{
	int res;

	res = __can_stm32_bxcan_set_bitrate(dev, bitrate);
	if (res < 0) {
		return res;
	}

	return 0;
}
