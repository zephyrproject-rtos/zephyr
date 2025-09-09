/*
 * Copyright 2022 The Chromium OS Authors
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief USB Type-C Cable and Connector API used for USB-C drivers
 *
 * The information in this file was taken from the USB Type-C
 * Cable and Connector Specification Release 2.1
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_USBC_USBC_TC_H_
#define ZEPHYR_INCLUDE_DRIVERS_USBC_USBC_TC_H_

/**
 * @brief Support for USB Type-C cables and connectors
 * @defgroup usb_type_c USB Type-C
 * @ingroup usb_interfaces
 * @{
 */

#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief VBUS minimum for a sink disconnect detection.
 *	  See Table 4-3 VBUS Sink Characteristics
 */
#define TC_V_SINK_DISCONNECT_MIN_MV 800

/**
 * @brief VBUS maximum for a sink disconnect detection.
 *	  See Table 4-3 VBUS Sink Characteristics
 */
#define TC_V_SINK_DISCONNECT_MAX_MV 3670

/**
 * @brief From entry to Attached.SRC until VBUS reaches the minimum vSafe5V threshold as
 *	  measured at the source’s receptacle
 *	  See Table 4-29 VBUS and VCONN Timing Parameters
 */
#define TC_T_VBUS_ON_MAX_MS 275

/**
 * @brief From the time the Sink is detached until the Source removes VBUS and reaches
 *	  vSafe0V (See USB PD).
 *	  See Table 4-29 VBUS and VCONN Timing Parameters
 */
#define TC_T_VBUS_OFF_MAX_MS 650

/**
 * @brief From the time the Source supplied VBUS in the Attached.SRC state.
 *	  See Table 4-29 VBUS and VCONN Timing Parameters
 */
#define TC_T_VCONN_ON_MAX_MS 2

/**
 * @brief From the time a Sink with accessory support enters the PoweredAccessory state
 *	  until the Sink sources minimum VCONN voltage (see Table 4-5)
 *	  See Table 4-29 VBUS and VCONN Timing Parameters
 */
#define TC_T_VCONN_ON_PA_MAX_MS 100

/**
 * @brief From the time that a Sink is detached or as directed until the VCONN supply is
 *	  disconnected.
 *	  See Table 4-29 VBUS and VCONN Timing Parameters
 */
#define TC_T_VCONN_OFF_MAX_MS 35

/**
 * @brief Response time for a Sink to adjust its current consumption to be in the specified
 *	  range due to a change in USB Type-C Current advertisement
 *	  See Table 4-29 VBUS and VCONN Timing Parameters
 */
#define TC_T_SINK_ADJ_MAX_MS 60

/**
 * @brief The minimum period a DRP shall complete a Source to Sink and back advertisement
 *	  See Table 4-30 DRP Timing Parameters
 */
#define TC_T_DRP_MIN_MS 50

/**
 * @brief The maximum period a DRP shall complete a Source to Sink and back advertisement
 *	  See Table 4-30 DRP Timing Parameters
 */
#define TC_T_DRP_MAX_MS 100

/**
 * @brief The minimum time a DRP shall complete transitions between Source and Sink roles
 *	  during role resolution
 *	  See Table 4-30 DRP Timing Parameters
 */
#define TC_T_DRP_TRANSITION_MIN_MS 0

/**
 * @brief The maximum time a DRP shall complete transitions between Source and Sink roles
 *	  during role resolution
 *	  See Table 4-30 DRP Timing Parameters
 */
#define TC_T_DRP_TRANSITION_MAX_MS 1

/**
 * @brief Minimum wait time associated with the Try.SRC state.
 *	  See Table 4-30 DRP Timing Parameters
 */
#define TC_T_DRP_TRY_MIN_MS 75

/**
 * @brief Maximum wait time associated with the Try.SRC state.
 *	  See Table 4-30 DRP Timing Parameters
 */
#define TC_T_DRP_TRY_MAX_MS 150

/**
 * @brief Minimum wait time associated with the Try.SNK state.
 *	  See Table 4-30 DRP Timing Parameters
 */
#define TC_T_DRP_TRY_WAIT_MIN_MS 400

/**
 * @brief Maximum wait time associated with the Try.SNK state.
 *	  See Table 4-30 DRP Timing Parameters
 */
#define TC_T_DRP_TRY_WAIT_MAX_MS 800

/**
 * @brief Minimum timeout for transition from Try.SRC to TryWait.SNK.
 *	  See Table 4-30 DRP Timing Parameters
 */
#define TC_T_TRY_TIMEOUT_MIN_MS 550

/**
 * @brief Maximum timeout for transition from Try.SRC to TryWait.SNK.
 *	  See Table 4-30 DRP Timing Parameters
 */
#define TC_T_TRY_TIMEOUT_MAX_MS 1100

/**
 * @brief Minimum Time for a DRP to detect that the connected Charge-Through VCONNPowered
 *	  USB Device has been detached, after VBUS has been removed.
 *	  See Table 4-30 DRP Timing Parameters
 */
#define TC_T_VPD_DETACH_MIN_MS 10

/**
 * @brief Maximum Time for a DRP to detect that the connected Charge-Through VCONNPowered
 *	  USB Device has been detached, after VBUS has been removed.
 *	  See Table 4-30 DRP Timing Parameters
 */
#define TC_T_VPD_DETACH_MAX_MS 20

/**
 * @brief Minimum time a port shall wait before it can determine it is attached
 *	  See Table 4-31 CC Timing
 */
#define TC_T_CC_DEBOUNCE_MIN_MS 100

/**
 * @brief Maximum time a port shall wait before it can determine it is attached
 *	  See Table 4-31 CC Timing
 */
#define TC_T_CC_DEBOUNCE_MAX_MS 200

/**
 * @brief Minimum time a Sink port shall wait before it can determine it is detached due to
 *	  the potential for USB PD signaling on CC as described in the state definitions.
 *	  See Table 4-31 CC Timing
 */
#define TC_T_PD_DEBOUNCE_MIN_MS 10

/**
 * @brief Maximum time a Sink port shall wait before it can determine it is detached due to
 *	  the potential for USB PD signaling on CC as described in the state definitions.
 *	  See Table 4-31 CC Timing
 */
#define TC_T_PD_DEBOUNCE_MAX_MS 20

/**
 * @brief Minimum Time a port shall wait before it can determine it is re-attached during
 *	  the try-wait process.
 *	  See Table 4-31 CC Timing
 */
#define TC_T_TRY_CC_DEBOUNCE_MIN_MS 10

/**
 * @brief Maximum Time a port shall wait before it can determine it is re-attached during
 *	  the try-wait process.
 *	  See Table 4-31 CC Timing
 */
#define TC_T_TRY_CC_DEBOUNCE_MAX_MS 10

/**
 * @brief Minimum time a self-powered port shall remain in the ErrorRecovery state.
 *	  See Table 4-31 CC Timing
 */
#define TC_T_ERROR_RECOVERY_SELF_POWERED_MIN_MS 25

/**
 * @brief Minimum time a source shall remain in the ErrorRecovery state if it was sourcing
 *	  VCONN in the previous state.
 *	  See Table 4-31 CC Timing
 */
#define TC_T_ERROR_RECOVERY_SOURCE_MIN_MS 240

/**
 * @brief Minimum time a Sink port shall wait before it can determine there has been a change
 *	  in Rp where CC is not BMC Idle or the port is unable to detect BMC Idle.
 *	  See Table 4-31 CC Timing
 */
#define TC_T_RP_VALUE_CHANGE_MIN_MS 10

/**
 * @brief Maximum time a Sink port shall wait before it can determine there has been a change
 *	  in Rp where CC is not BMC Idle or the port is unable to detect BMC Idle.
 *	  See Table 4-31 CC Timing
 */
#define TC_T_RP_VALUE_CHANGE_MAX_MS 20

/**
 * @brief Minimum time a Source shall detect the SRC.Open state. The Source should detect the
 *	  SRC.Open state as quickly as practical.
 *	  See Table 4-31 CC Timing
 */
#define TC_T_SRC_DISCONNECT_MIN_MS 0

/**
 * @brief Maximum time a Source shall detect the SRC.Open state. The Source should detect the
 *	  SRC.Open state as quickly as practical.
 *	  See Table 4-31 CC Timing
 */
#define TC_T_SRC_DISCONNECT_MAX_MS 20

/**
 * @brief Minimum time to detect connection when neither Port Partner is toggling.
 *	  See Table 4-31 CC Timing
 */
#define TC_T_NO_TOGGLE_CONNECT_MIN_MS 0

/**
 * @brief Maximum time to detect connection when neither Port Partner is toggling.
 *	  See Table 4-31 CC Timing
 */
#define TC_T_NO_TOGGLE_CONNECT_MAX_MS 5

/**
 * @brief Minimum time to detect connection when one Port Partner is toggling
 *	  0ms … dcSRC.DRP max * tDRP max + 2 * tNoToggleConnect).
 *	  See Table 4-31 CC Timing
 */
#define TC_T_ONE_PORT_TOGGLE_CONNECT_MIN_MS 0

/**
 * @brief Maximum time to detect connection when one Port Partner is toggling
 *	  0ms … dcSRC.DRP max * tDRP max + 2 * tNoToggleConnect).
 *	  See Table 4-31 CC Timing
 */
#define TC_T_ONE_PORT_TOGGLE_CONNECT_MAX_MS 80

/**
 * @brief Minimum time to detect connection when both Port Partners are toggling
 *	  (0ms … 5 * tDRP max + 2 * tNoToggleConnect).
 *	  See Table 4-31 CC Timing
 */
#define TC_T_TWO_PORT_TOGGLE_CONNECT_MIN_MS 0

/**
 * @brief Maximum time to detect connection when both Port Partners are toggling
 *	  (0ms … 5 * tDRP max + 2 * tNoToggleConnect).
 *	  See Table 4-31 CC Timing
 */
#define TC_T_TWO_PORT_TOGGLE_CONNECT_MAX_MS 510

/**
 * @brief Minimum time for a Charge-Through VCONN-Powered USB Device to detect that the
 *	  Charge-Through source has disconnected from CC after VBUS has been removed,
 *	  transition to CTUnattached.VPD, and re-apply its Rp termination advertising
 *	  3.0 A on the host port CC.
 *	  See Table 4-31 CC Timing
 */
#define TC_T_VPDCTDD_MIN_US 30

/**
 * @brief Maximum time for a Charge-Through VCONN-Powered USB Device to detect that the
 *	  Charge-Through source has disconnected from CC after VBUS has been removed,
 *	  transition to CTUnattached.VPD, and re-apply its Rp termination advertising
 *	  3.0 A on the host port CC.
 *	  See Table 4-31 CC Timing
 */
#define TC_T_VPDCTDD_MAX_MS 5

/**
 * @brief Minimum time for a Charge-Through VCONN-Powered USB Device shall remain
 *	  in CTDisabled.VPD state.
 *	  See Table 4-31 CC Timing
 */
#define TC_T_VPDDISABLE_MIN_MS 25

/**
 * @brief CC Voltage status
 */
enum tc_cc_voltage_state {
	/** No port partner connection */
	TC_CC_VOLT_OPEN         = 0,
	/** Port partner is applying Ra */
	TC_CC_VOLT_RA           = 1,
	/** Port partner is applying Rd */
	TC_CC_VOLT_RD           = 2,
	/** Port partner is applying Rp (0.5A) */
	TC_CC_VOLT_RP_DEF       = 5,
	/*8 Port partner is applying Rp (1.5A) */
	TC_CC_VOLT_RP_1A5       = 6,
	/** Port partner is applying Rp (3.0A) */
	TC_CC_VOLT_RP_3A0       = 7,
};

/**
 * @brief VBUS level voltages
 */
enum tc_vbus_level {
	/** VBUS is less than vSafe0V max */
	TC_VBUS_SAFE0V  = 0,
	/** VBUS is at least vSafe5V min */
	TC_VBUS_PRESENT = 1,
	/** VBUS is less than vSinkDisconnect max */
	TC_VBUS_REMOVED = 2
};

/**
 * @brief Pull-Up resistor values
 */
enum tc_rp_value {
	/** Pull-Up resistor for a current of 900mA */
	TC_RP_USB       = 0,
	/** Pull-Up resistor for a current of 1.5A */
	TC_RP_1A5       = 1,
	/** Pull-Up resistor for a current of 3.0A */
	TC_RP_3A0       = 2,
	/** No Pull-Up resistor is applied */
	TC_RP_RESERVED  = 3
};

/**
 * @brief CC pull resistors
 */
enum tc_cc_pull {
	/** Ra Pull-Down resistor */
	TC_CC_RA        = 0,
	/** Rp Pull-Up resistor */
	TC_CC_RP        = 1,
	/** Rd Pull-Down resistor */
	TC_CC_RD        = 2,
	/** No CC resistor */
	TC_CC_OPEN      = 3,
	/** Ra and Rd Pull-Down resistor */
	TC_RA_RD        = 4
};

/**
 * @brief Cable plug. See 6.2.1.1.7 Cable Plug. Only applies to SOP' and SOP".
 *	  Replaced by pd_power_role for SOP packets.
 */
enum tc_cable_plug {
	/* Message originated from a DFP or UFP */
	PD_PLUG_FROM_DFP_UFP = 0,
	/* Message originated from a Cable Plug or VPD */
	PD_PLUG_FROM_CABLE_VPD = 1
};

/**
 * @brief Power Delivery Power Role
 */
enum tc_power_role {
	/** Power role is a sink */
	TC_ROLE_SINK    = 0,
	/** Power role is a source */
	TC_ROLE_SOURCE  = 1
};

/**
 * @brief Power Delivery Data Role
 */
enum tc_data_role {
	/** Data role is an Upstream Facing Port */
	TC_ROLE_UFP             = 0,
	/** Data role is a Downstream Facing Port */
	TC_ROLE_DFP             = 1,
	/** Port is disconnected */
	TC_ROLE_DISCONNECTED    = 2
};

/**
 * @brief Polarity of the CC lines
 */
enum tc_cc_polarity {
	/** Use CC1 IO for Power Delivery communication */
	TC_POLARITY_CC1 = 0,
	/** Use CC2 IO for Power Delivery communication */
	TC_POLARITY_CC2 = 1
};

/**
 * @brief Possible port partner connections based on CC line states
 */
enum tc_cc_states {
	/** No port partner attached */
	TC_CC_NONE = 0,

	/** From DFP perspective */

	/** No UFP accessory connected */
	TC_CC_UFP_NONE          = 1,
	/** UFP Audio accessory connected */
	TC_CC_UFP_AUDIO_ACC     = 2,
	/** UFP Debug accessory connected */
	TC_CC_UFP_DEBUG_ACC     = 3,
	/** Plain UFP attached */
	TC_CC_UFP_ATTACHED      = 4,

	/** From UFP perspective */

	/** Plain DFP attached */
	TC_CC_DFP_ATTACHED      = 5,
	/** DFP debug accessory connected */
	TC_CC_DFP_DEBUG_ACC     = 6
};

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_USBC_USBC_TC_H_ */
