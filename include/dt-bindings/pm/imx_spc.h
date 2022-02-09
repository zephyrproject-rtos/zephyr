/*
 * Copyright (c) 2021, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */


/*
 * Setpoint definitions for IMX Set point controller. The SPC uses a series
 * of set points to determine the clock speeds and states of cores, as well
 * as which peripherals to gate clocks to. Higher values correspond to more
 * power saving. See your SOC's datasheet for specifics of what peripherals
 * have their clocks gated at each set point.
 *
 * Set point control is implemented at the soc level (see pm_power_state_set())
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PM_IMX_SPC_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PM_IMX_SPC_H_

#define IMX_GPC_RUN		0x0
#define IMX_GPC_WAIT	0x1
#define IMX_GPC_STOP	0x2
#define IMX_GPC_SUSPEND	0x3


#define IMX_SPC_MASK 0xF0
#define IMX_SPC_SHIFT 4
#define IMX_GPC_MODE_MASK 0xF

#define IMX_SPC(x) ((x & IMX_SPC_MASK) >> IMX_SPC_SHIFT)
#define IMX_GPC_MODE(x) (x & IMX_GPC_MODE_MASK)

#define IMX_SPC_0	0x00
#define IMX_SPC_1	0x10
#define IMX_SPC_2	0x20
#define IMX_SPC_3	0x30
#define IMX_SPC_4	0x40
#define IMX_SPC_5	0x50
#define IMX_SPC_6	0x60
#define IMX_SPC_7	0x70
#define IMX_SPC_8	0x80
#define IMX_SPC_9	0x90
#define IMX_SPC_10	0xA0
#define IMX_SPC_11	0xB0
#define IMX_SPC_12	0xC0
#define IMX_SPC_13	0xD0
#define IMX_SPC_14	0xE0
#define IMX_SPC_15	0xF0


#define IMX_SPC_SET_POINT_0_RUN			(IMX_SPC_0 | IMX_GPC_RUN)
#define IMX_SPC_SET_POINT_0_WAIT		(IMX_SPC_0 | IMX_GPC_WAIT)
#define IMX_SPC_SET_POINT_0_STOP		(IMX_SPC_0 | IMX_GPC_STOP)
#define IMX_SPC_SET_POINT_0_SUSPEND		(IMX_SPC_0 | IMX_GPC_SUSPEND)
#define IMX_SPC_SET_POINT_1_RUN			(IMX_SPC_1 | IMX_GPC_RUN)
#define IMX_SPC_SET_POINT_1_WAIT		(IMX_SPC_1 | IMX_GPC_WAIT)
#define IMX_SPC_SET_POINT_1_STOP		(IMX_SPC_1 | IMX_GPC_STOP)
#define IMX_SPC_SET_POINT_1_SUSPEND		(IMX_SPC_1 | IMX_GPC_SUSPEND)
#define IMX_SPC_SET_POINT_2_RUN			(IMX_SPC_2 | IMX_GPC_RUN)
#define IMX_SPC_SET_POINT_2_WAIT		(IMX_SPC_2 | IMX_GPC_WAIT)
#define IMX_SPC_SET_POINT_2_STOP		(IMX_SPC_2 | IMX_GPC_STOP)
#define IMX_SPC_SET_POINT_2_SUSPEND		(IMX_SPC_2 | IMX_GPC_SUSPEND)
#define IMX_SPC_SET_POINT_3_RUN			(IMX_SPC_3 | IMX_GPC_RUN)
#define IMX_SPC_SET_POINT_3_WAIT		(IMX_SPC_3 | IMX_GPC_WAIT)
#define IMX_SPC_SET_POINT_3_STOP		(IMX_SPC_3 | IMX_GPC_STOP)
#define IMX_SPC_SET_POINT_3_SUSPEND		(IMX_SPC_3 | IMX_GPC_SUSPEND)
#define IMX_SPC_SET_POINT_4_RUN			(IMX_SPC_4 | IMX_GPC_RUN)
#define IMX_SPC_SET_POINT_4_WAIT		(IMX_SPC_4 | IMX_GPC_WAIT)
#define IMX_SPC_SET_POINT_4_STOP		(IMX_SPC_4 | IMX_GPC_STOP)
#define IMX_SPC_SET_POINT_4_SUSPEND		(IMX_SPC_4 | IMX_GPC_SUSPEND)
#define IMX_SPC_SET_POINT_5_RUN			(IMX_SPC_5 | IMX_GPC_RUN)
#define IMX_SPC_SET_POINT_5_WAIT		(IMX_SPC_5 | IMX_GPC_WAIT)
#define IMX_SPC_SET_POINT_5_STOP		(IMX_SPC_5 | IMX_GPC_STOP)
#define IMX_SPC_SET_POINT_5_SUSPEND		(IMX_SPC_5 | IMX_GPC_SUSPEND)
#define IMX_SPC_SET_POINT_6_RUN			(IMX_SPC_6 | IMX_GPC_RUN)
#define IMX_SPC_SET_POINT_6_WAIT		(IMX_SPC_6 | IMX_GPC_WAIT)
#define IMX_SPC_SET_POINT_6_STOP		(IMX_SPC_6 | IMX_GPC_STOP)
#define IMX_SPC_SET_POINT_6_SUSPEND		(IMX_SPC_6 | IMX_GPC_SUSPEND)
#define IMX_SPC_SET_POINT_7_RUN			(IMX_SPC_7 | IMX_GPC_RUN)
#define IMX_SPC_SET_POINT_7_WAIT		(IMX_SPC_7 | IMX_GPC_WAIT)
#define IMX_SPC_SET_POINT_7_STOP		(IMX_SPC_7 | IMX_GPC_STOP)
#define IMX_SPC_SET_POINT_7_SUSPEND		(IMX_SPC_7 | IMX_GPC_SUSPEND)
#define IMX_SPC_SET_POINT_8_RUN			(IMX_SPC_8 | IMX_GPC_RUN)
#define IMX_SPC_SET_POINT_8_WAIT		(IMX_SPC_8 | IMX_GPC_WAIT)
#define IMX_SPC_SET_POINT_8_STOP		(IMX_SPC_8 | IMX_GPC_STOP)
#define IMX_SPC_SET_POINT_8_SUSPEND		(IMX_SPC_8 | IMX_GPC_SUSPEND)
#define IMX_SPC_SET_POINT_9_RUN			(IMX_SPC_9 | IMX_GPC_RUN)
#define IMX_SPC_SET_POINT_9_WAIT		(IMX_SPC_9 | IMX_GPC_WAIT)
#define IMX_SPC_SET_POINT_9_STOP		(IMX_SPC_9 | IMX_GPC_STOP)
#define IMX_SPC_SET_POINT_9_SUSPEND		(IMX_SPC_9 | IMX_GPC_SUSPEND)
#define IMX_SPC_SET_POINT_10_RUN		(IMX_SPC_10 | IMX_GPC_RUN)
#define IMX_SPC_SET_POINT_10_WAIT		(IMX_SPC_10 | IMX_GPC_WAIT)
#define IMX_SPC_SET_POINT_10_STOP		(IMX_SPC_10 | IMX_GPC_STOP)
#define IMX_SPC_SET_POINT_10_SUSPEND	(IMX_SPC_10 | IMX_GPC_SUSPEND)
#define IMX_SPC_SET_POINT_11_RUN		(IMX_SPC_11 | IMX_GPC_RUN)
#define IMX_SPC_SET_POINT_11_WAIT		(IMX_SPC_11 | IMX_GPC_WAIT)
#define IMX_SPC_SET_POINT_11_STOP		(IMX_SPC_11 | IMX_GPC_STOP)
#define IMX_SPC_SET_POINT_11_SUSPEND	(IMX_SPC_11 | IMX_GPC_SUSPEND)
#define IMX_SPC_SET_POINT_12_RUN		(IMX_SPC_12 | IMX_GPC_RUN)
#define IMX_SPC_SET_POINT_12_WAIT		(IMX_SPC_12 | IMX_GPC_WAIT)
#define IMX_SPC_SET_POINT_12_STOP		(IMX_SPC_12 | IMX_GPC_STOP)
#define IMX_SPC_SET_POINT_12_SUSPEND	(IMX_SPC_12 | IMX_GPC_SUSPEND)
#define IMX_SPC_SET_POINT_13_RUN		(IMX_SPC_13 | IMX_GPC_RUN)
#define IMX_SPC_SET_POINT_13_WAIT		(IMX_SPC_13 | IMX_GPC_WAIT)
#define IMX_SPC_SET_POINT_13_STOP		(IMX_SPC_13 | IMX_GPC_STOP)
#define IMX_SPC_SET_POINT_13_SUSPEND	(IMX_SPC_13 | IMX_GPC_SUSPEND)
#define IMX_SPC_SET_POINT_14_RUN		(IMX_SPC_14 | IMX_GPC_RUN)
#define IMX_SPC_SET_POINT_14_WAIT		(IMX_SPC_14 | IMX_GPC_WAIT)
#define IMX_SPC_SET_POINT_14_STOP		(IMX_SPC_14 | IMX_GPC_STOP)
#define IMX_SPC_SET_POINT_14_SUSPEND	(IMX_SPC_14 | IMX_GPC_SUSPEND)
#define IMX_SPC_SET_POINT_15_RUN		(IMX_SPC_15 | IMX_GPC_RUN)
#define IMX_SPC_SET_POINT_15_WAIT		(IMX_SPC_15 | IMX_GPC_WAIT)
#define IMX_SPC_SET_POINT_15_STOP		(IMX_SPC_15 | IMX_GPC_STOP)
#define IMX_SPC_SET_POINT_15_SUSPEND	(IMX_SPC_15 | IMX_GPC_SUSPEND)


#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_PM_IMX_SPC_H_ */
