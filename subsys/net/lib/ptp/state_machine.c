/*
 * Copyright (c) 2024 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include "state_machine.h"

enum ptp_port_state ptp_state_machine(enum ptp_port_state state,
				      enum ptp_port_event event,
				      bool tt_diff)
{
	enum ptp_port_state new_state = state;

	if (event == PTP_EVT_INITIALIZE || event == PTP_EVT_POWERUP) {
		/* initialize port data sets, HW and communication facilities */
		return PTP_PS_INITIALIZING;
	}

	switch (state) {
	case PTP_PS_INITIALIZING:
		switch (event) {
		case PTP_EVT_FAULT_DETECTED:
			new_state = IS_ENABLED(CONFIG_PTP_FAULTY_PRESENT) ?
					PTP_PS_FAULTY : new_state;
			break;
		case PTP_EVT_INIT_COMPLETE:
			new_state = IS_ENABLED(CONFIG_PTP_LISTENING_PRESENT) ?
					PTP_PS_LISTENING : PTP_PS_TIME_TRANSMITTER;
			break;
		default:
			break;
		}
		break;
#if CONFIG_PTP_FAULTY_PRESENT
	case PTP_PS_FAULTY:
		switch (event) {
		case PTP_EVT_DESIGNATED_DISABLED:
			new_state = IS_ENABLED(CONFIG_PTP_DISABLED_PRESENT) ?
					PTP_PS_DISABLED : new_state;
			break;
		case PTP_EVT_FAULT_CLEARED:
			new_state = PTP_PS_INITIALIZING;
			break;
		default:
			break;
		}
		break;
#endif /* CONFIG_PTP_FAULTY_PRESENT */
#if CONFIG_PTP_DISABLED_PRESENT
	case PTP_PS_DISABLED:
		if (event == PTP_EVT_DESIGNATED_ENABLED) {
			new_state = PTP_PS_INITIALIZING;
		}
		break;
#endif /* CONFIG_PTP_DISABLED_PRESENT */
#if CONFIG_PTP_LISTENING_PRESENT
	case PTP_PS_LISTENING:
		switch (event) {
		case PTP_EVT_DESIGNATED_DISABLED:
			new_state = IS_ENABLED(CONFIG_PTP_DISABLED_PRESENT) ?
					PTP_PS_DISABLED : new_state;
			break;
		case PTP_EVT_FAULT_DETECTED:
			new_state = IS_ENABLED(CONFIG_PTP_FAULTY_PRESENT) ?
					PTP_PS_FAULTY : new_state;
			break;
		case PTP_EVT_ANNOUNCE_RECEIPT_TIMEOUT_EXPIRES:
			new_state = PTP_PS_TIME_TRANSMITTER;
			break;
		case PTP_EVT_RS_TIME_TRANSMITTER:
			new_state = IS_ENABLED(CONFIG_PTP_PRE_TIME_TRANSMITTER_PRESENT) ?
					PTP_PS_PRE_TIME_TRANSMITTER : PTP_PS_TIME_TRANSMITTER;
			break;
		case PTP_EVT_RS_GRAND_MASTER:
			new_state = PTP_PS_GRAND_MASTER;
			break;
		case PTP_EVT_RS_PASSIVE:
			new_state = PTP_PS_PASSIVE;
			break;
		case PTP_EVT_RS_TIME_RECEIVER:
			new_state = IS_ENABLED(CONFIG_PTP_UNCALIBRATED_PRESENT) ?
					PTP_PS_UNCALIBRATED : PTP_PS_TIME_RECEIVER;
			break;
		default:
			break;
		}
		break;
#endif /* CONFIG_PTP_LISTENING_PRESENT */
#if CONFIG_PTP_PRE_TIME_TRANSMITTER_PRESENT
	case PTP_PS_PRE_TIME_TRANSMITTER:
		switch (event) {
		case PTP_EVT_DESIGNATED_DISABLED:
			new_state = IS_ENABLED(CONFIG_PTP_DISABLED_PRESENT) ?
					PTP_PS_DISABLED : new_state;
			break;
		case PTP_EVT_FAULT_DETECTED:
			new_state = IS_ENABLED(CONFIG_PTP_FAULTY_PRESENT) ?
					PTP_PS_FAULTY : new_state;
			break;
		case PTP_EVT_QUALIFICATION_TIMEOUT_EXPIRES:
			new_state = PTP_PS_TIME_TRANSMITTER;
			break;
		case PTP_EVT_RS_PASSIVE:
			new_state = PTP_PS_PASSIVE;
			break;
		case PTP_EVT_RS_TIME_RECEIVER:
			new_state = IS_ENABLED(CONFIG_PTP_UNCALIBRATED_PRESENT) ?
					PTP_PS_UNCALIBRATED : PTP_PS_TIME_RECEIVER;
			break;
		default:
			break;
		}
		break;
#endif /* CONFIG_PTP_PRE_TIME_TRANSMITTER_PRESENT */
	case PTP_PS_TIME_TRANSMITTER:
		switch (event) {
		case PTP_EVT_DESIGNATED_DISABLED:
			new_state = IS_ENABLED(CONFIG_PTP_DISABLED_PRESENT) ?
					PTP_PS_DISABLED : new_state;
			break;
		case PTP_EVT_FAULT_DETECTED:
			new_state = IS_ENABLED(CONFIG_PTP_FAULTY_PRESENT) ?
					PTP_PS_FAULTY : new_state;
			break;
		case PTP_EVT_RS_PASSIVE:
			new_state = PTP_PS_PASSIVE;
			break;
		case PTP_EVT_RS_TIME_RECEIVER:
			new_state = IS_ENABLED(CONFIG_PTP_UNCALIBRATED_PRESENT) ?
					PTP_PS_UNCALIBRATED : PTP_PS_TIME_RECEIVER;
			break;
		default:
			break;
		}
		break;
	case PTP_PS_PASSIVE:
		switch (event) {
		case PTP_EVT_DESIGNATED_DISABLED:
			new_state = IS_ENABLED(CONFIG_PTP_DISABLED_PRESENT) ?
					PTP_PS_DISABLED : new_state;
			break;
		case PTP_EVT_FAULT_DETECTED:
			new_state = IS_ENABLED(CONFIG_PTP_FAULTY_PRESENT) ?
					PTP_PS_FAULTY : new_state;
			break;
		case PTP_EVT_RS_TIME_TRANSMITTER:
			new_state = IS_ENABLED(CONFIG_PTP_PRE_TIME_TRANSMITTER_PRESENT) ?
					PTP_PS_PRE_TIME_TRANSMITTER : PTP_PS_TIME_TRANSMITTER;
			break;
		case PTP_EVT_RS_GRAND_MASTER:
			new_state = PTP_PS_GRAND_MASTER;
			break;
		case PTP_EVT_RS_TIME_RECEIVER:
			new_state = IS_ENABLED(CONFIG_PTP_UNCALIBRATED_PRESENT) ?
					PTP_PS_UNCALIBRATED : PTP_PS_TIME_RECEIVER;
			break;
		default:
			break;
		}
		break;
#if CONFIG_PTP_UNCALIBRATED_PRESENT
	case PTP_PS_UNCALIBRATED:
		switch (event) {
		case PTP_EVT_DESIGNATED_DISABLED:
			new_state = IS_ENABLED(CONFIG_PTP_DISABLED_PRESENT) ?
					PTP_PS_DISABLED : new_state;
			break;
		case PTP_EVT_FAULT_DETECTED:
			new_state = IS_ENABLED(CONFIG_PTP_FAULTY_PRESENT) ?
					PTP_PS_FAULTY : new_state;
			break;
		case PTP_EVT_ANNOUNCE_RECEIPT_TIMEOUT_EXPIRES:
			new_state = PTP_PS_TIME_TRANSMITTER;
			break;
		case PTP_EVT_TIME_TRANSMITTER_CLOCK_SELECTED:
			new_state = PTP_PS_TIME_RECEIVER;
			break;
		case PTP_EVT_RS_TIME_TRANSMITTER:
			new_state = IS_ENABLED(CONFIG_PTP_PRE_TIME_TRANSMITTER_PRESENT) ?
					PTP_PS_PRE_TIME_TRANSMITTER : PTP_PS_TIME_TRANSMITTER;
			break;
		case PTP_EVT_RS_GRAND_MASTER:
			new_state = PTP_PS_GRAND_MASTER;
			break;
		case PTP_EVT_RS_PASSIVE:
			new_state = PTP_PS_PASSIVE;
			break;
		default:
			break;
		}
		break;
#endif /* CONFIG_PTP_UNCALIBRATED_PRESENT */
	case PTP_PS_TIME_RECEIVER:
		switch (event) {
		case PTP_EVT_DESIGNATED_DISABLED:
			new_state = IS_ENABLED(CONFIG_PTP_DISABLED_PRESENT) ?
					PTP_PS_DISABLED : new_state;
			break;
		case PTP_EVT_FAULT_DETECTED:
			new_state = IS_ENABLED(CONFIG_PTP_FAULTY_PRESENT) ?
					PTP_PS_FAULTY : new_state;
			break;
		case PTP_EVT_SYNCHRONIZATION_FAULT:
			new_state = IS_ENABLED(CONFIG_PTP_UNCALIBRATED_PRESENT) ?
					PTP_PS_UNCALIBRATED : new_state;
			break;
		case PTP_EVT_ANNOUNCE_RECEIPT_TIMEOUT_EXPIRES:
			new_state = PTP_PS_TIME_TRANSMITTER;
			break;
		case PTP_EVT_RS_TIME_TRANSMITTER:
			new_state = IS_ENABLED(CONFIG_PTP_PRE_TIME_TRANSMITTER_PRESENT) ?
					PTP_PS_PRE_TIME_TRANSMITTER : PTP_PS_TIME_TRANSMITTER;
			break;
		case PTP_EVT_RS_GRAND_MASTER:
			new_state = PTP_PS_GRAND_MASTER;
			break;
		case PTP_EVT_RS_PASSIVE:
			new_state = PTP_PS_PASSIVE;
			break;
		case PTP_EVT_RS_TIME_RECEIVER:
			if (tt_diff) {
				new_state = IS_ENABLED(CONFIG_PTP_UNCALIBRATED_PRESENT) ?
						PTP_PS_UNCALIBRATED : new_state;
			}
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}

	return new_state;
}

enum ptp_port_state ptp_tr_state_machine(enum ptp_port_state state,
					 enum ptp_port_event event,
					 bool tt_diff)
{
	enum ptp_port_state new_state = state;

	if (event == PTP_EVT_INITIALIZE || event == PTP_EVT_POWERUP) {
		/* initialize port data sets, HW and communication facilities */
		return PTP_PS_INITIALIZING;
	}

	switch (state) {
	case PTP_PS_INITIALIZING:
		switch (event) {
		case PTP_EVT_FAULT_DETECTED:
			new_state = IS_ENABLED(CONFIG_PTP_FAULTY_PRESENT) ?
					PTP_PS_FAULTY : new_state;
			break;
		case PTP_EVT_INIT_COMPLETE:
			new_state = IS_ENABLED(CONFIG_PTP_LISTENING_PRESENT) ?
					PTP_PS_LISTENING : PTP_PS_TIME_RECEIVER;
			break;
		default:
			break;
		}
		break;
#if CONFIG_PTP_FAULTY_PRESENT
	case PTP_PS_FAULTY:
		switch (event) {
		case PTP_EVT_DESIGNATED_DISABLED:
			new_state = IS_ENABLED(CONFIG_PTP_DISABLED_PRESENT) ?
					PTP_PS_DISABLED : new_state;
			break;
		case PTP_EVT_FAULT_CLEARED:
			new_state = PTP_PS_INITIALIZING;
			break;
		default:
			break;
		}
		break;
#endif
#if CONFIG_PTP_DISABLED_PRESENT
	case PTP_PS_DISABLED:
		if (event == PTP_EVT_DESIGNATED_ENABLED) {
			new_state = PTP_PS_INITIALIZING;
		}
		break;
#endif
#if CONFIG_PTP_LISTENING_PRESENT
	case PTP_PS_LISTENING:
		switch (event) {
		case PTP_EVT_DESIGNATED_DISABLED:
			new_state = IS_ENABLED(CONFIG_PTP_DISABLED_PRESENT) ?
					PTP_PS_DISABLED : new_state;
			break;
		case PTP_EVT_FAULT_DETECTED:
			new_state = IS_ENABLED(CONFIG_PTP_FAULTY_PRESENT) ?
					PTP_PS_FAULTY : new_state;
			break;
		case PTP_EVT_RS_TIME_RECEIVER:
			new_state = IS_ENABLED(CONFIG_PTP_UNCALIBRATED_PRESENT) ?
					PTP_PS_UNCALIBRATED : PTP_PS_TIME_RECEIVER;
			break;
		default:
			break;
		}
		break;
#endif
#if CONFIG_PTP_UNCALIBRATED_PRESENT
	case PTP_PS_UNCALIBRATED:
		switch (event) {
		case PTP_EVT_DESIGNATED_DISABLED:
			new_state = IS_ENABLED(CONFIG_PTP_DISABLED_PRESENT) ?
					PTP_PS_DISABLED : new_state;
			break;
		case PTP_EVT_FAULT_DETECTED:
			new_state = IS_ENABLED(CONFIG_PTP_FAULTY_PRESENT) ?
					PTP_PS_FAULTY : new_state;
			break;
		case PTP_EVT_ANNOUNCE_RECEIPT_TIMEOUT_EXPIRES:
			__fallthrough;
		case PTP_EVT_RS_TIME_TRANSMITTER:
			__fallthrough;
		case PTP_EVT_RS_GRAND_MASTER:
			__fallthrough;
		case PTP_EVT_RS_PASSIVE:
			new_state = IS_ENABLED(CONFIG_PTP_LISTENING_PRESENT) ?
					PTP_PS_LISTENING : new_state;
			break;
		case PTP_EVT_TIME_TRANSMITTER_CLOCK_SELECTED:
			new_state = PTP_PS_TIME_RECEIVER;
		default:
			break;
		}
		break;
#endif
	case PTP_PS_TIME_RECEIVER:
		switch (event) {
		case PTP_EVT_DESIGNATED_DISABLED:
			new_state = IS_ENABLED(CONFIG_PTP_DISABLED_PRESENT) ?
					PTP_PS_DISABLED : new_state;
			break;
		case PTP_EVT_FAULT_DETECTED:
			new_state = IS_ENABLED(CONFIG_PTP_FAULTY_PRESENT) ?
					PTP_PS_FAULTY : new_state;
			break;
		case PTP_EVT_SYNCHRONIZATION_FAULT:
			new_state = IS_ENABLED(CONFIG_PTP_UNCALIBRATED_PRESENT) ?
					PTP_PS_UNCALIBRATED : new_state;
			break;
		case PTP_EVT_ANNOUNCE_RECEIPT_TIMEOUT_EXPIRES:
			__fallthrough;
		case PTP_EVT_RS_TIME_TRANSMITTER:
			__fallthrough;
		case PTP_EVT_RS_GRAND_MASTER:
			__fallthrough;
		case PTP_EVT_RS_PASSIVE:
			new_state = IS_ENABLED(CONFIG_PTP_LISTENING_PRESENT) ?
					PTP_PS_LISTENING : new_state;
			break;
		case PTP_EVT_RS_TIME_RECEIVER:
			if (tt_diff) {
				new_state = IS_ENABLED(CONFIG_PTP_UNCALIBRATED_PRESENT) ?
						PTP_PS_UNCALIBRATED : new_state;
			}
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}

	return new_state;
}
