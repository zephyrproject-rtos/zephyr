/* sensor_lps25hb.h - header file for LPS22HB pressure and temperature
 * sensor driver
 */

/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_LPS22HB_LPS22HB_H_
#define ZEPHYR_DRIVERS_SENSOR_LPS22HB_LPS22HB_H_

#include <stdint.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/util.h>

#define LPS22HB_REG_WHO_AM_I                    0x0F
#define LPS22HB_VAL_WHO_AM_I                    0xB1

#define LPS22HB_REG_INTERRUPT_CFG               0x0B
#define LPS22HB_MASK_INTERRUPT_CFG_AUTORIFP     BIT(7)
#define LPS22HB_SHIFT_INTERRUPT_CFG_AUTORIFP    7
#define LPS22HB_MASK_INTERRUPT_CFG_RESET_ARP    BIT(6)
#define LPS22HB_SHIFT_INTERRUPT_CFG_RESET_ARP   6
#define LPS22HB_MASK_INTERRUPT_CFG_AUTOZERO     BIT(5)
#define LPS22HB_SHIFT_INTERRUPT_CFG_AUTOZERO    5
#define LPS22HB_MASK_INTERRUPT_CFG_RESET_AZ     BIT(4)
#define LPS22HB_SHIFT_INTERRUPT_CFG_RESET_AZ    4
#define LPS22HB_MASK_INTERRUPT_CFG_DIFF_EN      BIT(3)
#define LPS22HB_SHIFT_INTERRUPT_CFG_DIFF_EN     3
#define LPS22HB_MASK_INTERRUPT_CFG_LIR          BIT(2)
#define LPS22HB_SHIFT_INTERRUPT_CFG_LIR         2
#define LPS22HB_MASK_INTERRUPT_CFG_PL_E         BIT(1)
#define LPS22HB_SHIFT_INTERRUPT_CFG_PL_E        1
#define LPS22HB_MASK_INTERRUPT_CFG_PH_E         BIT(0)
#define LPS22HB_SHIFT_INTERRUPT_CFG_PH_E        0

#define LPS22HB_REG_THS_P_L                     0x0C
#define LPS22HB_REG_THS_P_H                     0x0D

#define LPS22HB_REG_CTRL_REG1                   0x10
#define LPS22HB_MASK_CTRL_REG1_ODR              (BIT(6) | BIT(5) | BIT(4))
#define LPS22HB_SHIFT_CTRL_REG1_ODR             4
#define LPS22HB_MASK_CTRL_REG1_EN_LPFP          BIT(3)
#define LPS22HB_SHIFT_CTRL_REG1_EN_LPFP         3
#define LPS22HB_MASK_CTRL_REG1_LPFP_CFG         BIT(2)
#define LPS22HB_SHIFT_CTRL_REG1_LPFP_CFG        2
#define LPS22HB_MASK_CTRL_REG1_BDU              BIT(1)
#define LPS22HB_SHIFT_CTRL_REG1_BDU             1
#define LPS22HB_MASK_CTRL_REG1_SIM              BIT(0)
#define LPS22HB_SHIFT_CTRL_REG1_SIM             0

#define LPS22HB_REG_CTRL_REG2                   0x11
#define LPS22HB_MASK_CTRL_REG2_BOOT             BIT(7)
#define LPS22HB_SHIFT_CTRL_REG2_BOOT            7
#define LPS22HB_MASK_CTRL_REG2_FIFO_EN          BIT(6)
#define LPS22HB_SHIFT_CTRL_REG2_FIFO_EN         6
#define LPS22HB_MASK_CTRL_REG2_STOP_ON_FTH      BIT(5)
#define LPS22HB_SHIFT_CTRL_REG2_STOP_ON_FTH     5
#define LPS22HB_MASK_CTRL_REG2_IF_ADD_INC       BIT(4)
#define LPS22HB_SHIFT_CTRL_REG2_IF_ADD_INC      4
#define LPS22HB_MASK_CTRL_REG2_I2C_DIS          BIT(3)
#define LPS22HB_SHIFT_CTRL_REG2_I2C_DIS         3
#define LPS22HB_MASK_CTRL_REG2_SWRESET          BIT(2)
#define LPS22HB_SHIFT_CTRL_REG2_SWRESET         2
#define LPS22HB_MASK_CTRL_REG2_ONE_SHOT         BIT(0)
#define LPS22HB_SHIFT_CTRL_REG2_ONE_SHOT        0

#define LPS22HB_REG_CTRL_REG3                   0x12
#define LPS22HB_MASK_CTRL_REG3_INT_H_L          BIT(7)
#define LPS22HB_SHIFT_CTRL_REG3_INT_H_L         7
#define LPS22HB_MASK_CTRL_REG3_PP_OD            BIT(6)
#define LPS22HB_SHIFT_CTRL_REG3_PP_OD           6
#define LPS22HB_MASK_CTRL_REG3_F_FSS5           BIT(5)
#define LPS22HB_SHIFT_CTRL_REG3_F_FFS5          5
#define LPS22HB_MASK_CTRL_REG3_F_FTH            BIT(4)
#define LPS22HB_SHIFT_CTRL_REG3_F_FTH           4
#define LPS22HB_MASK_CTRL_REG3_F_OVR            BIT(3)
#define LPS22HB_SHIFT_CTRL_REG3_F_OVR           3
#define LPS22HB_MASK_CTRL_REG3_DRDY             BIT(2)
#define LPS22HB_SHIFT_CTRL_REG3_DRDY            2
#define LPS22HB_MASK_CTRL_REG3_INT_S            (BIT(1) | BIT(0))
#define LPS22HB_SHIFT_CTRL_REG_INT_S            0

#define LPS22HB_REG_FIFO_CTRL                   0x14
#define LPS22HB_MASK_FIFO_CTRL_F_MODE           (BIT(7) | BIT(6) | BIT(5))
#define LPS22HB_SHIFT_FIFO_CTRL_F_MODE          5
#define LPS22HB_MASK_FIFO_CTRL_WTM              (BIT(4) | BIT(3) | BIT(2) | \
						 BIT(2) | BIT(1) | BIT(0))
#define LPS22HB_SHIFT_FIFO_CTRL_WTM             0

#define LPS22HB_REG_REF_P_XL                    0x15
#define LPS22HB_REG_REF_P_L                     0x16
#define LPS22HB_REG_REF_P_H                     0x17

#define LPS22HB_REG_RPDS_L                      0x18
#define LPS22HB_REG_RPDS_H                      0x19

#define LPS22HB_REG_RES_CONF                    0x1A
#define LPS22HB_MASK_RES_CONF_LC_EN             BIT(0)
#define LPS22HB_SHIFT_RES_CONF_LC_EN            0

#define LPS22HB_REG_INT_SOURCE                  0x25
#define LPS22HB_MASK_INT_SOURCE_IA              BIT(2)
#define LPS22HB_SHIFT_INT_SOURCE_IA             2
#define LPS22HB_MASK_INT_SOURCE_PL              BIT(1)
#define LPS22HB_SHIFT_INT_SOURCE_PL             1
#define LPS22HB_MASK_INT_SOURCE_PH              BIT(0)
#define LPS22HB_SHIFT_INT_SOURCE_PH             0

#define LPS22HB_REG_FIFO_STATUS                 0x26
#define LPS22HB_MASK_FIFO_STATUS_FTH_FIFO       BIT(7)
#define LPS22HB_SHIFT_FIFO_STATUS_FTH_FIFO      7
#define LPS22HB_MASK_FIFO_STATUS_OVR            BIT(6)
#define LPS22HB_SHIFT_FIFO_STATUS_OVR           6
#define LPS22HB_MASK_FIFO_STATUS_EMPTY_FIFO     BIT(5)
#define LPS22HB_SHIFT_FIFO_STATUS_EMPTY_FIFO    5
#define LPS22HB_MASK_FIFO_STATUS_FSS            (BIT(4) | BIT(3) | BIT(2) | \
						 BIT(1) | BIT(0))
#define LPS22HB_SHIFT_FIFO_STATUS_FSS           0

#define LPS22HB_REG_STATUS                      0x27
#define LPS22HB_MASK_STATUS_P_OR                BIT(5)
#define LPS22HB_SHIFT_STATUS_P_OR               5
#define LPS22HB_MASK_STATUS_T_OR                BIT(4)
#define LPS22HB_SHIFT_STATUS_T_OR               4
#define LPS22HB_MASK_STATUS_P_DA                BIT(1)
#define LPS22HB_SHIFT_STATUS_P_DA               1
#define LPS22HB_MASK_STATUS_T_DA                BIT(0)
#define LPS22HB_SHIFT_STATUS_T_DA               0

#define LPS22HB_REG_PRESS_OUT_XL                0x28
#define LPS22HB_REG_PRESS_OUT_L                 0x29
#define LPS22HB_REG_PRESS_OUT_H                 0x2A

#define LPS22HB_REG_TEMP_OUT_L                  0x2B
#define LPS22HB_REG_TEMP_OUT_H                  0x2C

#define LPS22HB_REG_LPFP_RES                    0x33


#if CONFIG_LPS22HB_SAMPLING_RATE == 1
	#define LPS22HB_DEFAULT_SAMPLING_RATE      1
#elif CONFIG_LPS22HB_SAMPLING_RATE == 10
	#define LPS22HB_DEFAULT_SAMPLING_RATE      2
#elif CONFIG_LPS22HB_SAMPLING_RATE == 25
	#define LPS22HB_DEFAULT_SAMPLING_RATE      3
#elif CONFIG_LPS22HB_SAMPLING_RATE == 50
	#define LPS22HB_DEFAULT_SAMPLING_RATE      4
#elif CONFIG_LPS22HB_SAMPLING_RATE == 75
	#define LPS22HB_DEFAULT_SAMPLING_RATE      5
#endif


struct lps22hb_config {
	struct i2c_dt_spec i2c;
};

struct lps22hb_data {
	int32_t sample_press;
	int16_t sample_temp;
};

#endif /* ZEPHYR_DRIVERS_SENSOR_LPS22HB_LPS22HB_H_ */
