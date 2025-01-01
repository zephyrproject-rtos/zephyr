/*
 * Copyright (c) 2018 - 2020 Nordic Semiconductor ASA
 * Copyright (c) 2018 Ioannis Glaropoulos
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_BT_CTLR_PHY_CODED)
/* The 2 adjacent TIMER EVENTS_COMPARE event offsets used for implementing
 * SW_SWITCH_TIMER-based auto-switch for TIFS, when receiving in LE Coded PHY.
 *  'index' must be 0 or 1.
 */
#define SW_SWITCH_TIMER_S2_EVTS_COMP(index) \
	(SW_SWITCH_TIMER_EVTS_COMP_S2_BASE + (index))

/* Wire the SW SWITCH TIMER EVENTS_COMPARE[<cc_offset>] event
 * to RADIO TASKS_TXEN/RXEN task.
 */
#define HAL_SW_SWITCH_RADIO_ENABLE_S2_PPI(index) \
	(HAL_SW_SWITCH_RADIO_ENABLE_S2_PPI_BASE + (index))

static inline void hal_radio_sw_switch_coded_config_clear(uint8_t ppi_en,
	uint8_t ppi_dis, uint8_t cc_s2, uint8_t group_index);
#endif

static inline void hal_radio_nrf_ppi_channels_enable(uint32_t mask)
{
	nrf_ppi_channels_enable(NRF_PPI, mask);
}

static inline void hal_radio_nrf_ppi_channels_disable(uint32_t mask)
{
	nrf_ppi_channels_disable(NRF_PPI, mask);
}

/*******************************************************************************
 * Enable Radio on Event Timer tick:
 * wire the EVENT_TIMER EVENTS_COMPARE[0] event to RADIO TASKS_TXEN/RXEN task.
 *
 * Use the pre-programmed PPI channels if possible (if TIMER0 is used as the
 * EVENT_TIMER).
 */
#if (EVENT_TIMER_ID == 0)

static inline void hal_radio_enable_on_tick_ppi_config_and_enable(uint8_t trx)
{
	/* No need to configure anything for the pre-programmed channels.
	 * Just enable and disable them accordingly.
	 */
	if (trx) {
		nrf_ppi_channels_enable(NRF_PPI,
					BIT(HAL_RADIO_ENABLE_TX_ON_TICK_PPI));
	} else {
		nrf_ppi_channels_enable(NRF_PPI,
					BIT(HAL_RADIO_ENABLE_RX_ON_TICK_PPI));
	}
}

#else

static inline void hal_radio_enable_on_tick_ppi_config_and_enable(uint8_t trx)
{
	if (trx) {
		nrf_ppi_channel_endpoint_setup(NRF_PPI,
			HAL_RADIO_ENABLE_TX_ON_TICK_PPI,
			(uint32_t)&(EVENT_TIMER->EVENTS_COMPARE[HAL_EVENT_TIMER_TRX_CC_OFFSET]),
			(uint32_t)&(NRF_RADIO->TASKS_TXEN));

#if defined(CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER)
		NRF_PPI->CHG[SW_SWITCH_SINGLE_TIMER_TASK_GROUP_IDX] =
			BIT(HAL_RADIO_ENABLE_TX_ON_TICK_PPI);

		nrf_ppi_fork_endpoint_setup(NRF_PPI,
			HAL_RADIO_ENABLE_TX_ON_TICK_PPI,
			(uint32_t)&(NRF_PPI->TASKS_CHG[SW_SWITCH_SINGLE_TIMER_TASK_GROUP_IDX].DIS));
#endif /* CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER */

		nrf_ppi_channels_enable(NRF_PPI,
					BIT(HAL_RADIO_ENABLE_TX_ON_TICK_PPI));
	} else {
		nrf_ppi_channel_endpoint_setup(NRF_PPI,
			HAL_RADIO_ENABLE_RX_ON_TICK_PPI,
			(uint32_t)&(EVENT_TIMER->EVENTS_COMPARE[HAL_EVENT_TIMER_TRX_CC_OFFSET]),
			(uint32_t)&(NRF_RADIO->TASKS_RXEN));

#if defined(CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER)
		NRF_PPI->CHG[SW_SWITCH_SINGLE_TIMER_TASK_GROUP_IDX] =
			BIT(HAL_RADIO_ENABLE_RX_ON_TICK_PPI);

		nrf_ppi_fork_endpoint_setup(NRF_PPI,
			HAL_RADIO_ENABLE_RX_ON_TICK_PPI,
			(uint32_t)&(NRF_PPI->TASKS_CHG[SW_SWITCH_SINGLE_TIMER_TASK_GROUP_IDX].DIS));
#endif /* CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER */

		nrf_ppi_channels_enable(NRF_PPI,
					BIT(HAL_RADIO_ENABLE_RX_ON_TICK_PPI));
	}
}

#endif /* (EVENT_TIMER_ID == 0) */

/*******************************************************************************
 * Capture event timer on Address reception:
 * wire the RADIO EVENTS_ADDRESS event to the
 * EVENT_TIMER TASKS_CAPTURE[<address timer>] task.
 *
 * Use the pre-programmed PPI channel if possible (if TIMER0 is used as the
 * EVENT_TIMER).
 */
#if (EVENT_TIMER_ID == 0)

static inline void hal_radio_recv_timeout_cancel_ppi_config(void)
{
	/* No need to configure anything for the pre-programmed channel. */
}

#else

static inline void hal_radio_recv_timeout_cancel_ppi_config(void)
{
	nrf_ppi_channel_endpoint_setup(
		NRF_PPI,
		HAL_RADIO_RECV_TIMEOUT_CANCEL_PPI,
		(uint32_t)&(NRF_RADIO->EVENTS_ADDRESS),
		(uint32_t)&(EVENT_TIMER->TASKS_CAPTURE[HAL_EVENT_TIMER_HCTO_CC_OFFSET]));
}

#endif /* (EVENT_TIMER_ID == 0) */

/*******************************************************************************
 * Disable Radio on HCTO:
 * wire the EVENT_TIMER EVENTS_COMPARE[<HCTO timer>] event
 * to the RADIO TASKS_DISABLE task.
 *
 * Use the pre-programmed PPI channel if possible (if TIMER0 is used as the
 * EVENT_TIMER).
 */
#if (EVENT_TIMER_ID == 0)

static inline void hal_radio_disable_on_hcto_ppi_config(void)
{
	/* No need to configure anything for the pre-programmed channel. */
}

#else

static inline void hal_radio_disable_on_hcto_ppi_config(void)
{
	nrf_ppi_channel_endpoint_setup(
		NRF_PPI,
		HAL_RADIO_DISABLE_ON_HCTO_PPI,
		(uint32_t)&(EVENT_TIMER->EVENTS_COMPARE[HAL_EVENT_TIMER_HCTO_CC_OFFSET]),
		(uint32_t)&(NRF_RADIO->TASKS_DISABLE));
}

#endif /* (EVENT_TIMER_ID == 0) */

/*******************************************************************************
 * Capture event timer on Radio end:
 * wire the RADIO EVENTS_END event to the
 * EVENT_TIMER TASKS_CAPTURE[<radio end timer>] task.
 *
 * Use the pre-programmed PPI channel if possible (if TIMER0 is used as the
 * EVENT_TIMER).
 */
#if (EVENT_TIMER_ID == 0)

static inline void hal_radio_end_time_capture_ppi_config(void)
{
	/* No need to configure anything for the pre-programmed channel. */
}

#else

static inline void hal_radio_end_time_capture_ppi_config(void)
{
	nrf_ppi_channel_endpoint_setup(
		NRF_PPI,
		HAL_RADIO_END_TIME_CAPTURE_PPI,
		(uint32_t)&(NRF_RADIO->HAL_RADIO_TRX_EVENTS_END),
		(uint32_t)&(EVENT_TIMER->TASKS_CAPTURE[HAL_EVENT_TIMER_TRX_END_CC_OFFSET]));
}

#endif /* (EVENT_TIMER_ID == 0) */

/*******************************************************************************
 * Start event timer on RTC tick:
 * wire the RTC0 EVENTS_COMPARE[2] event to EVENT_TIMER  TASKS_START task.
 */
static inline void hal_event_timer_start_ppi_config(void)
{
	nrf_ppi_channel_endpoint_setup(
		NRF_PPI,
		HAL_EVENT_TIMER_START_PPI,
		(uint32_t)&(NRF_RTC0->EVENTS_COMPARE[2]),
		(uint32_t)&(EVENT_TIMER->TASKS_START));
}

/*******************************************************************************
 * Capture event timer on Radio ready:
 * wire the RADIO EVENTS_READY event to the
 * EVENT_TIMER TASKS_CAPTURE[<radio ready timer>] task.
 */
static inline void hal_radio_ready_time_capture_ppi_config(void)
{
	nrf_ppi_channel_endpoint_setup(
		NRF_PPI,
		HAL_RADIO_READY_TIME_CAPTURE_PPI,
		(uint32_t)&(NRF_RADIO->EVENTS_READY),
		(uint32_t)&(EVENT_TIMER->TASKS_CAPTURE[HAL_EVENT_TIMER_TRX_CC_OFFSET]));
}

/*******************************************************************************
 * Trigger encryption task upon address reception:
 * wire the RADIO EVENTS_ADDRESS event to the CCM TASKS_CRYPT task.
 *
 * PPI channel 25 is pre-programmed with the following fixed settings:
 *   EEP: RADIO->EVENTS_ADDRESS
 *   TEP: CCM->TASKS_CRYPT
 */
static inline void hal_trigger_crypt_ppi_config(void)
{
	/* No need to configure anything for the pre-programmed channel. */
}

/*******************************************************************************
 * Disable trigger encryption task
 */
static inline void hal_trigger_crypt_ppi_disable(void)
{
	/* No need to disable anything as ppi channel will be disabled in a
	 * separate disable ppi call by the caller of this function.
	 */
}

#if defined(CONFIG_BT_CTLR_DF_CONN_CTE_RX)
/*******************************************************************************
 * Trigger encryption task on Bit counter match:
 * wire the RADIO EVENTS_BCMATCH event to the CCM TASKS_CRYPT task.
 *
 * PPI channel HAL_TRIGGER_CRYPT_DELAY_PPI is also used for HAL_TRIGGER-
 * _RATEOVERRIDE_PPI.
 * Make sure the same PPI is not configured for both events at once.
 *
 *   EEP: RADIO->EVENTS_BCMATCH
 *   TEP: CCM->TASKS_CRYPT
 */
static inline void hal_trigger_crypt_by_bcmatch_ppi_config(void)
{
	/* Configure Bit counter to trigger EVENTS_BCMATCH for CCM_TASKS_CRYPT-
	 * _DELAY_BITS bit. This is a time required for Radio to store
	 * received data in memory before the CCM TASKS_CRYPT starts. This
	 * makes CCM to do not read the memory before Radio stores received
	 * data.
	 */
	nrf_ppi_channel_endpoint_setup(NRF_PPI, HAL_TRIGGER_CRYPT_DELAY_PPI,
				       (uint32_t)&(NRF_RADIO->EVENTS_BCMATCH),
				       (uint32_t)&(NRF_CCM->TASKS_CRYPT));
}
#endif /* CONFIG_BT_CTLR_DF_CONN_CTE_RX */

/*******************************************************************************
 * Trigger automatic address resolution on Bit counter match:
 * wire the RADIO EVENTS_BCMATCH event to the AAR TASKS_START task.
 *
 * PPI channel 23 is pre-programmed with the following fixed settings:
 *   EEP: RADIO->EVENTS_BCMATCH
 *   TEP: AAR->TASKS_START
 */
static inline void hal_trigger_aar_ppi_config(void)
{
	/* No need to configure anything for the pre-programmed channel. */
}

/*******************************************************************************
 * Trigger Radio Rate override upon Rateboost event.
 */
#if defined(CONFIG_BT_CTLR_PHY_CODED) && defined(CONFIG_HAS_HW_NRF_RADIO_BLE_CODED)
static inline void hal_trigger_rateoverride_ppi_config(void)
{
	nrf_ppi_channel_endpoint_setup(
		NRF_PPI,
		HAL_TRIGGER_RATEOVERRIDE_PPI,
		(uint32_t)&(NRF_RADIO->EVENTS_RATEBOOST),
		(uint32_t)&(NRF_CCM->TASKS_RATEOVERRIDE));
}
#endif /* CONFIG_BT_CTLR_PHY_CODED && CONFIG_HAS_HW_NRF_RADIO_BLE_CODED */

/******************************************************************************/
#if !defined(CONFIG_BT_CTLR_TIFS_HW)
/* PPI setup used for SW-based auto-switching during TIFS. */

#if !defined(CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER)

/* Clear SW-switch timer on packet end:
 * wire the RADIO EVENTS_END event to SW_SWITCH_TIMER TASKS_CLEAR task.
 *
 * Note: this PPI is not needed if we use a single TIMER instance in radio.c
 */

static inline void hal_sw_switch_timer_clear_ppi_config(void)
{
	nrf_ppi_channel_endpoint_setup(
		NRF_PPI,
		HAL_SW_SWITCH_TIMER_CLEAR_PPI,
		(uint32_t)&(NRF_RADIO->HAL_RADIO_TRX_EVENTS_END),
		(uint32_t)&(SW_SWITCH_TIMER->TASKS_CLEAR));
}

#else /* !CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER */

/* Clear event timer (sw-switch timer) on Radio end:
 * wire the RADIO EVENTS_END event to the
 * EVENT_TIMER TASKS_CLEAR task.
 *
 * Note: in nRF52X this PPI channel is forked for both capturing and clearing
 * timer on RADIO EVENTS_END.
 */

static inline void hal_sw_switch_timer_clear_ppi_config(void)
{
	nrf_ppi_fork_endpoint_setup(
		NRF_PPI,
		HAL_RADIO_END_TIME_CAPTURE_PPI,
		(uint32_t)&(SW_SWITCH_TIMER->TASKS_CLEAR));
}

#endif /* !CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER */

/* The 2 adjacent PPI groups used for implementing SW_SWITCH_TIMER-based
 * auto-switch for TIFS. 'index' must be 0 or 1.
 */
#define SW_SWITCH_TIMER_TASK_GROUP(index) \
	(SW_SWITCH_TIMER_TASK_GROUP_BASE + (index))

/* The 2 adjacent TIMER EVENTS_COMPARE event offsets used for implementing
 * SW_SWITCH_TIMER-based auto-switch for TIFS. 'index' must be 0 or 1.
 */
#define SW_SWITCH_TIMER_EVTS_COMP(index) \
	(SW_SWITCH_TIMER_EVTS_COMP_BASE + (index))

/* Wire a SW SWITCH TIMER EVENTS_COMPARE[<cc_offset>] event
 * to a PPI GROUP TASK DISABLE task (PPI group with index <index>).
 * 2 adjacent PPIs (9 & 10) and 2 adjacent PPI groups are used for this wiring;
 * <index> must be 0 or 1. <offset> must be a valid TIMER CC register offset.
 */
/* Because nRF52805 has limited number of programmable PPI channels,
 * tIFS Trx SW switching on this SoC can be used only when pre-programmed
 * PPI channels are also in use, i.e. when TIMER0 is the event timer.
 */
#if (EVENT_TIMER_ID != 0) && defined(CONFIG_SOC_NRF52805)
#error "tIFS Trx SW switch can be used on this SoC only with TIMER0 as the event timer"
#endif

#define HAL_SW_SWITCH_GROUP_TASK_DISABLE_PPI(index) \
	(HAL_SW_SWITCH_GROUP_TASK_DISABLE_PPI_BASE + (index))
#define HAL_SW_SWITCH_GROUP_TASK_DISABLE_PPI_EVT(cc_offset) \
	((uint32_t)&(SW_SWITCH_TIMER->EVENTS_COMPARE[cc_offset]))
#define HAL_SW_SWITCH_GROUP_TASK_DISABLE_PPI_TASK(index) \
	((uint32_t)&(NRF_PPI->TASKS_CHG[SW_SWITCH_TIMER_TASK_GROUP(index)].DIS))

/* Wire the RADIO EVENTS_END or EVENTS_PHYEND event to one of the PPI GROUP
 * TASK ENABLE task. 2 adjacent PPI groups are used for this wiring.
 * 'index' must be 0 or 1.
 */
#define HAL_SW_SWITCH_GROUP_TASK_ENABLE_PPI_EVT \
	((uint32_t)&(NRF_RADIO->HAL_RADIO_TRX_EVENTS_END))
#define HAL_SW_SWITCH_GROUP_TASK_ENABLE_PPI_TASK(index) \
	((uint32_t)&(NRF_PPI->TASKS_CHG[SW_SWITCH_TIMER_TASK_GROUP(index)].EN))

/*Enable Radio at specific time-stamp:
 * wire the SW SWITCH TIMER EVENTS_COMPARE[<cc_offset>] event
 * to RADIO TASKS_TXEN/RXEN task.
 * 2 adjacent PPIs (12 & 13) are used for this wiring; <index> must be 0 or 1.
 * <offset> must be a valid TIMER CC register offset.
 */
#define HAL_SW_SWITCH_RADIO_ENABLE_PPI(index) \
	(HAL_SW_SWITCH_RADIO_ENABLE_PPI_BASE + (index))
#define HAL_SW_SWITCH_RADIO_ENABLE_PPI_EVT(cc_offset) \
	((uint32_t)&(SW_SWITCH_TIMER->EVENTS_COMPARE[cc_offset]))
#define HAL_SW_SWITCH_RADIO_ENABLE_PPI_TASK_TX \
	((uint32_t)&(NRF_RADIO->TASKS_TXEN))
#define HAL_SW_SWITCH_RADIO_ENABLE_PPI_TASK_RX \
	((uint32_t)&(NRF_RADIO->TASKS_RXEN))

static inline void hal_radio_sw_switch_setup(uint8_t ppi_group_index)
{
	/* Set up software switch mechanism for next Radio switch. */

	/* Wire RADIO END event to PPI Group[<index>] enable task,
	 * over PPI[<HAL_SW_SWITCH_GROUP_TASK_ENABLE_PPI>]
	 */
	nrf_ppi_channel_endpoint_setup(
		NRF_PPI,
		HAL_SW_SWITCH_GROUP_TASK_ENABLE_PPI,
		HAL_SW_SWITCH_GROUP_TASK_ENABLE_PPI_EVT,
		HAL_SW_SWITCH_GROUP_TASK_ENABLE_PPI_TASK(ppi_group_index));
}

static inline void hal_radio_txen_on_sw_switch(uint8_t compare_reg_index, uint8_t radio_enable_ppi)
{
	/* Wire SW Switch timer event <compare_reg> to the
	 * PPI[<radio_enable_ppi>] for enabling Radio. Do
	 * not wire the task; it is done by the caller of
	 * the function depending on the desired direction
	 * (TX/RX).
	 */
	nrf_ppi_event_endpoint_setup(NRF_PPI, radio_enable_ppi,
				     HAL_SW_SWITCH_RADIO_ENABLE_PPI_EVT(compare_reg_index));

	nrf_ppi_task_endpoint_setup(NRF_PPI, radio_enable_ppi,
				    HAL_SW_SWITCH_RADIO_ENABLE_PPI_TASK_TX);
}

static inline void hal_radio_b2b_txen_on_sw_switch(uint8_t compare_reg_index,
						   uint8_t radio_enable_ppi)
{
	/* NOTE: As independent PPI are used to trigger the Radio Tx task,
	 *       double buffers implementation works for sw_switch using PPIs,
	 *       simply reuse the hal_radio_txen_on_sw_switch() function to set
	 *	 the next PPIs task to be Radio Tx enable.
	 */
	hal_radio_txen_on_sw_switch(compare_reg_index, radio_enable_ppi);
}

static inline void hal_radio_rxen_on_sw_switch(uint8_t compare_reg_index, uint8_t radio_enable_ppi)
{
	/* Wire SW Switch timer event <compare_reg> to the
	 * PPI[<radio_enable_ppi>] for enabling Radio. Do
	 * not wire the task; it is done by the caller of
	 * the function depending on the desired direction
	 * (TX/RX).
	 */
	nrf_ppi_event_endpoint_setup(NRF_PPI, radio_enable_ppi,
				     HAL_SW_SWITCH_RADIO_ENABLE_PPI_EVT(compare_reg_index));

	nrf_ppi_task_endpoint_setup(NRF_PPI, radio_enable_ppi,
				    HAL_SW_SWITCH_RADIO_ENABLE_PPI_TASK_RX);
}

static inline void hal_radio_b2b_rxen_on_sw_switch(uint8_t compare_reg_index,
						   uint8_t radio_enable_ppi)
{
	/* NOTE: As independent PPI are used to trigger the Radio Tx task,
	 *       double buffers implementation works for sw_switch using PPIs,
	 *       simply reuse the hal_radio_txen_on_sw_switch() function to set
	 *	 the next PPIs task to be Radio Tx enable.
	 */
	hal_radio_rxen_on_sw_switch(compare_reg_index, radio_enable_ppi);
}

static inline void hal_radio_sw_switch_disable(void)
{
	/* Disable the following PPI channels that implement SW Switch:
	 * - Clearing SW SWITCH TIMER on RADIO END event
	 * - Enabling SW SWITCH PPI Group on RADIO END event
	 */
	nrf_ppi_channels_disable(
		NRF_PPI,
		BIT(HAL_SW_SWITCH_TIMER_CLEAR_PPI) |
		BIT(HAL_SW_SWITCH_GROUP_TASK_ENABLE_PPI));

	/* Invalidation of subscription of S2 timer Compare used when
	 * RXing on LE Coded PHY is not needed, as other DPPI subscription
	 * is disable on each sw_switch call already.
	 */
}

static inline void hal_radio_sw_switch_b2b_tx_disable(uint8_t compare_reg_index)
{
	hal_radio_sw_switch_disable();
}

static inline void hal_radio_sw_switch_b2b_rx_disable(uint8_t compare_reg_index)
{
	hal_radio_sw_switch_disable();
}

static inline void hal_radio_sw_switch_cleanup(void)
{
	hal_radio_sw_switch_disable();
	nrf_ppi_group_disable(NRF_PPI, SW_SWITCH_TIMER_TASK_GROUP(0));
	nrf_ppi_group_disable(NRF_PPI, SW_SWITCH_TIMER_TASK_GROUP(1));
}

#if defined(CONFIG_BT_CTLR_PHY_CODED) && \
	defined(CONFIG_HAS_HW_NRF_RADIO_BLE_CODED)
/* Cancel the SW switch timer running considering S8 timing:
 * wire the RADIO EVENTS_RATEBOOST event to SW_SWITCH_TIMER TASKS_CAPTURE task.
 */
#define HAL_SW_SWITCH_TIMER_S8_DISABLE_PPI_EVT \
	((uint32_t)&(NRF_RADIO->EVENTS_RATEBOOST))
#define HAL_SW_SWITCH_TIMER_S8_DISABLE_PPI_TASK(index) \
	((uint32_t)&(SW_SWITCH_TIMER->TASKS_CAPTURE[index]))

static inline void hal_radio_sw_switch_coded_tx_config_set(uint8_t ppi_en,
	uint8_t ppi_dis, uint8_t cc_s2, uint8_t group_index)
{
	nrf_ppi_event_endpoint_setup(NRF_PPI, ppi_en,
				     HAL_SW_SWITCH_RADIO_ENABLE_PPI_EVT(cc_s2));

	nrf_ppi_task_endpoint_setup(NRF_PPI, ppi_en,
				    HAL_SW_SWITCH_RADIO_ENABLE_PPI_TASK_TX);

	/* Wire the Group task disable
	 * to the S2 EVENTS_COMPARE.
	 */
	nrf_ppi_event_endpoint_setup(NRF_PPI, ppi_dis,
				     HAL_SW_SWITCH_GROUP_TASK_DISABLE_PPI_EVT(cc_s2));

	nrf_ppi_task_endpoint_setup(NRF_PPI, ppi_dis,
				    HAL_SW_SWITCH_GROUP_TASK_DISABLE_PPI_TASK(group_index));

	/* Capture CC to cancel the timer that has assumed
	 * S8 reception, if packet will be received in S2.
	 */
	nrf_ppi_event_endpoint_setup(NRF_PPI, HAL_SW_SWITCH_TIMER_S8_DISABLE_PPI,
				     HAL_SW_SWITCH_TIMER_S8_DISABLE_PPI_EVT);
	nrf_ppi_task_endpoint_setup(NRF_PPI, HAL_SW_SWITCH_TIMER_S8_DISABLE_PPI,
				    HAL_SW_SWITCH_TIMER_S8_DISABLE_PPI_TASK(cc_s2));

	nrf_ppi_channels_enable(
		NRF_PPI,
		BIT(HAL_SW_SWITCH_TIMER_S8_DISABLE_PPI));

	/* Note: below code is not absolutely needed, as other DPPI subscription
	 *       is disable on each sw_switch call already.
	 */
	if (IS_ENABLED(CONFIG_BT_CTLR_PHY_CODED) && false) {
		/* We need to clear the other group S2 PPI */
		group_index = (group_index + 1) & 0x01;
		hal_radio_sw_switch_coded_config_clear(
				HAL_SW_SWITCH_RADIO_ENABLE_S2_PPI(group_index),
				0U, 0U, 0U);
	}
}

static inline void hal_radio_sw_switch_coded_config_clear(uint8_t ppi_en,
	uint8_t ppi_dis, uint8_t cc_s2, uint8_t group_index)
{
	/* Invalidate PPI used when RXing on LE Coded PHY. */
	nrf_ppi_event_endpoint_setup(NRF_PPI, ppi_en, 0);
	nrf_ppi_task_endpoint_setup(NRF_PPI, ppi_en, 0);
}

static inline void hal_radio_sw_switch_disable_group_clear(uint8_t ppi_dis, uint8_t cc_reg,
							   uint8_t group_index)
{
	/* Wire the Group task disable to the default EVENTS_COMPARE. */
	nrf_ppi_event_endpoint_setup(NRF_PPI, ppi_dis,
				     HAL_SW_SWITCH_GROUP_TASK_DISABLE_PPI_EVT(cc_reg));
	nrf_ppi_task_endpoint_setup(NRF_PPI, ppi_dis,
				    HAL_SW_SWITCH_GROUP_TASK_DISABLE_PPI_TASK(group_index));
}

#else

static inline void hal_radio_group_task_disable_ppi_setup(void)
{
	/* Wire SW SWITCH TIMER EVENTS COMPARE event <cc index-0> to
	 * PPI Group TASK [<index-0>] DISABLE task, over PPI<index-0>.
	 */
	nrf_ppi_channel_endpoint_setup(
		NRF_PPI,
		HAL_SW_SWITCH_GROUP_TASK_DISABLE_PPI(0),
		HAL_SW_SWITCH_GROUP_TASK_DISABLE_PPI_EVT(
			SW_SWITCH_TIMER_EVTS_COMP(0)),
		HAL_SW_SWITCH_GROUP_TASK_DISABLE_PPI_TASK(0));

	/* Wire SW SWITCH TIMER event <compare index-1> to
	 * PPI Group[<index-1>] Disable Task, over PPI<index-1>.
	 */
	nrf_ppi_channel_endpoint_setup(
		NRF_PPI,
		HAL_SW_SWITCH_GROUP_TASK_DISABLE_PPI(1),
		HAL_SW_SWITCH_GROUP_TASK_DISABLE_PPI_EVT(
			SW_SWITCH_TIMER_EVTS_COMP(1)),
		HAL_SW_SWITCH_GROUP_TASK_DISABLE_PPI_TASK(1));
}
#endif /* CONFIG_HAS_HW_NRF_RADIO_BLE_CODED */

#if defined(CONFIG_BT_CTLR_DF_PHYEND_OFFSET_COMPENSATION_ENABLE)
/* The 2 adjacent TIMER EVENTS_COMPARE event offsets used for implementing PHYEND delay compensation
 * for SW_SWITCH_TIMER-based TIFS auto-switch. 'index' must be 0 or 1.
 */
#define SW_SWITCH_TIMER_PHYEND_DELAY_COMPENSATION_EVTS_COMP(index) \
	(SW_SWITCH_TIMER_EVTS_COMP_PHYEND_DELAY_COMPENSATION_BASE + (index))
/* Wire the SW SWITCH PHYEND delay compensation TIMER EVENTS_COMPARE[<cc_offset>] event to software
 * switch TIMER0->CLEAR task.
 */
#define HAL_SW_SWITCH_RADIO_ENABLE_PHYEND_DELAY_COMPENSATION_PPI(index) \
	(HAL_SW_SWITCH_RADIO_ENABLE_PHYEND_DELAY_COMPENSATION_PPI_BASE + (index))

/* Cancel the SW switch timer running considering PHYEND delay compensation timing:
 * wire the RADIO EVENTS_CTEPRESENT event to SW_SWITCH_TIMER TASKS_CAPTURE task.
 */
#define HAL_SW_SWITCH_TIMER_PHYEND_DELAY_COMPENSATION_DISABLE_PPI_EVT \
	((uint32_t)&(NRF_RADIO->EVENTS_CTEPRESENT))
#define HAL_SW_SWITCH_TIMER_PHYEND_DELAY_COMPENSATION_DISABLE_PPI_TASK(index) \
	((uint32_t)&(SW_SWITCH_TIMER->TASKS_CAPTURE[index]))

/**
 * @brief Setup additional EVENTS_COMPARE to compensate EVENTS_PHYEND delay.
 *
 * When EVENTS_PHYEND event is used to mark actual end of PDU, CTEINLINE is enabled but received
 * PDU does not include CTE after CRC, the EVENTS_PHYEND will be generated with a short delay.
 * That influences maintenance of TISF by software switch. To compensate the delay additional
 * EVENTS_COMPARE events is set with TIFS value subtracted by the delay. This EVENT_COMPARE
 * event will timeout before regular EVENTS_COMPARE and start radio switching procedure.
 * In case there is a CTEInfo in the received PDU, an EVENTS_CTEPRESENT will be generated by
 * Radio peripheral. The EVENTS_CTEPRESENT event is wired to cancel EVENTS_COMPARE setup for
 * handling delayed EVENTS_PHYEND.
 *
 * Disable of the group of PPIs responsible for handling of software based switch is done by
 * timeout of regular EVENTS_PHYEND event. The EVENTS_PHYEND delay is short enough (16 us) that
 * it can use the same PPI to trigger disable the group for timeout for both EVENTS_COMPAREs.
 * In case the EVENTS_COMPARE for delayed EVENTS_PHYEND event timesout, the group will be disabled
 * within the Radio TX rampup period.
 *
 * @param radio_enable_ppi Index of PPI to wire EVENTS_PHYEND event to Radio enable TX task.
 * @param phyend_delay_cc  Index of EVENTS_COMPARE event to be set for delayed EVENTS_PHYEND event
 */
static inline void
hal_radio_sw_switch_phyend_delay_compensation_config_set(uint8_t radio_enable_ppi,
							 uint8_t phyend_delay_cc)
{
	/* Wire EVENTS_COMPARE[<phyend_delay_cc_offs>] event to Radio TASKS_TXEN */
	nrf_ppi_event_endpoint_setup(NRF_PPI, radio_enable_ppi,
				     HAL_SW_SWITCH_RADIO_ENABLE_PPI_EVT(phyend_delay_cc));

	nrf_ppi_task_endpoint_setup(NRF_PPI, radio_enable_ppi,
				    HAL_SW_SWITCH_RADIO_ENABLE_PPI_TASK_TX);

	/* Wire Radio CTEPRESENT event to cancel EVENTS_COMPARE[<cc_offs>] timer */
	nrf_ppi_event_endpoint_setup(NRF_PPI,
				     HAL_SW_SWITCH_TIMER_PHYEND_DELAY_COMPENSATION_DISABLE_PPI,
				     HAL_SW_SWITCH_TIMER_PHYEND_DELAY_COMPENSATION_DISABLE_PPI_EVT);
	nrf_ppi_task_endpoint_setup(NRF_PPI,
		HAL_SW_SWITCH_TIMER_PHYEND_DELAY_COMPENSATION_DISABLE_PPI,
		HAL_SW_SWITCH_TIMER_PHYEND_DELAY_COMPENSATION_DISABLE_PPI_TASK(phyend_delay_cc));

	/*  Enable CTEPRESENT event to disable EVENTS_COMPARE[<cc_offs>] PPI channel */
	nrf_ppi_channels_enable(NRF_PPI,
				BIT(HAL_SW_SWITCH_TIMER_PHYEND_DELAY_COMPENSATION_DISABLE_PPI));
}

/**
 * @brief Clear additional EVENTS_COMAPRE responsible for compensation EVENTS_PHYEND delay.
 *
 * Disables PPI responsible for cancel of EVENTS_COMPARE set for delayed EVENTS_PHYEND.
 * Removes wireing of delayed EVENTS_COMPARE event to enable Radio TX task.
 *
 * @param radio_enable_ppi Index of PPI to wire EVENTS_PHYEND event to Radio enable TX task.
 * @param phyend_delay_cc  Not used in context of nRF52 SOCs
 */
static inline void
hal_radio_sw_switch_phyend_delay_compensation_config_clear(uint8_t radio_enable_ppi,
							   uint8_t phyend_delay_cc)
{
	/* Invalidate PPI used when */
	nrf_ppi_event_endpoint_setup(NRF_PPI, radio_enable_ppi, NRF_PPI_NONE);
	nrf_ppi_task_endpoint_setup(NRF_PPI, radio_enable_ppi, NRF_PPI_NONE);

	/* Disable CTEPRESENT event to disable EVENTS_COMPARE[<phyend_delay_cc_offs>] PPI channel */
	nrf_ppi_channels_disable(NRF_PPI,
				 BIT(HAL_SW_SWITCH_TIMER_PHYEND_DELAY_COMPENSATION_DISABLE_PPI));
}
#endif /* CONFIG_BT_CTLR_DF_PHYEND_OFFSET_COMPENSATION_ENABLE */

static inline void hal_radio_sw_switch_ppi_group_setup(void)
{
	/* Include the appropriate PPI channels in the two PPI Groups. */
#if !defined(CONFIG_BT_CTLR_PHY_CODED) || \
	!defined(CONFIG_HAS_HW_NRF_RADIO_BLE_CODED)
	NRF_PPI->CHG[SW_SWITCH_TIMER_TASK_GROUP(0)] =
		BIT(HAL_SW_SWITCH_GROUP_TASK_DISABLE_PPI(0)) |
#if defined(CONFIG_BT_CTLR_DF_PHYEND_OFFSET_COMPENSATION_ENABLE)
		BIT(HAL_SW_SWITCH_RADIO_ENABLE_PHYEND_DELAY_COMPENSATION_PPI(0)) |
#endif /* CONFIG_BT_CTLR_DF_PHYEND_OFFSET_COMPENSATION_ENABLE */
		BIT(HAL_SW_SWITCH_RADIO_ENABLE_PPI(0));
	NRF_PPI->CHG[SW_SWITCH_TIMER_TASK_GROUP(1)] =
		BIT(HAL_SW_SWITCH_GROUP_TASK_DISABLE_PPI(1)) |
#if defined(CONFIG_BT_CTLR_DF_PHYEND_OFFSET_COMPENSATION_ENABLE)
		BIT(HAL_SW_SWITCH_RADIO_ENABLE_PHYEND_DELAY_COMPENSATION_PPI(1)) |
#endif /* CONFIG_BT_CTLR_DF_PHYEND_OFFSET_COMPENSATION_ENABLE */
		BIT(HAL_SW_SWITCH_RADIO_ENABLE_PPI(1));
#else
	NRF_PPI->CHG[SW_SWITCH_TIMER_TASK_GROUP(0)] =
		BIT(HAL_SW_SWITCH_GROUP_TASK_DISABLE_PPI(0)) |
		BIT(HAL_SW_SWITCH_RADIO_ENABLE_PPI(0)) |
		BIT(HAL_SW_SWITCH_RADIO_ENABLE_S2_PPI(0));
	NRF_PPI->CHG[SW_SWITCH_TIMER_TASK_GROUP(1)] =
		BIT(HAL_SW_SWITCH_GROUP_TASK_DISABLE_PPI(1)) |
		BIT(HAL_SW_SWITCH_RADIO_ENABLE_PPI(1)) |
		BIT(HAL_SW_SWITCH_RADIO_ENABLE_S2_PPI(1));
#endif /* CONFIG_BT_CTLR_PHY_CODED && CONFIG_HAS_HW_NRF_RADIO_BLE_CODED */
}

#endif /* !CONFIG_BT_CTLR_TIFS_HW */
