/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/sys/dlist.h>

#include <hal/nrf_ecb.h>

#include "util/mem.h"
#include "hal/ecb.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_DRIVER)
#define LOG_MODULE_NAME bt_ctlr_hal_ecb
#include "common/log.h"
#include "hal/debug.h"

struct ecb_param {
	uint8_t key[16];
	uint8_t clear_text[16];
	uint8_t cipher_text[16];
} __packed;

static void do_ecb(struct ecb_param *ecb)
{
	do {
		nrf_ecb_task_trigger(NRF_ECB, NRF_ECB_TASK_STOPECB);
		NRF_ECB->ECBDATAPTR = (uint32_t)ecb;
		NRF_ECB->EVENTS_ENDECB = 0;
		NRF_ECB->EVENTS_ERRORECB = 0;
		nrf_ecb_task_trigger(NRF_ECB, NRF_ECB_TASK_STARTECB);
		while ((NRF_ECB->EVENTS_ENDECB == 0) &&
		       (NRF_ECB->EVENTS_ERRORECB == 0) &&
		       (NRF_ECB->ECBDATAPTR != 0)) {
#if defined(CONFIG_SOC_SERIES_BSIM_NRFXX)
			k_busy_wait(10);
#else
			/*__WFE();*/
#endif
		}
		nrf_ecb_task_trigger(NRF_ECB, NRF_ECB_TASK_STOPECB);
	} while ((NRF_ECB->EVENTS_ERRORECB != 0) || (NRF_ECB->ECBDATAPTR == 0));

	NRF_ECB->ECBDATAPTR = 0;
}

void ecb_encrypt_be(uint8_t const *const key_be, uint8_t const *const clear_text_be,
		    uint8_t * const cipher_text_be)
{
	struct ecb_param ecb;

	memcpy(&ecb.key[0], key_be, sizeof(ecb.key));
	memcpy(&ecb.clear_text[0], clear_text_be, sizeof(ecb.clear_text));

	do_ecb(&ecb);

	memcpy(cipher_text_be, &ecb.cipher_text[0], sizeof(ecb.cipher_text));
}

void ecb_encrypt(uint8_t const *const key_le, uint8_t const *const clear_text_le,
		 uint8_t * const cipher_text_le, uint8_t * const cipher_text_be)
{
	struct ecb_param ecb;

	mem_rcopy(&ecb.key[0], key_le, sizeof(ecb.key));
	mem_rcopy(&ecb.clear_text[0], clear_text_le, sizeof(ecb.clear_text));

	do_ecb(&ecb);

	if (cipher_text_le) {
		mem_rcopy(cipher_text_le, &ecb.cipher_text[0],
			  sizeof(ecb.cipher_text));
	}

	if (cipher_text_be) {
		memcpy(cipher_text_be, &ecb.cipher_text[0],
			 sizeof(ecb.cipher_text));
	}
}

uint32_t ecb_encrypt_nonblocking(struct ecb *ecb)
{
	/* prepare to be used in a BE AES h/w */
	if (ecb->in_key_le) {
		mem_rcopy(&ecb->in_key_be[0], ecb->in_key_le,
			  sizeof(ecb->in_key_be));
	}
	if (ecb->in_clear_text_le) {
		mem_rcopy(&ecb->in_clear_text_be[0],
			  ecb->in_clear_text_le,
			  sizeof(ecb->in_clear_text_be));
	}

	/* setup the encryption h/w */
	NRF_ECB->ECBDATAPTR = (uint32_t)ecb;
	NRF_ECB->EVENTS_ENDECB = 0;
	NRF_ECB->EVENTS_ERRORECB = 0;
	nrf_ecb_int_enable(NRF_ECB, ECB_INTENSET_ERRORECB_Msk
				    | ECB_INTENSET_ENDECB_Msk);

	/* enable interrupt */
	NVIC_ClearPendingIRQ(ECB_IRQn);
	irq_enable(ECB_IRQn);

	/* start the encryption h/w */
	nrf_ecb_task_trigger(NRF_ECB, NRF_ECB_TASK_STARTECB);

	return 0;
}

static void ecb_cleanup(void)
{
	/* stop h/w */
	NRF_ECB->TASKS_STOPECB = 1;
	nrf_ecb_task_trigger(NRF_ECB, NRF_ECB_TASK_STOPECB);

	/* cleanup interrupt */
	irq_disable(ECB_IRQn);
}

void isr_ecb(void *param)
{
	ARG_UNUSED(param);

	if (NRF_ECB->EVENTS_ERRORECB) {
		struct ecb *ecb = (struct ecb *)NRF_ECB->ECBDATAPTR;

		ecb_cleanup();

		ecb->fp_ecb(1, NULL, ecb->context);
	}

	else if (NRF_ECB->EVENTS_ENDECB) {
		struct ecb *ecb = (struct ecb *)NRF_ECB->ECBDATAPTR;

		ecb_cleanup();

		ecb->fp_ecb(0, &ecb->out_cipher_text_be[0],
			      ecb->context);
	}

	else {
		LL_ASSERT(0);
	}
}

struct ecb_ut_context {
	uint32_t volatile done;
	uint32_t status;
	uint8_t  cipher_text[16];
};

static void ecb_cb(uint32_t status, uint8_t *cipher_be, void *context)
{
	struct ecb_ut_context *ecb_ut_context =
		(struct ecb_ut_context *)context;

	ecb_ut_context->done = 1U;
	ecb_ut_context->status = status;
	if (!status) {
		mem_rcopy(ecb_ut_context->cipher_text, cipher_be,
			  sizeof(ecb_ut_context->cipher_text));
	}
}

uint32_t ecb_ut(void)
{
	uint8_t key[16] = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
			 0x99, 0x00, 0x11, 0x22, 0x33, 0x44, 0x55 };
	uint8_t clear_text[16] = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
				0x88, 0x99, 0x00, 0x11, 0x22, 0x33, 0x44,
				0x55 };
	uint8_t cipher_text[16];
	uint32_t status = 0U;
	struct ecb ecb;
	struct ecb_ut_context context;

	ecb_encrypt(key, clear_text, cipher_text, NULL);

	context.done = 0U;
	ecb.in_key_le = key;
	ecb.in_clear_text_le = clear_text;
	ecb.fp_ecb = ecb_cb;
	ecb.context = &context;
	status = ecb_encrypt_nonblocking(&ecb);
	do {
		__WFE();
		__SEV();
		__WFE();
	} while (!context.done);

	if (context.status != 0U) {
		return context.status;
	}

	status = memcmp(cipher_text, context.cipher_text, sizeof(cipher_text));
	if (status) {
		return status;
	}

	return status;
}
