/*
 * Copyright (c) 2018 - 2019 Nordic Semiconductor ASA
 * Copyright (c) 2018 Ioannis Glaropoulos
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_SOC_SERIES_NRF51X) || defined(CONFIG_SOC_COMPATIBLE_NRF52X)

#include <nrfx/hal/nrf_ppi.h>

static inline void hal_radio_nrf_ppi_channels_enable(u32_t mask)
{
	nrf_ppi_channels_enable(mask);
}

static inline void hal_radio_nrf_ppi_channels_disable(u32_t mask)
{
	nrf_ppi_channels_disable(mask);
}

static inline void hal_radio_nrf_ppi_group_disable(u32_t group)
{
	nrf_ppi_group_disable(group);
}

/*******************************************************************************
 * Enable Radio on Event Timer tick:
 * wire the EVENT_TIMER EVENTS_COMPARE[0] event to RADIO TASKS_TXEN/RXEN task.
 *
 * Use the pre-programmed PPI channels if possible (if TIMER0 is used as the
 * EVENT_TIMER).
 */
#if (EVENT_TIMER_ID == 0)

/* PPI channel 20 is pre-programmed with the following fixed settings:
 *   EEP: TIMER0->EVENTS_COMPARE[0]
 *   TEP: RADIO->TASKS_TXEN
 */
#define HAL_RADIO_ENABLE_TX_ON_TICK_PPI 20
/* PPI channel 21 is pre-programmed with the following fixed settings:
 *   EEP: TIMER0->EVENTS_COMPARE[0]
 *   TEP: RADIO->TASKS_RXEN
 */
#define HAL_RADIO_ENABLE_RX_ON_TICK_PPI 21

static inline void hal_radio_enable_on_tick_ppi_config_and_enable(u8_t trx)
{
	/* No need to configure anything for the pre-programmed channels.
	 * Just enable and disable them accordingly.
	 */
	nrf_ppi_channels_disable(
		trx ? BIT(HAL_RADIO_ENABLE_RX_ON_TICK_PPI)
		    : BIT(HAL_RADIO_ENABLE_TX_ON_TICK_PPI));
	nrf_ppi_channels_enable(
		trx ? BIT(HAL_RADIO_ENABLE_TX_ON_TICK_PPI)
		    : BIT(HAL_RADIO_ENABLE_RX_ON_TICK_PPI));
}

#else

#define HAL_RADIO_ENABLE_ON_TICK_PPI 1
#define HAL_RADIO_ENABLE_TX_ON_TICK_PPI HAL_RADIO_ENABLE_ON_TICK_PPI
#define HAL_RADIO_ENABLE_RX_ON_TICK_PPI HAL_RADIO_ENABLE_ON_TICK_PPI

static inline void hal_radio_enable_on_tick_ppi_config_and_enable(u8_t trx)
{
	u32_t event_address = (trx ? (u32_t)&(NRF_RADIO->TASKS_TXEN)
				   : (u32_t)&(NRF_RADIO->TASKS_RXEN));
	nrf_ppi_channel_endpoint_setup(
		HAL_RADIO_ENABLE_ON_TICK_PPI,
		(u32_t)&(EVENT_TIMER->EVENTS_COMPARE[0]),
		event_address);
	nrf_ppi_channels_enable(BIT(HAL_RADIO_ENABLE_ON_TICK_PPI));
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

/* PPI channel 26 is pre-programmed with the following fixed settings:
 *   EEP: RADIO->EVENTS_ADDRESS
 *   TEP: TIMER0->TASKS_CAPTURE[1]
 */
#define HAL_RADIO_RECV_TIMEOUT_CANCEL_PPI 26

static inline void hal_radio_recv_timeout_cancel_ppi_config(void)
{
	/* No need to configure anything for the pre-programmed channel. */
}

#else

#define HAL_RADIO_RECV_TIMEOUT_CANCEL_PPI 2

static inline void hal_radio_recv_timeout_cancel_ppi_config(void)
{
	nrf_ppi_channel_endpoint_setup(
		HAL_RADIO_RECV_TIMEOUT_CANCEL_PPI,
		(u32_t)&(NRF_RADIO->EVENTS_ADDRESS),
		(u32_t)&(EVENT_TIMER->TASKS_CAPTURE[1]));
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

/* PPI channel 22 is pre-programmed with the following fixed settings:
 *   EEP: TIMER0->EVENTS_COMPARE[1]
 *   TEP: RADIO->TASKS_DISABLE
 */
#define HAL_RADIO_DISABLE_ON_HCTO_PPI 22

static inline void hal_radio_disable_on_hcto_ppi_config(void)
{
	/* No need to configure anything for the pre-programmed channel. */
}

#else

#define HAL_RADIO_DISABLE_ON_HCTO_PPI 3

static inline void hal_radio_disable_on_hcto_ppi_config(void)
{
	nrf_ppi_channel_endpoint_setup(
		HAL_RADIO_DISABLE_ON_HCTO_PPI,
		(u32_t)&(EVENT_TIMER->EVENTS_COMPARE[1]),
		(u32_t)&(NRF_RADIO->TASKS_DISABLE));
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

/* PPI channel 27 is pre-programmed with the following fixed settings:
 *   EEP: RADIO->EVENTS_END
 *   TEP: TIMER0->TASKS_CAPTURE[2]
 */
#define HAL_RADIO_END_TIME_CAPTURE_PPI 27

static inline void hal_radio_end_time_capture_ppi_config(void)
{
	/* No need to configure anything for the pre-programmed channel. */
}

#else

#define HAL_RADIO_END_TIME_CAPTURE_PPI 4

static inline void hal_radio_end_time_capture_ppi_config(void)
{
	nrf_ppi_channel_endpoint_setup(
		HAL_RADIO_END_TIME_CAPTURE_PPI,
		(u32_t)&(NRF_RADIO->EVENTS_END),
		(u32_t)&(EVENT_TIMER->TASKS_CAPTURE[2]));
}

#endif /* (EVENT_TIMER_ID == 0) */

/*******************************************************************************
 * Start event timer on RTC tick:
 * wire the RTC0 EVENTS_COMPARE[2] event to EVENT_TIMER  TASKS_START task.
 */
#define HAL_EVENT_TIMER_START_PPI 5

static inline void hal_event_timer_start_ppi_config(void)
{
	nrf_ppi_channel_endpoint_setup(
		HAL_EVENT_TIMER_START_PPI,
		(u32_t)&(NRF_RTC0->EVENTS_COMPARE[2]),
		(u32_t)&(EVENT_TIMER->TASKS_START));
}

/*******************************************************************************
 * Capture event timer on Radio ready:
 * wire the RADIO EVENTS_READY event to the
 * EVENT_TIMER TASKS_CAPTURE[<radio ready timer>] task.
 */
#define HAL_RADIO_READY_TIME_CAPTURE_PPI 6

static inline void hal_radio_ready_time_capture_ppi_config(void)
{
	nrf_ppi_channel_endpoint_setup(
		HAL_RADIO_READY_TIME_CAPTURE_PPI,
		(u32_t)&(NRF_RADIO->EVENTS_READY),
		(u32_t)&(EVENT_TIMER->TASKS_CAPTURE[0]));
}

/*******************************************************************************
 * Trigger encryption task upon address reception:
 * wire the RADIO EVENTS_ADDRESS event to the CCM TASKS_CRYPT task.
 *
 * PPI channel 25 is pre-programmed with the following fixed settings:
 *   EEP: RADIO->EVENTS_ADDRESS
 *   TEP: CCM->TASKS_CRYPT
 */
#define HAL_TRIGGER_CRYPT_PPI 25

static inline void hal_trigger_crypt_ppi_config(void)
{
	/* No need to configure anything for the pre-programmed channel. */
}

/*******************************************************************************
 * Trigger automatic address resolution on Bit counter match:
 * wire the RADIO EVENTS_BCMATCH event to the AAR TASKS_START task.
 *
 * PPI channel 23 is pre-programmed with the following fixed settings:
 *   EEP: RADIO->EVENTS_BCMATCH
 *   TEP: AAR->TASKS_START
 */
#define HAL_TRIGGER_AAR_PPI 23

static inline void hal_trigger_aar_ppi_config(void)
{
	/* No need to configure anything for the pre-programmed channel. */
}

/*******************************************************************************
 * Trigger Radio Rate override upon Rateboost event.
 */
#if defined(CONFIG_HAS_HW_NRF_RADIO_BLE_CODED)

#define HAL_TRIGGER_RATEOVERRIDE_PPI 13

static inline void hal_trigger_rateoverride_ppi_config(void)
{
	nrf_ppi_channel_endpoint_setup(
		HAL_TRIGGER_RATEOVERRIDE_PPI,
		(u32_t)&(NRF_RADIO->EVENTS_RATEBOOST),
		(u32_t)&(NRF_CCM->TASKS_RATEOVERRIDE));
}

#endif /* CONFIG_HAS_HW_NRF_RADIO_BLE_CODED */

/******************************************************************************/
#if defined(CONFIG_BT_CTLR_GPIO_PA_PIN) || defined(CONFIG_BT_CTLR_GPIO_LNA_PIN)

#define HAL_ENABLE_PALNA_PPI 14

static inline void hal_enable_palna_ppi_config(void)
{
	nrf_ppi_channel_endpoint_setup(
		HAL_ENABLE_PALNA_PPI,
		(u32_t)&(EVENT_TIMER->EVENTS_COMPARE[2]),
		(u32_t)&(NRF_GPIOTE->TASKS_OUT[
				CONFIG_BT_CTLR_PA_LNA_GPIOTE_CHAN]));
}

#define HAL_DISABLE_PALNA_PPI 15

static inline void hal_disable_palna_ppi_config(void)
{
	nrf_ppi_channel_endpoint_setup(HAL_DISABLE_PALNA_PPI,
		(u32_t)&(NRF_RADIO->EVENTS_DISABLED),
		(u32_t)&(NRF_GPIOTE->TASKS_OUT[
				CONFIG_BT_CTLR_PA_LNA_GPIOTE_CHAN]));
}

#endif /* CONFIG_BT_CTLR_GPIO_PA_PIN || CONFIG_BT_CTLR_GPIO_LNA_PIN */

/******************************************************************************/
#if !defined(CONFIG_BT_CTLR_TIFS_HW)
/* PPI setup used for SW-based auto-switching during TIFS. */

#if !defined(CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER)

/* Clear SW-switch timer on packet end:
 * wire the RADIO EVENTS_END event to SW_SWITCH_TIMER TASKS_CLEAR task.
 *
 * Note: this PPI is not needed if we use a single TIMER instance in radio.c
 */
#define HAL_SW_SWITCH_TIMER_CLEAR_PPI 7

static inline void hal_sw_switch_timer_clear_ppi_config(void)
{
	nrf_ppi_channel_endpoint_setup(
		HAL_SW_SWITCH_TIMER_CLEAR_PPI,
		(u32_t)&(NRF_RADIO->EVENTS_END),
		(u32_t)&(SW_SWITCH_TIMER->TASKS_CLEAR));
}

#else /* !CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER */

/* Clear event timer (sw-switch timer) on Radio end:
 * wire the RADIO EVENTS_END event to the
 * EVENT_TIMER TASKS_CLEAR task.
 *
 * Note: in nRF52X this PPI channel is forked for both capturing and clearing
 * timer on RADIO EVENTS_END.
 */
#define HAL_SW_SWITCH_TIMER_CLEAR_PPI HAL_RADIO_END_TIME_CAPTURE_PPI

static inline void hal_sw_switch_timer_clear_ppi_config(void)
{
	nrf_ppi_fork_endpoint_setup(
		HAL_RADIO_END_TIME_CAPTURE_PPI,
		(u32_t)&(SW_SWITCH_TIMER->TASKS_CLEAR));
}

#endif /* !CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER */

/* The 2 adjacent PPI groups used for implementing SW_SWITCH_TIMER-based
 * auto-switch for TIFS. 'index' must be 0 or 1.
 */
#define SW_SWITCH_TIMER_TASK_GROUP(index) \
	(SW_SWITCH_TIMER_TASK_GROUP_BASE + index)

/* The 2 adjacent TIMER EVENTS_COMPARE event offsets used for implementing
 * SW_SWITCH_TIMER-based auto-switch for TIFS. 'index' must be 0 or 1.
 */
#define SW_SWITCH_TIMER_EVTS_COMP(index) \
	(SW_SWITCH_TIMER_EVTS_COMP_BASE + index)

/* Wire a SW SWITCH TIMER EVENTS_COMPARE[<cc_offset>] event
 * to a PPI GROUP TASK DISABLE task (PPI group with index <index>).
 * 2 adjacent PPIs (8 & 9) and 2 adjacent PPI groups are used for this wiring;
 * <index> must be 0 or 1. <offset> must be a valid TIMER CC register offset.
 */
#define HAL_SW_SWITCH_GROUP_TASK_DISABLE_PPI_BASE 8
#define HAL_SW_SWITCH_GROUP_TASK_DISABLE_PPI(index) \
	(HAL_SW_SWITCH_GROUP_TASK_DISABLE_PPI_BASE + index)
#define HAL_SW_SWITCH_GROUP_TASK_DISABLE_PPI_0_INCLUDE \
	((PPI_CHG_CH8_Included << PPI_CHG_CH8_Pos) & PPI_CHG_CH8_Msk)
#define HAL_SW_SWITCH_GROUP_TASK_DISABLE_PPI_0_EXCLUDE \
	((PPI_CHG_CH8_Excluded << PPI_CHG_CH8_Pos) & PPI_CHG_CH8_Msk)
#define HAL_SW_SWITCH_GROUP_TASK_DISABLE_PPI_1_INCLUDE \
	((PPI_CHG_CH9_Included << PPI_CHG_CH9_Pos) & PPI_CHG_CH9_Msk)
#define HAL_SW_SWITCH_GROUP_TASK_DISABLE_PPI_1_EXCLUDE \
	((PPI_CHG_CH9_Excluded << PPI_CHG_CH9_Pos) & PPI_CHG_CH9_Msk)
#define HAL_SW_SWITCH_GROUP_TASK_DISABLE_PPI_REGISTER_EVT(chan) \
	NRF_PPI->CH[chan].EEP
#define HAL_SW_SWITCH_GROUP_TASK_DISABLE_PPI_EVT(cc_offset) \
	((u32_t)&(SW_SWITCH_TIMER->EVENTS_COMPARE[cc_offset]))
#define HAL_SW_SWITCH_GROUP_TASK_DISABLE_PPI_REGISTER_TASK(chan) \
	NRF_PPI->CH[chan].TEP
#define HAL_SW_SWITCH_GROUP_TASK_DISABLE_PPI_TASK(index) \
	((u32_t)&(NRF_PPI->TASKS_CHG[SW_SWITCH_TIMER_TASK_GROUP(index)].DIS))

/* Wire the RADIO EVENTS_END event to one of the PPI GROUP TASK ENABLE task.
 * 2 adjacent PPI groups are used for this wiring. 'index' must be 0 or 1.
 */
#define HAL_SW_SWITCH_GROUP_TASK_ENABLE_PPI 10
#define HAL_SW_SWITCH_GROUP_TASK_ENABLE_PPI_EVT \
	((u32_t)&(NRF_RADIO->EVENTS_END))
#define HAL_SW_SWITCH_GROUP_TASK_ENABLE_PPI_TASK(index) \
	((u32_t)&(NRF_PPI->TASKS_CHG[SW_SWITCH_TIMER_TASK_GROUP(index)].EN))

/*Enable Radio at specific time-stamp:
 * wire the SW SWITCH TIMER EVENTS_COMPARE[<cc_offset>] event
 * to RADIO TASKS_TXEN/RXEN task.
 * 2 adjacent PPIs (11 & 12) are used for this wiring; <index> must be 0 or 1.
 * <offset> must be a valid TIMER CC register offset.
 */
#define HAL_SW_SWITCH_RADIO_ENABLE_PPI_BASE 11
#define HAL_SW_SWITCH_RADIO_ENABLE_PPI(index) \
	(HAL_SW_SWITCH_RADIO_ENABLE_PPI_BASE + index)
#define HAL_SW_SWITCH_RADIO_ENABLE_PPI_0_INCLUDE \
	((PPI_CHG_CH11_Included << PPI_CHG_CH11_Pos) & PPI_CHG_CH11_Msk)
#define HAL_SW_SWITCH_RADIO_ENABLE_PPI_0_EXCLUDE \
	((PPI_CHG_CH11_Excluded << PPI_CHG_CH11_Pos) & PPI_CHG_CH11_Msk)
#define HAL_SW_SWITCH_RADIO_ENABLE_PPI_1_INCLUDE \
	((PPI_CHG_CH12_Included << PPI_CHG_CH12_Pos) & PPI_CHG_CH12_Msk)
#define HAL_SW_SWITCH_RADIO_ENABLE_PPI_1_EXCLUDE \
	((PPI_CHG_CH12_Excluded << PPI_CHG_CH12_Pos) & PPI_CHG_CH12_Msk)
#define HAL_SW_SWITCH_RADIO_ENABLE_PPI_REGISTER_EVT(chan) \
	NRF_PPI->CH[chan].EEP
#define HAL_SW_SWITCH_RADIO_ENABLE_PPI_EVT(cc_offset) \
	((u32_t)&(SW_SWITCH_TIMER->EVENTS_COMPARE[cc_offset]))
#define HAL_SW_SWITCH_RADIO_ENABLE_PPI_REGISTER_TASK(chan) \
	NRF_PPI->CH[chan].TEP
#define HAL_SW_SWITCH_RADIO_ENABLE_PPI_TASK_TX \
	((u32_t)&(NRF_RADIO->TASKS_TXEN))
#define HAL_SW_SWITCH_RADIO_ENABLE_PPI_TASK_RX \
	((u32_t)&(NRF_RADIO->TASKS_RXEN))

static inline void hal_radio_sw_switch_setup(
		u8_t compare_reg,
		u8_t radio_enable_ppi,
		u8_t ppi_group_index)
{
	/* Set up software switch mechanism for next Radio switch. */

	/* Wire RADIO END event to PPI Group[<index>] enable task,
	 * over PPI[<HAL_SW_SWITCH_GROUP_TASK_ENABLE_PPI>]
	 */
	nrf_ppi_channel_endpoint_setup(HAL_SW_SWITCH_GROUP_TASK_ENABLE_PPI,
		HAL_SW_SWITCH_GROUP_TASK_ENABLE_PPI_EVT,
		HAL_SW_SWITCH_GROUP_TASK_ENABLE_PPI_TASK(ppi_group_index));

	/* Wire SW Switch timer event <compare_reg> to the
	 * PPI[<radio_enable_ppi>] for enabling Radio. Do
	 * not wire the task; it is done by the caller of
	 * the function depending on the desired direction
	 * (TX/RX).
	 */
	nrf_ppi_event_endpoint_setup(radio_enable_ppi,
		HAL_SW_SWITCH_RADIO_ENABLE_PPI_EVT(compare_reg));
}

static inline void hal_radio_txen_on_sw_switch(u8_t ppi)
{
	nrf_ppi_task_endpoint_setup(ppi,
		HAL_SW_SWITCH_RADIO_ENABLE_PPI_TASK_TX);
}

static inline void hal_radio_rxen_on_sw_switch(u8_t ppi)
{
	nrf_ppi_task_endpoint_setup(ppi,
		HAL_SW_SWITCH_RADIO_ENABLE_PPI_TASK_RX);
}

static inline void hal_radio_sw_switch_disable(void)
{
	/* Disable the following PPI channels that implement SW Switch:
	 * - Clearing SW SWITCH TIMER on RADIO END event
	 * - Enabling SW SWITCH PPI Group on RADIO END event
	 */
	nrf_ppi_channels_disable(BIT(HAL_SW_SWITCH_TIMER_CLEAR_PPI) |
		 BIT(HAL_SW_SWITCH_GROUP_TASK_ENABLE_PPI));
}

#if defined(CONFIG_BT_CTLR_PHY_CODED) && \
	defined(CONFIG_HAS_HW_NRF_RADIO_BLE_CODED)
/* The 2 adjacent TIMER EVENTS_COMPARE event offsets used for implementing
 * SW_SWITCH_TIMER-based auto-switch for TIFS, when receiving in LE Coded PHY.
 *  'index' must be 0 or 1.
 */
#define SW_SWITCH_TIMER_S2_EVTS_COMP(index) \
	(SW_SWITCH_TIMER_EVTS_COMP_S2_BASE + index)

/* Wire the SW SWITCH TIMER EVENTS_COMPARE[<cc_offset>] event
 * to RADIO TASKS_TXEN/RXEN task.
 */
#define HAL_SW_SWITCH_RADIO_ENABLE_S2_PPI_BASE 16
#define HAL_SW_SWITCH_RADIO_ENABLE_S2_PPI(index) \
	(HAL_SW_SWITCH_RADIO_ENABLE_S2_PPI_BASE + index)
#define HAL_SW_SWITCH_RADIO_ENABLE_S2_PPI_0_INCLUDE \
	((PPI_CHG_CH16_Included << PPI_CHG_CH16_Pos) & PPI_CHG_CH16_Msk)
#define HAL_SW_SWITCH_RADIO_ENABLE_S2_PPI_0_EXCLUDE \
	((PPI_CHG_CH16_Excluded << PPI_CHG_CH16_Pos) & PPI_CHG_CH16_Msk)
#define HAL_SW_SWITCH_RADIO_ENABLE_S2_PPI_1_INCLUDE \
	((PPI_CHG_CH17_Included << PPI_CHG_CH17_Pos) & PPI_CHG_CH17_Msk)
#define HAL_SW_SWITCH_RADIO_ENABLE_S2_PPI_1_EXCLUDE \
	((PPI_CHG_CH17_Excluded << PPI_CHG_CH17_Pos) & PPI_CHG_CH17_Msk)

/* Cancel the SW switch timer running considering S8 timing:
 * wire the RADIO EVENTS_RATEBOOST event to SW_SWITCH_TIMER TASKS_CAPTURE task.
 */
#define HAL_SW_SWITCH_TIMER_S8_DISABLE_PPI 18
#define HAL_SW_SWITCH_TIMER_S8_DISABLE_PPI_REGISTER_EVT \
	NRF_PPI->CH[HAL_SW_SWITCH_TIMER_S8_DISABLE_PPI].EEP
#define HAL_SW_SWITCH_TIMER_S8_DISABLE_PPI_EVT \
	((u32_t)&(NRF_RADIO->EVENTS_RATEBOOST))
#define HAL_SW_SWITCH_TIMER_S8_DISABLE_PPI_REGISTER_TASK \
	NRF_PPI->CH[HAL_SW_SWITCH_TIMER_S8_DISABLE_PPI].TEP
#define HAL_SW_SWITCH_TIMER_S8_DISABLE_PPI_TASK(index) \
	((u32_t)&(SW_SWITCH_TIMER->TASKS_CAPTURE[index]))

static inline void hal_radio_sw_switch_coded_tx_config_set(u8_t ppi_en,
	u8_t ppi_dis, u8_t cc_s2, u8_t group_index)
{
	HAL_SW_SWITCH_RADIO_ENABLE_PPI_REGISTER_EVT(ppi_en) =
		HAL_SW_SWITCH_RADIO_ENABLE_PPI_EVT(cc_s2);

	HAL_SW_SWITCH_RADIO_ENABLE_PPI_REGISTER_TASK(ppi_en) =
		HAL_SW_SWITCH_RADIO_ENABLE_PPI_TASK_TX;

	/* Wire the Group task disable
	 * to the S2 EVENTS_COMPARE.
	 */
	HAL_SW_SWITCH_GROUP_TASK_DISABLE_PPI_REGISTER_EVT(
	    ppi_dis)	=
	    HAL_SW_SWITCH_GROUP_TASK_DISABLE_PPI_EVT(cc_s2);

	HAL_SW_SWITCH_GROUP_TASK_DISABLE_PPI_REGISTER_TASK(
	    ppi_dis) =
	    HAL_SW_SWITCH_GROUP_TASK_DISABLE_PPI_TASK(
	    group_index);

	/* Capture CC to cancel the timer that has assumed
	 * S8 reception, if packet will be received in S2.
	 */
	HAL_SW_SWITCH_TIMER_S8_DISABLE_PPI_REGISTER_EVT =
		HAL_SW_SWITCH_TIMER_S8_DISABLE_PPI_EVT;
	HAL_SW_SWITCH_TIMER_S8_DISABLE_PPI_REGISTER_TASK =
		HAL_SW_SWITCH_TIMER_S8_DISABLE_PPI_TASK(
			group_index);

	nrf_ppi_channels_enable(
		BIT(HAL_SW_SWITCH_TIMER_S8_DISABLE_PPI));
}

static inline void hal_radio_sw_switch_coded_config_clear(u8_t ppi_en,
	u8_t ppi_dis, u8_t cc_reg, u8_t group_index)
{
	/* Invalidate PPI used when RXing on LE Coded PHY. */
	HAL_SW_SWITCH_RADIO_ENABLE_PPI_REGISTER_EVT(
		ppi_en) = 0;
	HAL_SW_SWITCH_RADIO_ENABLE_PPI_REGISTER_TASK(
		ppi_en) = 0;

	/* Wire the Group task disable to the default EVENTS_COMPARE. */
	HAL_SW_SWITCH_GROUP_TASK_DISABLE_PPI_REGISTER_EVT(
		ppi_dis) =
		HAL_SW_SWITCH_GROUP_TASK_DISABLE_PPI_EVT(cc_reg);
	HAL_SW_SWITCH_GROUP_TASK_DISABLE_PPI_REGISTER_TASK(
		ppi_dis) =
		HAL_SW_SWITCH_GROUP_TASK_DISABLE_PPI_TASK(
			group_index);
}

#else

static inline void hal_radio_group_task_disable_ppi_setup(void)
{
	/* Wire SW SWITCH TIMER EVENTS COMPARE event <cc index-0> to
	 * PPI Group TASK [<index-0>] DISABLE task, over PPI<index-0>.
	 */
	nrf_ppi_channel_endpoint_setup(
		HAL_SW_SWITCH_GROUP_TASK_DISABLE_PPI(0),
		HAL_SW_SWITCH_GROUP_TASK_DISABLE_PPI_EVT(
			SW_SWITCH_TIMER_EVTS_COMP(0)),
		HAL_SW_SWITCH_GROUP_TASK_DISABLE_PPI_TASK(0));

	/* Wire SW SWITCH TIMER event <compare index-1> to
	 * PPI Group[<index-1>] Disable Task, over PPI<index-1>.
	 */
	nrf_ppi_channel_endpoint_setup(
		HAL_SW_SWITCH_GROUP_TASK_DISABLE_PPI(1),
		HAL_SW_SWITCH_GROUP_TASK_DISABLE_PPI_EVT(
			SW_SWITCH_TIMER_EVTS_COMP(1)),
		HAL_SW_SWITCH_GROUP_TASK_DISABLE_PPI_TASK(1));
}
#endif /* CONFIG_HAS_HW_NRF_RADIO_BLE_CODED */

static inline void hal_radio_sw_switch_ppi_group_setup(void)
{
	/* Include the appropriate PPI channels in the two PPI Groups. */
#if !defined(CONFIG_BT_CTLR_PHY_CODED) || \
	!defined(CONFIG_HAS_HW_NRF_RADIO_BLE_CODED)
	NRF_PPI->CHG[SW_SWITCH_TIMER_TASK_GROUP(0)] =
		HAL_SW_SWITCH_GROUP_TASK_DISABLE_PPI_0_INCLUDE |
			HAL_SW_SWITCH_RADIO_ENABLE_PPI_0_INCLUDE;
	NRF_PPI->CHG[SW_SWITCH_TIMER_TASK_GROUP(1)] =
		HAL_SW_SWITCH_GROUP_TASK_DISABLE_PPI_1_INCLUDE |
			HAL_SW_SWITCH_RADIO_ENABLE_PPI_1_INCLUDE;
#else
	NRF_PPI->CHG[SW_SWITCH_TIMER_TASK_GROUP(0)] =
		HAL_SW_SWITCH_GROUP_TASK_DISABLE_PPI_0_INCLUDE |
		HAL_SW_SWITCH_RADIO_ENABLE_PPI_0_INCLUDE |
		HAL_SW_SWITCH_RADIO_ENABLE_S2_PPI_0_INCLUDE;
	NRF_PPI->CHG[SW_SWITCH_TIMER_TASK_GROUP(1)] =
		HAL_SW_SWITCH_GROUP_TASK_DISABLE_PPI_1_INCLUDE |
		HAL_SW_SWITCH_RADIO_ENABLE_PPI_1_INCLUDE |
		HAL_SW_SWITCH_RADIO_ENABLE_S2_PPI_1_INCLUDE;
#endif /* CONFIG_BT_CTLR_PHY_CODED && CONFIG_HAS_HW_NRF_RADIO_BLE_CODED */
}

#endif /* !CONFIG_BT_CTLR_TIFS_HW */

/******************************************************************************/

#define HAL_USED_PPI_CHANNELS \
	(BIT(HAL_RADIO_ENABLE_TX_ON_TICK_PPI) | \
	 BIT(HAL_RADIO_ENABLE_RX_ON_TICK_PPI) | \
	 BIT(HAL_RADIO_RECV_TIMEOUT_CANCEL_PPI) | \
	 BIT(HAL_RADIO_DISABLE_ON_HCTO_PPI) | \
	 BIT(HAL_RADIO_END_TIME_CAPTURE_PPI) | \
	 BIT(HAL_EVENT_TIMER_START_PPI) | \
	 BIT(HAL_RADIO_READY_TIME_CAPTURE_PPI) | \
	 BIT(HAL_TRIGGER_CRYPT_PPI) | \
	 BIT(HAL_TRIGGER_AAR_PPI) | \
	 HAL_USED_PPI_CHANNELS_2 | HAL_USED_PPI_CHANNELS_3 | \
	 HAL_USED_PPI_CHANNELS_4 | HAL_USED_PPI_CHANNELS_5)

#if defined(HAL_TRIGGER_RATEOVERRIDE_PPI)
#define HAL_USED_PPI_CHANNELS_2 \
	BIT(HAL_TRIGGER_RATEOVERRIDE_PPI)
#else
#define HAL_USED_PPI_CHANNELS_2 0
#endif

#if defined(HAL_ENABLE_PALNA_PPI)
#define HAL_USED_PPI_CHANNELS_3 \
	(BIT(HAL_ENABLE_PALNA_PPI) | \
	 BIT(HAL_DISABLE_PALNA_PPI))
#else
#define HAL_USED_PPI_CHANNELS_3 0
#endif

#if defined(HAL_SW_SWITCH_TIMER_CLEAR_PPI)
#define HAL_USED_PPI_CHANNELS_4 \
	(BIT(HAL_SW_SWITCH_TIMER_CLEAR_PPI) | \
	 BIT(HAL_SW_SWITCH_GROUP_TASK_DISABLE_PPI_BASE) | \
	 BIT(HAL_SW_SWITCH_GROUP_TASK_DISABLE_PPI_BASE+1) | \
	 BIT(HAL_SW_SWITCH_GROUP_TASK_ENABLE_PPI) | \
	 BIT(HAL_SW_SWITCH_RADIO_ENABLE_PPI_BASE) | \
	 BIT(HAL_SW_SWITCH_RADIO_ENABLE_PPI_BASE+1))
#else
#define HAL_USED_PPI_CHANNELS_4 0
#endif

#if defined(HAL_SW_SWITCH_RADIO_ENABLE_S2_PPI_BASE)
#define HAL_USED_PPI_CHANNELS_5 \
	(BIT(HAL_SW_SWITCH_RADIO_ENABLE_S2_PPI_BASE) | \
	 BIT(HAL_SW_SWITCH_RADIO_ENABLE_S2_PPI_BASE+1) | \
	 BIT(HAL_SW_SWITCH_TIMER_S8_DISABLE_PPI))
#else
#define HAL_USED_PPI_CHANNELS_5 0
#endif

#if defined(CONFIG_SOC_SERIES_BSIM_NRFXX)
/* When the build is targeting an NRF board simulated with BabbleSim,
 * nrfx_glue.h is not processed and the following symbol is not defined.
 */
#define NRFX_PPI_CHANNELS_USED_BY_PWM_SW  0
#endif
BUILD_ASSERT_MSG(
	(HAL_USED_PPI_CHANNELS & NRFX_PPI_CHANNELS_USED_BY_PWM_SW) == 0,
	"PPI channels used by the Bluetooth controller overlap with those "
	"assigned to the pwm_nrf5_sw driver.");

#if defined(SW_SWITCH_TIMER_TASK_GROUP_BASE)
#define HAL_USED_PPI_GROUPS \
	(BIT(SW_SWITCH_TIMER_TASK_GROUP_BASE) | \
	 BIT(SW_SWITCH_TIMER_TASK_GROUP_BASE+1))
#else
#define HAL_USED_PPI_GROUPS 0
#endif

#endif /* CONFIG_SOC_SERIES_NRF51X || CONFIG_SOC_COMPATIBLE_NRF52X */
