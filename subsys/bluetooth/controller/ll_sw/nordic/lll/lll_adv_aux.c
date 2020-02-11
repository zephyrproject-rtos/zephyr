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

void lll_adv_aux_prepare(struct lll_adv *lll)
{
	struct pdu_adv_com_ext_adv *p;
	struct ext_adv_aux_ptr *aux;
	struct pdu_adv *pri, *sec;
	struct ext_adv_hdr *h;
	uint32_t start_us;
	uint8_t *ptr;
	uint8_t upd = 0U;

	/* TODO: if coded we use S8? */
	radio_phy_set(lll->phy_s, 1);
	radio_pkt_configure(8, PDU_AC_PAYLOAD_SIZE_MAX, (lll->phy_s << 1));

	/* AUX_ADV_IND PDU buffer get */
	/* FIXME: get latest only when primary PDU without Aux PDUs */
	sec = lll_adv_aux_data_latest_get(lll, &upd);

	radio_pkt_tx_set(sec);

	/* Get reference to primary PDU aux_ptr */
	pri = lll_adv_data_curr_get(lll);
	p = (void *)&pri->adv_ext_ind;
	h = (void *)p->ext_hdr_adi_adv_data;
	ptr = (uint8_t *)h + sizeof(*h);

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
	radio_isr_set(lll_isr_done, lll);
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
