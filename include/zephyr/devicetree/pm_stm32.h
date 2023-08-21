/*
 * Copyright (c) 2023 Kenneth J. Miller
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DEVICETREE_PM_STM32_H_
#define ZEPHYR_INCLUDE_DEVICETREE_PM_STM32_H_

#include <zephyr/pm/state.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Helper macro to initialize an entry of a struct pm_state_info array
 * when using UTIL_LISTIFY in PM_STATE_INFO_LIST_FROM_DT_REINIT.
 *
 * @note Only enabled states are initialized.
 *
 * @param i UTIL_LISTIFY entry index.
 * @param node_id A node identifier with compatible zephyr,power-state
 */
#define Z_PM_STATE_INFO_FROM_DT_REINIT(i, node_id)						   \
	COND_CODE_1(										   \
		DT_NODE_HAS_STATUS(DT_PHANDLE_BY_IDX(node_id, st_reinit_power_states, i), okay),   \
		(PM_STATE_INFO_DT_INIT(DT_PHANDLE_BY_IDX(node_id, st_reinit_power_states, i)),),   \
		())

/**
 * @brief Initialize an array of struct pm_state_info with information from all
 * the states present and enabled in the given device node identifier.
 *
 * Example devicetree fragment:
 *
 * @code{.dts}
 *
 *	cpus {
 *		cpu0: cpu@0 {
 *			device_type = "cpu";
 *			...
 *			cpu-power-states = <&state0 &state1>;
 *		};
 *
 *		power-states {
			stop1: state1 {
				compatible = "zephyr,power-state";
				power-state-name = "suspend-to-idle";
				substate-id = <2>;
				...
			};
			stop2: state2 {
				compatible = "zephyr,power-state";
				power-state-name = "suspend-to-idle";
				substate-id = <3>;
				...
			};
 *		};
 *	};
 *
 *	soc {
 *		i2c1: i2c@50000000 {
 *			...
 *			compatible = "st,stm32-i2c-v2";
 *			st,reinit-power-states = <&stop2>;
 *		};
 *	};
 *
 * @endcode
 *
 * Example usage:
 *
 * @code{.c}
 * const struct pm_state_info states[] =
 *	PM_STATE_INFO_LIST_FROM_DT_REINIT(DT_NODELABEL(i2c1));
 * @endcode
 *
 * @param node_id A device node identifier.
 */
#define PM_STATE_INFO_LIST_FROM_DT_REINIT(node_id)						   \
	{											   \
		LISTIFY(DT_PROP_LEN_OR(node_id, st_reinit_power_states, 0),			   \
			Z_PM_STATE_INFO_FROM_DT_REINIT, (), node_id)				   \
	}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DEVICETREE_PM_STM32_H_ */
