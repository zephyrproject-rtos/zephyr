/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#if defined(CONFIG_SOC_NRF5340_CPUNET) || defined(DPPI_PRESENT)

/*******************************************************************************
 * Enable Radio on Event Timer tick:
 * wire the EVENT_TIMER EVENTS_COMPARE[0] event to RADIO TASKS_TXEN/RXEN task.
 */
#define HAL_RADIO_ENABLE_ON_TICK_PPI 6
#define HAL_RADIO_ENABLE_TX_ON_TICK_PPI HAL_RADIO_ENABLE_ON_TICK_PPI
#define HAL_RADIO_ENABLE_RX_ON_TICK_PPI HAL_RADIO_ENABLE_ON_TICK_PPI

/*******************************************************************************
 * Capture event timer on Address reception:
 * wire the RADIO EVENTS_ADDRESS event to the
 * EVENT_TIMER TASKS_CAPTURE[<address timer>] task.
 */
#define HAL_RADIO_RECV_TIMEOUT_CANCEL_PPI 9

/*******************************************************************************
 * Disable Radio on HCTO:
 * wire the EVENT_TIMER EVENTS_COMPARE[<HCTO timer>] event
 * to the RADIO TASKS_DISABLE task.
 */
#define HAL_RADIO_DISABLE_ON_HCTO_PPI 10

/*******************************************************************************
 * Capture event timer on Radio end:
 * wire the RADIO EVENTS_END event to the
 * EVENT_TIMER TASKS_CAPTURE[<radio end timer>] task.
 */
#define HAL_RADIO_END_TIME_CAPTURE_PPI 11

/*******************************************************************************
 * Start event timer on RTC tick:
 * wire the RTC0 EVENTS_COMPARE[2] event to EVENT_TIMER  TASKS_START task.
 */
#define HAL_EVENT_TIMER_START_PPI 7

/*******************************************************************************
 * Capture event timer on Radio ready:
 * wire the RADIO EVENTS_READY event to the
 * EVENT_TIMER TASKS_CAPTURE[<radio ready timer>] task.
 */
#define HAL_RADIO_READY_TIME_CAPTURE_PPI 8

/*******************************************************************************
 * Trigger encryption task upon address reception:
 * wire the RADIO EVENTS_ADDRESS event to the CCM TASKS_CRYPT task.
 *
 * Note: we do not need an additional PPI, since we have already set up
 * a PPI to publish RADIO ADDRESS event.
 */
#define HAL_TRIGGER_CRYPT_PPI HAL_RADIO_RECV_TIMEOUT_CANCEL_PPI

/*******************************************************************************
 * Trigger automatic address resolution on Bit counter match:
 * wire the RADIO EVENTS_BCMATCH event to the AAR TASKS_START task.
 */
#define HAL_TRIGGER_AAR_PPI 12

/*******************************************************************************
 * Trigger Radio Rate override upon Rateboost event.
 */
#define HAL_TRIGGER_RATEOVERRIDE_PPI 13

/******************************************************************************/
#define HAL_ENABLE_PALNA_PPI 5

#if defined(HAL_RADIO_FEM_IS_NRF21540)
#define HAL_DISABLE_PALNA_PPI 4
#else
#define HAL_DISABLE_PALNA_PPI HAL_ENABLE_PALNA_PPI
#endif

#define HAL_ENABLE_FEM_PPI 3
#define HAL_DISABLE_FEM_PPI HAL_DISABLE_PALNA_PPI

/******************************************************************************/
#if !defined(CONFIG_BT_CTLR_TIFS_HW)
/* DPPI setup used for SW-based auto-switching during TIFS. */

/* Clear SW-switch timer on packet end:
 * wire the RADIO EVENTS_END event to SW_SWITCH_TIMER TASKS_CLEAR task.
 *
 * Note: In case of HW TIFS support or single timer configuration we do not need
 * an additional PPI, since we have already set up a PPI to publish RADIO END
 * event. In other case separate PPI is used because packet end is marked by
 * PHYEND event while last bit or CRC is marked by END event.
 */
#if !defined(CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER)
#define HAL_SW_SWITCH_TIMER_CLEAR_PPI 24
#else
#define HAL_SW_SWITCH_TIMER_CLEAR_PPI HAL_RADIO_END_TIME_CAPTURE_PPI
#endif /* !CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER */

/* Wire a SW SWITCH TIMER EVENTS_COMPARE[<cc_offset>] event
 * to a PPI GROUP TASK DISABLE task (PPI group with index <index>).
 * 2 adjacent PPIs (8 & 9) and 2 adjacent PPI groups are used for this wiring;
 * <index> must be 0 or 1. <offset> must be a valid TIMER CC register offset.
 */
#define HAL_SW_SWITCH_GROUP_TASK_DISABLE_PPI_BASE 14

/* Enable the SW Switch PPI Group on RADIO END Event.
 *
 * Note: we do not need an additional PPI, since we have already set up
 * a PPI to publish RADIO END event.
 */
#define HAL_SW_SWITCH_GROUP_TASK_ENABLE_PPI HAL_SW_SWITCH_TIMER_CLEAR_PPI

/* Enable Radio on SW Switch timer event.
 * Wire a SW SWITCH TIMER EVENTS_COMPARE[<cc_offset>] event
 * to a RADIO Enable task (TX or RX).
 *
 * Note:
 * We use the same PPI as for disabling the SW Switch PPI groups,
 * since we need to listen for the same event (SW Switch event).
 *
 * We use the same PPI for the alternative SW Switch Timer compare
 * event.
 */
#define HAL_SW_SWITCH_RADIO_ENABLE_PPI_BASE 14

#define HAL_SW_SWITCH_RADIO_ENABLE_S2_PPI_BASE \
	HAL_SW_SWITCH_RADIO_ENABLE_PPI_BASE

/* Cancel the SW switch timer running considering S8 timing:
 * wire the RADIO EVENTS_RATEBOOST event to SW_SWITCH_TIMER TASKS_CAPTURE task.
 *
 * Note: We already have a PPI where we publish the RATEBOOST event.
 */
#define HAL_SW_SWITCH_TIMER_S8_DISABLE_PPI HAL_TRIGGER_RATEOVERRIDE_PPI

#if defined(CONFIG_BT_CTLR_DF_PHYEND_OFFSET_COMPENSATION_ENABLE)
/* Cancel the SW switch timer running considering PHYEND delay compensation timing:
 * wire the RADIO EVENTS_CTEPRESENT event to SW_SWITCH_TIMER TASKS_CAPTURE task.
 */
#define HAL_SW_SWITCH_TIMER_PHYEND_DELAY_COMPENSATION_DISABLE_PPI 16
#endif /* CONFIG_BT_CTLR_DF_PHYEND_OFFSET_COMPENSATION_ENABLE */

#if defined(CONFIG_BT_CTLR_DF_CONN_CTE_RX)
/* Trigger encryption task upon bit counter match event fire:
 * wire the RADIO EVENTS_BCMATCH event to the CCM TASKS_CRYPT task.
 *
 * Note: The PPI number is shared with HAL_TRIGGER_RATEOVERRIDE_PPI because it is used only
 * when direction finding RX and PHY is set to PHY1M. Due to that it can be shared with Radio Rate
 * override.
 */
#define HAL_TRIGGER_CRYPT_DELAY_PPI 13
#endif /* CONFIG_BT_CTLR_DF_CONN_CTE_RX */

/* The 2 adjacent PPI groups used for implementing SW_SWITCH_TIMER-based
 * auto-switch for TIFS. 'index' must be 0 or 1.
 */
#define SW_SWITCH_TIMER_TASK_GROUP_BASE 0
#endif /* !CONFIG_BT_CTLR_TIFS_HW */

#endif /* CONFIG_SOC_NRF5340_CPUNET || DPPI_PRESENT */
