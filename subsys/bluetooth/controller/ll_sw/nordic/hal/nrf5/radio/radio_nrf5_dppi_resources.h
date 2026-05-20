/*
 * Copyright (c) 2021-2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*******************************************************************************
 * Enable Radio on Event Timer tick:
 * wire the EVENT_TIMER EVENTS_COMPARE[0] event to RADIO TASKS_TXEN/RXEN task.
 */
#define HAL_RADIO_ENABLE_TX_ON_TICK_PPI 9
#define HAL_RADIO_ENABLE_RX_ON_TICK_PPI 9

/*******************************************************************************
 * Capture event timer on Address reception:
 * wire the RADIO EVENTS_ADDRESS event to the
 * EVENT_TIMER TASKS_CAPTURE[<address timer>] task.
 */
#define HAL_RADIO_RECV_TIMEOUT_CANCEL_PPI 11

/*******************************************************************************
 * Disable Radio on HCTO:
 * wire the EVENT_TIMER EVENTS_COMPARE[<HCTO timer>] event
 * to the RADIO TASKS_DISABLE task.
 */
#define HAL_RADIO_DISABLE_ON_HCTO_PPI 12

/*******************************************************************************
 * Capture event timer on Radio end:
 * wire the RADIO EVENTS_END event to the
 * EVENT_TIMER TASKS_CAPTURE[<radio end timer>] task.
 */
#define HAL_RADIO_END_TIME_CAPTURE_PPI 13

/*******************************************************************************
 * Start event timer on RTC tick:
 * wire the RTC0 EVENTS_COMPARE[2] event to EVENT_TIMER  TASKS_START task.
 */
#define HAL_EVENT_TIMER_START_PPI 8
#define HAL_PPIB_SEND_EVENT_TIMER_START_PPI \
	_CONCAT(NRF_PPIB_TASK_SEND_, HAL_EVENT_TIMER_START_PPI)
#define HAL_PPIB_RECEIVE_EVENT_TIMER_START_PPI \
	_CONCAT(NRF_PPIB_EVENT_RECEIVE_, HAL_EVENT_TIMER_START_PPI)

/*******************************************************************************
 * Capture event timer on Radio ready:
 * wire the RADIO EVENTS_READY event to the
 * EVENT_TIMER TASKS_CAPTURE[<radio ready timer>] task.
 */
#define HAL_RADIO_READY_TIME_CAPTURE_PPI 10

/*******************************************************************************
 * Trigger encryption task upon address reception:
 * wire the RADIO EVENTS_ADDRESS event to the CCM TASKS_CRYPT task.
 *
 * Note: we do not need an additional PPI, since we have already set up
 * a PPI to publish RADIO ADDRESS event.
 * Note: For nRF54L, CCM is started on RADIO PAYLOAD, hence need an explicit
 * PPI channel.
 */
#if defined(CONFIG_SOC_COMPATIBLE_NRF54LX)
#define HAL_TRIGGER_CRYPT_PPI 7
#define HAL_PPIB_SEND_TRIGGER_CRYPT_PPI \
	_CONCAT(NRF_PPIB_TASK_SEND_, HAL_TRIGGER_CRYPT_PPI)
#define HAL_PPIB_RECEIVE_TRIGGER_CRYPT_PPI \
	_CONCAT(NRF_PPIB_EVENT_RECEIVE_, HAL_TRIGGER_CRYPT_PPI)
#else /* !CONFIG_SOC_COMPATIBLE_NRF54LX */
#define HAL_TRIGGER_CRYPT_PPI HAL_RADIO_RECV_TIMEOUT_CANCEL_PPI
#endif /* !CONFIG_SOC_COMPATIBLE_NRF54LX */

/*******************************************************************************
 * Trigger automatic address resolution on Bit counter match:
 * wire the RADIO EVENTS_BCMATCH event to the AAR TASKS_START task.
 */
#define HAL_TRIGGER_AAR_PPI 6
#if defined(CONFIG_SOC_COMPATIBLE_NRF54LX)
#define HAL_PPIB_SEND_TRIGGER_AAR_PPI \
	_CONCAT(NRF_PPIB_TASK_SEND_, HAL_TRIGGER_AAR_PPI)
#define HAL_PPIB_RECEIVE_TRIGGER_AAR_PPI \
	_CONCAT(NRF_PPIB_EVENT_RECEIVE_, HAL_TRIGGER_AAR_PPI)
#endif /* CONFIG_SOC_COMPATIBLE_NRF54LX */

#if defined(CONFIG_BT_CTLR_PHY_CODED) && \
	defined(CONFIG_HAS_HW_NRF_RADIO_BLE_CODED)
/*******************************************************************************
 * Trigger Radio Rate override upon Rateboost event.
 */
#define HAL_TRIGGER_RATEOVERRIDE_PPI 3
#endif /* CONFIG_BT_CTLR_PHY_CODED && CONFIG_HAS_HW_NRF_RADIO_BLE_CODED */

#if defined(HAL_RADIO_GPIO_HAVE_PA_PIN) || defined(HAL_RADIO_GPIO_HAVE_LNA_PIN)
/******************************************************************************/
#define HAL_ENABLE_PALNA_PPI 2

#if defined(HAL_RADIO_FEM_IS_NRF21540)
#define HAL_DISABLE_PALNA_PPI 0
#else
#define HAL_DISABLE_PALNA_PPI HAL_ENABLE_PALNA_PPI
#endif

#define HAL_ENABLE_FEM_PPI 1
#define HAL_DISABLE_FEM_PPI HAL_DISABLE_PALNA_PPI

#endif /* HAL_RADIO_GPIO_HAVE_PA_PIN || HAL_RADIO_GPIO_HAVE_LNA_PIN */

/******************************************************************************/
#if !defined(CONFIG_BT_CTLR_TIFS_HW)
/* DPPI setup used for SW-based auto-switching during TIFS. */

/* Clear SW-switch timer on packet end:
 * wire the RADIO EVENTS_END event to SW_SWITCH_TIMER TASKS_CLEAR task.
 *
 * Note: In case of single timer configuration we do not need an additional PPI,
 * since we have already set up a PPI to publish RADIO END event. In other case
 * separate PPI is used because packet end is marked by PHYEND event while last
 * bit or CRC is marked by END event.
 */
#if defined(CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER)
#define HAL_SW_SWITCH_TIMER_CLEAR_PPI HAL_RADIO_END_TIME_CAPTURE_PPI
#else /* !CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER */
#define HAL_SW_SWITCH_TIMER_CLEAR_PPI 5
#endif /* !CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER */

/* Wire a SW SWITCH TIMER EVENTS_COMPARE[<cc_offset>] event
 * to a PPI GROUP TASK DISABLE task (PPI group with index <index>).
 * 2 adjacent PPIs (14 & 15) and 2 adjacent PPI groups are used for this wiring;
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

#if defined(CONFIG_BT_CTLR_PHY_CODED) && \
	defined(CONFIG_HAS_HW_NRF_RADIO_BLE_CODED)

#define HAL_SW_SWITCH_RADIO_ENABLE_S2_PPI_BASE \
	HAL_SW_SWITCH_RADIO_ENABLE_PPI_BASE

/* Cancel the SW switch timer running considering S8 timing:
 * wire the RADIO EVENTS_RATEBOOST event to SW_SWITCH_TIMER TASKS_CAPTURE task.
 *
 * Note: We already have a PPI where we publish the RATEBOOST event.
 */
#define HAL_SW_SWITCH_TIMER_S8_DISABLE_PPI HAL_TRIGGER_RATEOVERRIDE_PPI

#endif /* CONFIG_BT_CTLR_PHY_CODED && CONFIG_HAS_HW_NRF_RADIO_BLE_CODED */

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
#define HAL_TRIGGER_CRYPT_DELAY_PPI 3
#endif /* CONFIG_BT_CTLR_DF_CONN_CTE_RX */

/* The 2 adjacent PPI groups used for implementing SW_SWITCH_TIMER-based
 * auto-switch for TIFS. 'index' must be 0 or 1.
 */
#define SW_SWITCH_TIMER_TASK_GROUP_BASE 4

#if defined(CONFIG_SOC_COMPATIBLE_NRF54LX) && !defined(CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER)
/*******************************************************************************
 * nRF54L dual-timer mode: EVENT_TIMER = NRF_TIMER00 (MCU power domain, DPPIC00)
 *                          SW_SWITCH_TIMER = NRF_TIMER10 (Radio power domain, DPPIC10)
 *
 * Cross-domain EVENT_TIMER connections use two PPIB bridges:
 *   PPIB01/PPIB20: Peripheral domain (DPPIC20) -> MCU domain (DPPIC00)
 *                  Used for GRTC -> NRF_TIMER00 START.
 *   PPIB00/PPIB10: MCU domain (DPPIC00) <-> Radio domain (DPPIC10)
 *                  Used for NRF_TIMER00 <-> Radio connections.
 *
 * DPPIC00 has 8 channels (0-7). Channels 6 and 7 are used by AAR and CCM
 * respectively (existing). Channels 0-5 are assigned here for EVENT_TIMER.
 * PPIB00/PPIB10 bridge channels 1-5 are used for EVENT_TIMER <-> Radio.
 * PPIB01/PPIB20 bridge channel 0 is used for GRTC -> EVENT_TIMER.
 ******************************************************************************/

/* GRTC (Peripheral domain, DPPIC20 ch HAL_EVENT_TIMER_START_PPI)
 * -> NRF_TIMER00 START (MCU domain, DPPIC00 ch 0):
 * PPIB20 SEND_0 subscribes to DPPIC20 ch HAL_EVENT_TIMER_START_PPI,
 * PPIB01 RECEIVE_0 publishes to DPPIC00 ch 0.
 */
#define HAL_NRF54LX_DUAL_TIMER_EVENT_TIMER_START_DPPIC00      0
#define HAL_PPIB_SEND_EVENT_TIMER_START_DPPIC00 \
	_CONCAT(NRF_PPIB_TASK_SEND_, HAL_NRF54LX_DUAL_TIMER_EVENT_TIMER_START_DPPIC00)
#define HAL_PPIB_RECEIVE_EVENT_TIMER_START_DPPIC00 \
	_CONCAT(NRF_PPIB_EVENT_RECEIVE_, HAL_NRF54LX_DUAL_TIMER_EVENT_TIMER_START_DPPIC00)

/* NRF_TIMER00 COMPARE (MCU domain, DPPIC00 ch 1)
 * -> Radio TASKS_TXEN/RXEN (Radio domain, DPPIC10 ch HAL_RADIO_ENABLE_TX/RX_ON_TICK_PPI):
 * PPIB00 SEND_1 subscribes to DPPIC00 ch 1,
 * PPIB10 RECEIVE_1 publishes to DPPIC10 ch HAL_RADIO_ENABLE_TX_ON_TICK_PPI.
 */
#define HAL_NRF54LX_DUAL_TIMER_RADIO_ENABLE_ON_TICK_DPPIC00   1
#define HAL_PPIB_SEND_RADIO_ENABLE_ON_TICK_DPPIC00 \
	_CONCAT(NRF_PPIB_TASK_SEND_, HAL_NRF54LX_DUAL_TIMER_RADIO_ENABLE_ON_TICK_DPPIC00)
#define HAL_PPIB_RECEIVE_RADIO_ENABLE_ON_TICK_DPPIC00 \
	_CONCAT(NRF_PPIB_EVENT_RECEIVE_, HAL_NRF54LX_DUAL_TIMER_RADIO_ENABLE_ON_TICK_DPPIC00)

/* Radio EVENTS_READY (Radio domain, DPPIC10 ch HAL_RADIO_READY_TIME_CAPTURE_PPI)
 * -> NRF_TIMER00 TASKS_CAPTURE (MCU domain, DPPIC00 ch 2):
 * PPIB10 SEND_2 subscribes to DPPIC10 ch HAL_RADIO_READY_TIME_CAPTURE_PPI,
 * PPIB00 RECEIVE_2 publishes to DPPIC00 ch 2.
 */
#define HAL_NRF54LX_DUAL_TIMER_RADIO_READY_CAPTURE_DPPIC00    2
#define HAL_PPIB_SEND_RADIO_READY_CAPTURE_DPPIC00 \
	_CONCAT(NRF_PPIB_TASK_SEND_, HAL_NRF54LX_DUAL_TIMER_RADIO_READY_CAPTURE_DPPIC00)
#define HAL_PPIB_RECEIVE_RADIO_READY_CAPTURE_DPPIC00 \
	_CONCAT(NRF_PPIB_EVENT_RECEIVE_, HAL_NRF54LX_DUAL_TIMER_RADIO_READY_CAPTURE_DPPIC00)

/* Radio EVENTS_ADDRESS (Radio domain, DPPIC10 ch HAL_RADIO_RECV_TIMEOUT_CANCEL_PPI)
 * -> NRF_TIMER00 TASKS_CAPTURE (MCU domain, DPPIC00 ch 3):
 * PPIB10 SEND_3 subscribes to DPPIC10 ch HAL_RADIO_RECV_TIMEOUT_CANCEL_PPI,
 * PPIB00 RECEIVE_3 publishes to DPPIC00 ch 3.
 */
#define HAL_NRF54LX_DUAL_TIMER_RECV_TIMEOUT_CANCEL_DPPIC00    3
#define HAL_PPIB_SEND_RECV_TIMEOUT_CANCEL_DPPIC00 \
	_CONCAT(NRF_PPIB_TASK_SEND_, HAL_NRF54LX_DUAL_TIMER_RECV_TIMEOUT_CANCEL_DPPIC00)
#define HAL_PPIB_RECEIVE_RECV_TIMEOUT_CANCEL_DPPIC00 \
	_CONCAT(NRF_PPIB_EVENT_RECEIVE_, HAL_NRF54LX_DUAL_TIMER_RECV_TIMEOUT_CANCEL_DPPIC00)

/* NRF_TIMER00 COMPARE (MCU domain, DPPIC00 ch 4)
 * -> Radio TASKS_DISABLE (HCTO) (Radio domain, DPPIC10 ch HAL_RADIO_DISABLE_ON_HCTO_PPI):
 * PPIB00 SEND_4 subscribes to DPPIC00 ch 4,
 * PPIB10 RECEIVE_4 publishes to DPPIC10 ch HAL_RADIO_DISABLE_ON_HCTO_PPI.
 */
#define HAL_NRF54LX_DUAL_TIMER_RADIO_DISABLE_ON_HCTO_DPPIC00  4
#define HAL_PPIB_SEND_RADIO_DISABLE_ON_HCTO_DPPIC00 \
	_CONCAT(NRF_PPIB_TASK_SEND_, HAL_NRF54LX_DUAL_TIMER_RADIO_DISABLE_ON_HCTO_DPPIC00)
#define HAL_PPIB_RECEIVE_RADIO_DISABLE_ON_HCTO_DPPIC00 \
	_CONCAT(NRF_PPIB_EVENT_RECEIVE_, HAL_NRF54LX_DUAL_TIMER_RADIO_DISABLE_ON_HCTO_DPPIC00)

/* Radio EVENTS_END (Radio domain, DPPIC10 ch HAL_RADIO_END_TIME_CAPTURE_PPI)
 * -> NRF_TIMER00 TASKS_CAPTURE (MCU domain, DPPIC00 ch 5):
 * PPIB10 SEND_5 subscribes to DPPIC10 ch HAL_RADIO_END_TIME_CAPTURE_PPI,
 * PPIB00 RECEIVE_5 publishes to DPPIC00 ch 5.
 */
#define HAL_NRF54LX_DUAL_TIMER_END_TIME_CAPTURE_DPPIC00       5
#define HAL_PPIB_SEND_END_TIME_CAPTURE_DPPIC00 \
	_CONCAT(NRF_PPIB_TASK_SEND_, HAL_NRF54LX_DUAL_TIMER_END_TIME_CAPTURE_DPPIC00)
#define HAL_PPIB_RECEIVE_END_TIME_CAPTURE_DPPIC00 \
	_CONCAT(NRF_PPIB_EVENT_RECEIVE_, HAL_NRF54LX_DUAL_TIMER_END_TIME_CAPTURE_DPPIC00)

/* Bitmask of DPPIC00 channels used by EVENT_TIMER in dual-timer mode */
#define HAL_NRF54LX_DUAL_TIMER_DPPIC00_CHANNELS_USED \
	(BIT(HAL_NRF54LX_DUAL_TIMER_EVENT_TIMER_START_DPPIC00) | \
	 BIT(HAL_NRF54LX_DUAL_TIMER_RADIO_ENABLE_ON_TICK_DPPIC00) | \
	 BIT(HAL_NRF54LX_DUAL_TIMER_RADIO_READY_CAPTURE_DPPIC00) | \
	 BIT(HAL_NRF54LX_DUAL_TIMER_RECV_TIMEOUT_CANCEL_DPPIC00) | \
	 BIT(HAL_NRF54LX_DUAL_TIMER_RADIO_DISABLE_ON_HCTO_DPPIC00) | \
	 BIT(HAL_NRF54LX_DUAL_TIMER_END_TIME_CAPTURE_DPPIC00))

/* Bitmask of PPIB01/PPIB20 bridge channels used for dual-timer
 * (Peripheral domain <-> MCU domain, for GRTC -> NRF_TIMER00 START).
 */
#define HAL_NRF54LX_DUAL_TIMER_PPIB_01_20_CHANNELS_USED \
	BIT(HAL_NRF54LX_DUAL_TIMER_EVENT_TIMER_START_DPPIC00)

/* Bitmask of PPIB00/PPIB10 bridge channels used for dual-timer
 * EVENT_TIMER <-> Radio connections (MCU domain <-> Radio domain).
 */
#define HAL_NRF54LX_DUAL_TIMER_PPIB_00_10_CHANNELS_USED \
	(BIT(HAL_NRF54LX_DUAL_TIMER_RADIO_ENABLE_ON_TICK_DPPIC00) | \
	 BIT(HAL_NRF54LX_DUAL_TIMER_RADIO_READY_CAPTURE_DPPIC00) | \
	 BIT(HAL_NRF54LX_DUAL_TIMER_RECV_TIMEOUT_CANCEL_DPPIC00) | \
	 BIT(HAL_NRF54LX_DUAL_TIMER_RADIO_DISABLE_ON_HCTO_DPPIC00) | \
	 BIT(HAL_NRF54LX_DUAL_TIMER_END_TIME_CAPTURE_DPPIC00))

#endif /* CONFIG_SOC_COMPATIBLE_NRF54LX && !CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER */
#endif /* !CONFIG_BT_CTLR_TIFS_HW */
