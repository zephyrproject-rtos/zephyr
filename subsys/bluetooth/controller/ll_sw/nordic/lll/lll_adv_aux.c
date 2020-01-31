/*
 * Copyright (c) 2018-2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>

#include <toolchain.h>
#include <zephyr/types.h>

#include "hal/ccm.h"
#include "hal/radio.h"

#include "util/util.h"
#include "util/memq.h"

#include "pdu.h"

#include "lll.h"
#include "lll_clock.h"
#include "lll_adv.h"
#include "lll_chan.h"

#include "lll_internal.h"
#include "lll_adv_internal.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_DRIVER)
#define LOG_MODULE_NAME bt_ctlr_lll_adv_aux
#include "common/log.h"
#include <soc.h>
#include "hal/debug.h"

static void isr_done(void *param);
static void isr_cleanup(void *param);
static void isr_race(void *param);

void lll_adv_aux_prepare(struct lll_adv *lll)
{
	struct pdu_adv_com_ext_adv *p;
	struct ext_adv_aux_ptr *aux;
	struct pdu_adv *pri, *sec;
	struct ext_adv_hdr *h;
	u32_t start_us;
	u8_t *ptr;
	u8_t upd = 0U;

	/* AUX_ADV_IND PDU buffer get */
	/* FIXME: get latest only when primary PDU without Aux PDUs */
	sec = lll_adv_aux_data_latest_get(lll, &upd);

	radio_pkt_tx_set(sec);

	/* Get reference to primary PDU aux_ptr */
	pri = lll_adv_data_curr_get(lll);
	p = (void *)&pri->adv_ext_ind;
	h = (void *)p->ext_hdr_adi_adv_data;
	ptr = (u8_t *)h + sizeof(*h);

	/* traverse through adv_addr, if present */
	if (h->adv_addr) {
		ptr += BDADDR_SIZE;
	}

	/* traverse through adi, if present */
	if (h->adi) {
		ptr += sizeof(struct ext_adv_adi);
	}
	aux = (void *)ptr;

	/* Use channel idx in aux_ptr */
	lll_chan_set(aux->chan_idx);

	/* TODO: Based on adv_mode switch to Rx, if needed */
	radio_isr_set(isr_done, lll);
	radio_switch_complete_and_disable();

	start_us = 1000;
	radio_tmr_start_us(1, start_us);

	/* capture end of Tx-ed PDU, used to calculate HCTO. */
	radio_tmr_end_capture();

#if defined(CONFIG_BT_CTLR_GPIO_PA_PIN)
	radio_gpio_pa_setup();
	radio_gpio_pa_lna_enable(start_us +
				 radio_tx_ready_delay_get(0, 0) -
				 CONFIG_BT_CTLR_GPIO_PA_OFFSET);
#endif /* !CONFIG_BT_CTLR_GPIO_PA_PIN */
}

static void isr_done(void *param)
{
	/* TODO: MOVE to a common interface, isr_lll_radio_status? */
	/* Clear radio status and events */
	radio_status_reset();
	radio_tmr_status_reset();
	radio_filter_status_reset();
	radio_ar_status_reset();
	radio_rssi_status_reset();

	if (IS_ENABLED(CONFIG_BT_CTLR_GPIO_PA_PIN) ||
	    IS_ENABLED(CONFIG_BT_CTLR_GPIO_LNA_PIN)) {
		radio_gpio_pa_lna_disable();
	}
	/* TODO: MOVE ^^ */

	isr_cleanup(param);
}

static void isr_cleanup(void *param)
{
	int err;

	radio_isr_set(isr_race, param);
	radio_tmr_stop();

	err = lll_hfclock_off();
	LL_ASSERT(!err || err == -EBUSY);

	lll_done(NULL);
}

static void isr_race(void *param)
{
	/* NOTE: lll_disable could have a race with ... */
	radio_status_reset();
}
