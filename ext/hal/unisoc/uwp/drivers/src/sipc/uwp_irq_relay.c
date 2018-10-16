/*
 * Copyright (c) 2018, UNISOC Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <hal_log.h>
LOG_MODULE_DECLARE(LOG_MODULE_NAME);

#include <zephyr.h>
#include <kernel.h>
#include <device.h>
#include <init.h>
#include <string.h>
#include <uwp_hal.h>
#include <irq_nextlevel.h>

#include "sipc.h"
#include "sipc_priv.h"

#if defined(CONFIG_BT_CTLR_WORKER_PRIO)
#define RADIO_TICKER_USER_ID_WORKER_PRIO CONFIG_BT_CTLR_WORKER_PRIO
#else
#define RADIO_TICKER_USER_ID_WORKER_PRIO 0
#endif

#if defined(CONFIG_BT_CTLR_JOB_PRIO)
#define RADIO_TICKER_USER_ID_JOB_PRIO CONFIG_BT_CTLR_JOB_PRIO
#else
#define RADIO_TICKER_USER_ID_JOB_PRIO 0
#endif

#define CTL_INTC_BASE            0x40000000

#define CEVA_IP_INT_CLEAR_ADDR (0x40246000+0x28)
#define CEVA_IP_int_clear(clear_bit) { int temp = \
	*((volatile u32_t *)CEVA_IP_INT_CLEAR_ADDR); \
	temp |= clear_bit; *((volatile u32_t *)CEVA_IP_INT_CLEAR_ADDR) = temp; }

#define CEVA_IP_int_MASK(bit) {int temp = \
	*((volatile u32_t *)CEVA_IP_INT_CLEAR_ADDR); \
	temp |= bit; *((volatile u32_t *)CEVA_IP_INT_CLEAR_ADDR) = temp; }
#define CEVA_IP_int_UNMASK(bit) {int temp = \
	*((volatile u32_t *)CEVA_IP_INT_CLEAR_ADDR); \
	temp &= ~bit; *((volatile u32_t *)CEVA_IP_INT_CLEAR_ADDR) = temp; }


#define HW_DEC_INT_CLEAR (0x40240000+0x304)
#define HW_DEC_INT1_CLEAR (0x40240000+0x308)

#define HW_DEC_int_clear(clear_bit) \
{unsigned int temp = *((volatile u32_t *)HW_DEC_INT_CLEAR); \
	temp |= clear_bit; \
	*((volatile u32_t *)HW_DEC_INT_CLEAR) = temp; \
	temp &= ~clear_bit; \
	*((volatile u32_t *)HW_DEC_INT_CLEAR) = temp; }

#define HW_DEC_int_clear_sts \
{	unsigned int temp1; \
	unsigned int temp = *((volatile u32_t *)HW_DEC_INT_CLEAR); \
	temp1 = (temp >> 16)&0xff; \
	*((volatile u32_t *)HW_DEC_INT_CLEAR) = temp | temp1; \
	*((volatile u32_t *)HW_DEC_INT_CLEAR) = temp&(~0xff); }

#define HW_DEC_int1_clear_sts \
{	unsigned int temp1; \
	unsigned int temp = *((volatile u32_t *)HW_DEC_INT1_CLEAR); \
	temp1 = (temp >> 16)&0xff; \
	*((volatile u32_t *)HW_DEC_INT1_CLEAR) = temp | temp1; \
	*((volatile u32_t *)HW_DEC_INT1_CLEAR) = temp&(~0xff); }

#define  PKD_INTR_MASK          (1<<4)
#define  AUX_TMR_INTR_MASK      (1<<5)
#define  PKA_INTR_MASK          (1<<6)
#define  PKD_RX_HDR_INTR_MASK   (1<<7)
#define  PKD_NO_PKD_INTR_MASK   (1<<13)
#define  SYNC_DET_INTR_MASK     (1<<14)


#define TIM_INTRO_CLR     (1<<16)
#define TIM_INTR1_CLR     (1<<17)
#define TIM_INTR2_CLR     (1<<18)
#define TIM_INTR3_CLR     (1<<19)
#define AUX_TMR_INTR      (1<<21)
#define PKA_INTR          (1<<22)
#define SYNC_DET_INTR     (1<<30)
#define PKD_RX_HDR        (1<<23)
#define PKD_INTR          (1<<20)
#define PAGE_TIMEOUT_INTR (1<<29)
#define ATOR_INTR0        (1<<0)
#define ATOR_INTR1        (1<<1)
#define ATOR_INTR2        (1<<2)

#define SHARE_MEM_WATCH   (0x1EEF00)
u16_t clear_bt_int(int irq_num)
{
	u16_t tem=0;
	switch (irq_num) {
	case NVIC_BT_MASKED_TIM_INTR0:
		*((u16_t *)(SHARE_MEM_WATCH+0x0c)) += 1;
		tem = *((u16_t *)(SHARE_MEM_WATCH+0x0c));
		CEVA_IP_int_clear(TIM_INTRO_CLR); break;
	case NVIC_BT_MASKED_TIM_INTR1:
		*((u16_t *)(SHARE_MEM_WATCH+0x10)) += 1;
		tem = *((u16_t *)(SHARE_MEM_WATCH+0x10));
		CEVA_IP_int_clear(TIM_INTR1_CLR); break;
	case NVIC_BT_MASKED_TIM_INTR2:
		*((u16_t *)(SHARE_MEM_WATCH+0x14)) += 1;
		tem = *((u16_t *)(SHARE_MEM_WATCH+0x14));
		CEVA_IP_int_clear(TIM_INTR2_CLR); break;
	case NVIC_BT_MASKED_TIM_INTR3:
		*((u16_t *)(SHARE_MEM_WATCH+0x18)) += 1;
		tem = *((u16_t *)(SHARE_MEM_WATCH+0x18));
		CEVA_IP_int_clear(TIM_INTR3_CLR); break;
	case NVIC_BT_MASKED_AUX_TMR_INTR:
		*((u16_t *)(SHARE_MEM_WATCH+0x24)) += 1;
		tem = *((u16_t *)(SHARE_MEM_WATCH+0x24));
		CEVA_IP_int_clear(AUX_TMR_INTR); break;
	case NVIC_BT_MASKED_PKA_INTR:
		*((u16_t *)(SHARE_MEM_WATCH+0x20)) += 1;
		tem = *((u16_t *)(SHARE_MEM_WATCH+0x20));
		CEVA_IP_int_clear(PKA_INTR); break;
	case NVIC_BT_MASKED_SYNC_DET_INTR:
		*((u16_t *)(SHARE_MEM_WATCH+0x04)) += 1;
		tem = *((u16_t *)(SHARE_MEM_WATCH+0x04));
		CEVA_IP_int_clear(SYNC_DET_INTR); break;
	case NVIC_BT_MASKED_PKD_RX_HDR:
		*((u16_t *)(SHARE_MEM_WATCH+0x08)) += 1;
		tem = *((u16_t *)(SHARE_MEM_WATCH+0x08));
		CEVA_IP_int_clear(PKD_RX_HDR); break;
	case NVIC_BT_MASKED_PKD_INTR:
		*((u16_t *)(SHARE_MEM_WATCH+0x1c)) += 1;
		tem =  *((u16_t *)(SHARE_MEM_WATCH+0x1c));
		CEVA_IP_int_clear(PKD_INTR); break;
	case NVIC_BT_MASKED_PAGE_TIMEOUT_INTR:
		*((u16_t *)(SHARE_MEM_WATCH+0)) += 1;
		tem = *((u16_t *)(SHARE_MEM_WATCH+0));
		CEVA_IP_int_MASK(PKD_NO_PKD_INTR_MASK); break;
	case NVIC_BT_ACCELERATOR_INTR0:
		*((u16_t *)(SHARE_MEM_WATCH+0x28)) += 1;
		tem = *((u16_t *)(SHARE_MEM_WATCH+0x28));
		HW_DEC_int_clear(ATOR_INTR0); break;
	case NVIC_BT_ACCELERATOR_INTR1:
		*((u16_t *)(SHARE_MEM_WATCH+0x2c)) += 1;
		tem = *((u16_t *)(SHARE_MEM_WATCH+0x2c));
		HW_DEC_int_clear(ATOR_INTR1); break;
	case NVIC_BT_ACCELERATOR_INTR2:
		*((u16_t *)(SHARE_MEM_WATCH+0x30)) += 1;
		tem = *((u16_t *)(SHARE_MEM_WATCH+0x30));
		HW_DEC_int_clear(ATOR_INTR2); break;
	case NVIC_BT_ACCELERATOR_INTR3:
		*((u16_t *)(SHARE_MEM_WATCH+0x34)) += 1;
		tem = *((u16_t *)(SHARE_MEM_WATCH+0x34));
		HW_DEC_int_clear_sts; break;
	case NVIC_BT_ACCELERATOR_INTR4:
		*((u16_t *)(SHARE_MEM_WATCH+0x38)) += 1;
		tem = *((u16_t *)(SHARE_MEM_WATCH+0x38));
		HW_DEC_int1_clear_sts; break;

    default:
		LOG_INF("bt clear irq error %d\n", irq_num); break;
	}
	if((tem == SMSG_OPEN_MAGIC) || (tem == SMSG_CLOSE_MAGIC)) {
		return 0;
	} else {
		return tem;
	}
}

void sprd_bt_irq_enable(void)
{
	irq_enable(NVIC_BT_MASKED_PAGE_TIMEOUT_INTR);
	irq_enable(NVIC_BT_MASKED_SYNC_DET_INTR);
	irq_enable(NVIC_BT_MASKED_PKD_RX_HDR);
	irq_enable(NVIC_BT_MASKED_TIM_INTR0);
	irq_enable(NVIC_BT_MASKED_TIM_INTR1);
	irq_enable(NVIC_BT_MASKED_TIM_INTR2);
	irq_enable(NVIC_BT_MASKED_TIM_INTR3);
	irq_enable(NVIC_BT_MASKED_PKD_INTR);
	irq_enable(NVIC_BT_MASKED_PKA_INTR);
	irq_enable(NVIC_BT_MASKED_AUX_TMR_INTR);
	irq_enable(NVIC_BT_ACCELERATOR_INTR0);
	irq_enable(NVIC_BT_ACCELERATOR_INTR1);
	irq_enable(NVIC_BT_ACCELERATOR_INTR2);
	irq_enable(NVIC_BT_ACCELERATOR_INTR3);
	irq_enable(NVIC_BT_ACCELERATOR_INTR4);
}

void sprd_bt_irq_disable(void)
{
	irq_disable(NVIC_BT_MASKED_PAGE_TIMEOUT_INTR);
	irq_disable(NVIC_BT_MASKED_SYNC_DET_INTR);
	irq_disable(NVIC_BT_MASKED_PKD_RX_HDR);
	irq_disable(NVIC_BT_MASKED_TIM_INTR0);
	irq_disable(NVIC_BT_MASKED_TIM_INTR1);
	irq_disable(NVIC_BT_MASKED_TIM_INTR2);
	irq_disable(NVIC_BT_MASKED_TIM_INTR3);
	irq_disable(NVIC_BT_MASKED_PKD_INTR);
	irq_disable(NVIC_BT_MASKED_PKA_INTR);
	irq_disable(NVIC_BT_MASKED_AUX_TMR_INTR);
	irq_disable(NVIC_BT_ACCELERATOR_INTR0);
	irq_disable(NVIC_BT_ACCELERATOR_INTR1);
	irq_disable(NVIC_BT_ACCELERATOR_INTR2);
	irq_disable(NVIC_BT_ACCELERATOR_INTR3);
	irq_disable(NVIC_BT_ACCELERATOR_INTR4);

}

static int bt_irq_handler(void *arg)
{
	struct smsg msg;
	s32_t irq = (s32_t)arg;
	u16_t tem;
	tem = clear_bt_int(irq);
	smsg_set(&msg, SMSG_CH_IRQ_DIS, SMSG_TYPE_EVENT, tem, irq);
	smsg_send_irq(SIPC_ID_AP, &msg);
	return 0;
}

int sprd_bt_irq_init(void)
{
	IRQ_CONNECT(NVIC_BT_MASKED_PAGE_TIMEOUT_INTR,
		RADIO_TICKER_USER_ID_WORKER_PRIO, bt_irq_handler,
		NVIC_BT_MASKED_PAGE_TIMEOUT_INTR, 0);
	IRQ_CONNECT(NVIC_BT_MASKED_SYNC_DET_INTR,
		RADIO_TICKER_USER_ID_WORKER_PRIO, bt_irq_handler,
		NVIC_BT_MASKED_SYNC_DET_INTR, 0);
	IRQ_CONNECT(NVIC_BT_MASKED_PKD_RX_HDR,
		RADIO_TICKER_USER_ID_WORKER_PRIO, bt_irq_handler,
		NVIC_BT_MASKED_PKD_RX_HDR, 0);
	IRQ_CONNECT(NVIC_BT_MASKED_TIM_INTR0,
		RADIO_TICKER_USER_ID_WORKER_PRIO, bt_irq_handler,
		NVIC_BT_MASKED_TIM_INTR0, 0);
	IRQ_CONNECT(NVIC_BT_MASKED_TIM_INTR1,
		RADIO_TICKER_USER_ID_WORKER_PRIO, bt_irq_handler,
		NVIC_BT_MASKED_TIM_INTR1, 0);
	IRQ_CONNECT(NVIC_BT_MASKED_TIM_INTR2,
		RADIO_TICKER_USER_ID_WORKER_PRIO, bt_irq_handler,
		NVIC_BT_MASKED_TIM_INTR2, 0);
	IRQ_CONNECT(NVIC_BT_MASKED_TIM_INTR3,
		RADIO_TICKER_USER_ID_WORKER_PRIO, bt_irq_handler,
		NVIC_BT_MASKED_TIM_INTR3, 0);
	IRQ_CONNECT(NVIC_BT_MASKED_PKD_INTR,
		RADIO_TICKER_USER_ID_WORKER_PRIO, bt_irq_handler,
		NVIC_BT_MASKED_PKD_INTR, 0);
	IRQ_CONNECT(NVIC_BT_MASKED_PKA_INTR,
		RADIO_TICKER_USER_ID_WORKER_PRIO, bt_irq_handler,
		NVIC_BT_MASKED_PKA_INTR, 0);
	IRQ_CONNECT(NVIC_BT_MASKED_AUX_TMR_INTR,
		RADIO_TICKER_USER_ID_WORKER_PRIO, bt_irq_handler,
		NVIC_BT_MASKED_AUX_TMR_INTR, 0);
	IRQ_CONNECT(NVIC_BT_ACCELERATOR_INTR0,
		RADIO_TICKER_USER_ID_WORKER_PRIO, bt_irq_handler,
		NVIC_BT_ACCELERATOR_INTR0, 0);
	IRQ_CONNECT(NVIC_BT_ACCELERATOR_INTR1,
		RADIO_TICKER_USER_ID_WORKER_PRIO, bt_irq_handler,
		NVIC_BT_ACCELERATOR_INTR1, 0);
	IRQ_CONNECT(NVIC_BT_ACCELERATOR_INTR2,
		RADIO_TICKER_USER_ID_WORKER_PRIO, bt_irq_handler,
		NVIC_BT_ACCELERATOR_INTR2, 0);
	IRQ_CONNECT(NVIC_BT_ACCELERATOR_INTR3,
		RADIO_TICKER_USER_ID_WORKER_PRIO, bt_irq_handler,
		NVIC_BT_ACCELERATOR_INTR3, 0);
	IRQ_CONNECT(NVIC_BT_ACCELERATOR_INTR4,
		RADIO_TICKER_USER_ID_WORKER_PRIO, bt_irq_handler,
		NVIC_BT_ACCELERATOR_INTR4, 0);

	sprd_bt_irq_enable();

	return 0;
}

void sprd_wifi_irq_enable_num(u32_t num)
{
	LOG_INF("wifi irq enable %d\n", num);

	switch (num) {
	case NVIC_INT_REQ_COM_TMR:
	case NVIC_INT_DPD:
	case NVIC_INT_MAC:
		irq_enable(num);
	break;
	default:
		LOG_WRN("wifi irq enable error num %d\n", num);
	break;
	}

}

void sprd_wifi_irq_disable_num(u32_t num)
{
	LOG_INF("wifi irq enable %d\n", num);
	switch (num) {
	case NVIC_INT_REQ_COM_TMR:
	case NVIC_INT_DPD:
	case NVIC_INT_MAC:
		irq_disable(num);
	break;
	default:
		LOG_WRN("wifi irq disable error num %d\n", num);
	break;
	}

}

static int wifi_int_irq_handler(void *arg)
{
	struct smsg msg;
	s32_t irq = (s32_t)arg;

	smsg_set(&msg, SMSG_CH_IRQ_DIS, SMSG_TYPE_EVENT, 0, irq);
	smsg_send_irq(SIPC_ID_AP, &msg);

	return 0;
}

int wifi_irq_init(void)
{
	IRQ_CONNECT(NVIC_INT_DPD, 5, wifi_int_irq_handler,
		NVIC_INT_DPD, 0);
	IRQ_CONNECT(NVIC_INT_REQ_COM_TMR, 5, wifi_int_irq_handler,
		NVIC_INT_REQ_COM_TMR, 0);
	IRQ_CONNECT(NVIC_INT_MAC, 5, wifi_int_irq_handler,
		NVIC_INT_MAC, 0);

	return 0;
}
