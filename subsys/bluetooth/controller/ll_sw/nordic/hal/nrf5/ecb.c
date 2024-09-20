/*
 * Copyright (c) 2016-2024 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <string.h>

#include <zephyr/sys/byteorder.h>

#include <hal/nrf_ecb.h>

#include "util/mem.h"

#include "hal/cpu.h"
#include "hal/ecb.h"

#include "hal/debug.h"

#if defined(NRF54L_SERIES)
#define NRF_ECB                   NRF_ECB00
#define ECB_IRQn                  ECB00_IRQn
#define ECB_INTENSET_ERRORECB_Msk ECB_INTENSET_ERROR_Msk
#define ECB_INTENSET_ENDECB_Msk   ECB_INTENSET_END_Msk
#define TASKS_STARTECB            TASKS_START
#define TASKS_STOPECB             TASKS_STOP
#define EVENTS_ENDECB             EVENTS_END
#define EVENTS_ERRORECB           EVENTS_ERROR
#define NRF_ECB_TASK_STARTECB     NRF_ECB_TASK_START
#define NRF_ECB_TASK_STOPECB      NRF_ECB_TASK_STOP
#define ECBDATAPTR                IN.PTR

struct ecb_job_ptr {
	void *ptr;
	struct {
		uint32_t length:24;
		uint32_t attribute:8;
	} __packed;
} __packed;

/* Product Specification recommends a value of 11, but prior work had used 7 */
#define ECB_JOB_PTR_ATTRIBUTE 7U
#endif /* NRF54L_SERIES */

struct ecb_param {
	uint8_t key[16];
	uint8_t clear_text[16];
	uint8_t cipher_text[16];

#if defined(NRF54L_SERIES)
	struct ecb_job_ptr in[2];
	struct ecb_job_ptr out[2];
#endif /* NRF54L_SERIES */
} __packed;

static void do_ecb(struct ecb_param *ep)
{
	do {
		nrf_ecb_task_trigger(NRF_ECB, NRF_ECB_TASK_STOPECB);

#if defined(NRF54L_SERIES)
		NRF_ECB->KEY.VALUE[3] = sys_get_be32(&ep->key[0]);
		NRF_ECB->KEY.VALUE[2] = sys_get_be32(&ep->key[4]);
		NRF_ECB->KEY.VALUE[1] = sys_get_be32(&ep->key[8]);
		NRF_ECB->KEY.VALUE[0] = sys_get_be32(&ep->key[12]);

		ep->in[0].ptr = ep->clear_text;
		ep->in[0].length = sizeof(ep->clear_text);
		ep->in[0].attribute = ECB_JOB_PTR_ATTRIBUTE;
		ep->in[1].ptr = NULL;
		ep->in[1].length = 0U;
		ep->in[1].attribute = 0U;

		ep->out[0].ptr = ep->cipher_text;
		ep->out[0].length = sizeof(ep->cipher_text);
		ep->out[0].attribute = ECB_JOB_PTR_ATTRIBUTE;
		ep->out[1].ptr = NULL;
		ep->out[1].length = 0U;
		ep->out[1].attribute = 0U;

		NRF_ECB->IN.PTR = (uint32_t)ep->in;
		NRF_ECB->OUT.PTR = (uint32_t)ep->out;
#else /* !NRF54L_SERIES */
		NRF_ECB->ECBDATAPTR = (uint32_t)ep;
#endif /* !NRF54L_SERIES */

		NRF_ECB->EVENTS_ENDECB = 0;
		NRF_ECB->EVENTS_ERRORECB = 0;
		nrf_ecb_task_trigger(NRF_ECB, NRF_ECB_TASK_STARTECB);
		while ((NRF_ECB->EVENTS_ENDECB == 0) &&
		       (NRF_ECB->EVENTS_ERRORECB == 0) &&
		       (NRF_ECB->ECBDATAPTR != 0)) {
#if defined(CONFIG_SOC_SERIES_BSIM_NRFXX)
			k_busy_wait(10);
#else
			/* FIXME: use cpu_sleep(), but that will need interrupt
			 *        wake up source and hence necessary appropriate
			 *        code.
			 */
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

void ecb_encrypt_nonblocking(struct ecb *e)
{
	/* prepare to be used in a BE AES h/w */
	if (e->in_key_le) {
		mem_rcopy(&e->in_key_be[0], e->in_key_le, sizeof(e->in_key_be));
	}
	if (e->in_clear_text_le) {
		mem_rcopy(&e->in_clear_text_be[0], e->in_clear_text_le,
			  sizeof(e->in_clear_text_be));
	}

	/* setup the encryption h/w */
#if defined(NRF54L_SERIES)
	NRF_ECB->KEY.VALUE[3] = sys_get_be32(&e->in_key_be[0]);
	NRF_ECB->KEY.VALUE[2] = sys_get_be32(&e->in_key_be[4]);
	NRF_ECB->KEY.VALUE[1] = sys_get_be32(&e->in_key_be[8]);
	NRF_ECB->KEY.VALUE[0] = sys_get_be32(&e->in_key_be[12]);

	struct ecb_job_ptr *in = (void *)((uint8_t *)e + sizeof(*e));
	struct ecb_job_ptr *out = (void *)((uint8_t *)in + 16U);

	in[0].ptr = e->in_clear_text_be;
	in[0].length = sizeof(e->in_clear_text_be);
	in[0].attribute = ECB_JOB_PTR_ATTRIBUTE;
	in[1].ptr = NULL;
	in[1].length = 0U;
	in[1].attribute = 0U;

	out[0].ptr = e->out_cipher_text_be;
	out[0].length = sizeof(e->out_cipher_text_be);
	out[0].attribute = ECB_JOB_PTR_ATTRIBUTE;
	out[1].ptr = NULL;
	out[1].length = 0U;
	out[1].attribute = 0U;

	NRF_ECB->IN.PTR = (uint32_t)in;
	NRF_ECB->OUT.PTR = (uint32_t)out;
#else /* !NRF54L_SERIES */
	NRF_ECB->ECBDATAPTR = (uint32_t)e;
#endif /* !NRF54L_SERIES */
	NRF_ECB->EVENTS_ENDECB = 0;
	NRF_ECB->EVENTS_ERRORECB = 0;
	nrf_ecb_int_enable(NRF_ECB, ECB_INTENSET_ERRORECB_Msk
				    | ECB_INTENSET_ENDECB_Msk);

	/* enable interrupt */
	NVIC_ClearPendingIRQ(ECB_IRQn);
	irq_enable(ECB_IRQn);

	/* start the encryption h/w */
	nrf_ecb_task_trigger(NRF_ECB, NRF_ECB_TASK_STARTECB);
}

static void isr_ecb(const void *arg)
{
#if defined(NRF54L_SERIES)
	struct ecb *e = (void *)((uint8_t *)NRF_ECB->ECBDATAPTR -
				 sizeof(struct ecb));
#else /* !NRF54L_SERIES */
	struct ecb *e = (void *)NRF_ECB->ECBDATAPTR;
#endif /* !NRF54L_SERIES */

	ARG_UNUSED(arg);

	/* Stop ECB h/w */
	nrf_ecb_task_trigger(NRF_ECB, NRF_ECB_TASK_STOPECB);

	/* We are done or encountered error, disable interrupt */
	irq_disable(ECB_IRQn);

	if (NRF_ECB->EVENTS_ERRORECB) {
		NRF_ECB->EVENTS_ERRORECB = 0U;

		e->fp_ecb(1U, NULL, e->context);
	}

	else if (NRF_ECB->EVENTS_ENDECB) {
		NRF_ECB->EVENTS_ENDECB = 0U;

		e->fp_ecb(0U, &e->out_cipher_text_be[0],
			      e->context);
	}

	else {
		LL_ASSERT(false);
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

int ecb_ut(void)
{
	uint8_t key[] = {
		0xbf, 0x01, 0xfb, 0x9d, 0x4e, 0xf3, 0xbc, 0x36,
		0xd8, 0x74, 0xf5, 0x39, 0x41, 0x38, 0x68, 0x4c
	};
	uint8_t clear_text[] = {
		0x13, 0x02, 0xf1, 0xe0, 0xdf, 0xce, 0xbd, 0xac,
		0x79, 0x68, 0x57, 0x46, 0x35, 0x24, 0x13, 0x02
	};
	uint8_t cipher_text_expected[] = {
		0x66, 0xc6, 0xc2, 0x27, 0x8e, 0x3b, 0x8e, 0x05,
		0x3e, 0x7e, 0xa3, 0x26, 0x52, 0x1b, 0xad, 0x99
	};
	uint8_t cipher_text_actual[16];
	int status;

	(void)memset(cipher_text_actual, 0, sizeof(cipher_text_actual));
	ecb_encrypt(key, clear_text, cipher_text_actual, NULL);

	status = memcmp(cipher_text_actual, cipher_text_expected,
			sizeof(cipher_text_actual));
	if (status) {
		return status;
	}

#if defined(CONFIG_BT_CTLR_DYNAMIC_INTERRUPTS)
	irq_connect_dynamic(ECB_IRQn, CONFIG_BT_CTLR_ULL_LOW_PRIO, isr_ecb, NULL, 0);
#else /* !CONFIG_BT_CTLR_DYNAMIC_INTERRUPTS */
	IRQ_CONNECT(ECB_IRQn, CONFIG_BT_CTLR_ULL_LOW_PRIO, isr_ecb, NULL, 0);
#endif /* !CONFIG_BT_CTLR_DYNAMIC_INTERRUPTS */

	uint8_t ecb_mem[sizeof(struct ecb) + 32U];
	struct ecb *e = (void *)ecb_mem;
	struct ecb_ut_context context;

	(void)memset(&context, 0, sizeof(context));
	e->in_key_le = key;
	e->in_clear_text_le = clear_text;
	e->fp_ecb = ecb_cb;
	e->context = &context;
	ecb_encrypt_nonblocking(e);
	do {
#if defined(CONFIG_SOC_SERIES_BSIM_NRFXX)
		k_busy_wait(10);
#else
		cpu_sleep();
#endif
	} while (!context.done);

	if (context.status != 0U) {
		return context.status;
	}

	status = memcmp(cipher_text_expected, context.cipher_text,
			sizeof(cipher_text_expected));
	if (status) {
		return status;
	}

	return status;
}
