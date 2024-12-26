/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sw_isr_table.h>
#include <zephyr/irq.h>
#include <kswap.h>
#include <zephyr/tracing/tracing.h>
#include <zephyr/arch/rx/sw_nmi_table.h>

#define NMI_NMIST_MASK  0x01
#define NMI_OSTST_MASK  0x02
#define NMI_IWDTST_MASK 0x08
#define NMI_LVD1ST_MASK 0x10
#define NMI_LVD2ST_MASK 0x20

struct nmi_vector_entry _nmi_vector_table[NMI_TABLE_SIZE] = {
	{(nmi_callback_t)0xFFFFFFFFU, (void *)0xFFFFFFFFU}, /* NMI Pin Interrupt */
	{(nmi_callback_t)0xFFFFFFFFU,
	 (void *)0xFFFFFFFFU}, /* Oscillation Stop Detection Interrupt */
	{(nmi_callback_t)0xFFFFFFFFU, (void *)0xFFFFFFFFU}, /* IWDT Underflow/Refresh Error */
	{(nmi_callback_t)0xFFFFFFFFU, (void *)0xFFFFFFFFU}, /* Voltage Monitoring 1 Interrupt */
	{(nmi_callback_t)0xFFFFFFFFU, (void *)0xFFFFFFFFU}, /* Voltage Monitoring 2 Interrupt */
};

void nmi_enable(uint8_t nmi_vector, nmi_callback_t callback, void *arg)
{
	if (nmi_vector >= NMI_TABLE_SIZE) {
		return;
	}

	_nmi_vector_table[nmi_vector].callback = callback;
	_nmi_vector_table[nmi_vector].arg = arg;

	ICU.NMIER.BIT.IWDTEN = 0x1;
}

int get_nmi_request(void)
{
	uint32_t nmi_status = ICU.NMISR.BYTE;

	if (nmi_status & NMI_NMIST_MASK) {
		return 0;
	} else if (nmi_status & NMI_OSTST_MASK) {
		return 1;
	} else if (nmi_status & NMI_IWDTST_MASK) {
		return 2;
	} else if (nmi_status & NMI_LVD1ST_MASK) {
		return 3;
	} else if (nmi_status & NMI_LVD2ST_MASK) {
		return 4;
	}

	return NMI_TABLE_SIZE;
}

void handle_nmi(uint8_t nmi_vector)
{
	if (nmi_vector >= NMI_TABLE_SIZE) {
		return;
	}

	_nmi_vector_table[nmi_vector].callback(_nmi_vector_table[nmi_vector].arg);

	switch (nmi_vector) {
	case 0:
		ICU.NMICLR.BIT.NMICLR = 0x01;
		break;
	case 1:
		ICU.NMICLR.BIT.OSTCLR = 0x01;
		break;
	case 2:
		ICU.NMICLR.BIT.IWDTCLR = 0x01;
		break;
	case 3:
		ICU.NMICLR.BIT.LVD1CLR = 0x01;
		break;
	case 4:
		ICU.NMICLR.BIT.LVD2CLR = 0x01;
		break;
	default:
		break;
	}
}
