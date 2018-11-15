/*
 * Copyright (c) 2018 Thomas Li Fredriksen
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_DRIVERS_SENSOR_SI1133_SI1133_REGS_H_
#define ZEPHYR_DRIVERS_SENSOR_SI1133_SI1133_REGS_H_

/*******************************************************************************
 ************************** Si1133 I2C Registers *******************************
 ******************************************************************************/
//
// I2C Registers
//
#define SI1133_REG_PART_ID      0x00
#define SI1133_REG_HW_ID        0x01
#define SI1133_REG_REV_ID       0x02
#define SI1133_REG_HOSTIN0      0x0A
#define SI1133_REG_COMMAND      0x0B
#define SI1133_REG_IRQ_ENABLE   0x0F
#define SI1133_REG_RESPONSE1    0x10
#define SI1133_REG_RESPONSE0    0x11
#define SI1133_REG_IRQ_STATUS   0x12
#define SI1133_REG_HOSTOUT0     0x13
#define SI1133_REG_HOSTOUT1     0x14
#define SI1133_REG_HOSTOUT2     0x15
#define SI1133_REG_HOSTOUT3     0x16
#define SI1133_REG_HOSTOUT4     0x17
#define SI1133_REG_HOSTOUT5     0x18
#define SI1133_REG_HOSTOUT6     0x19
#define SI1133_REG_HOSTOUT7     0x1A
#define SI1133_REG_HOSTOUT8     0x1B
#define SI1133_REG_HOSTOUT9     0x1C
#define SI1133_REG_HOSTOUT10    0x1D
#define SI1133_REG_HOSTOUT11    0x1E
#define SI1133_REG_HOSTOUT12    0x1F
#define SI1133_REG_HOSTOUT13    0x20
#define SI1133_REG_HOSTOUT14    0x21
#define SI1133_REG_HOSTOUT15    0x22
#define SI1133_REG_HOSTOUT16    0x23
#define SI1133_REG_HOSTOUT17    0x24
#define SI1133_REG_HOSTOUT18    0x25
#define SI1133_REG_HOSTOUT19    0x26
#define SI1133_REG_HOSTOUT20    0x27
#define SI1133_REG_HOSTOUT21    0x28
#define SI1133_REG_HOSTOUT22    0x29
#define SI1133_REG_HOSTOUT23    0x2A
#define SI1133_REG_HOSTOUT24    0x2B
#define SI1133_REG_HOSTOUT25    0x2C


/*******************************************************************************
 ************************** Si1133 I2C Parameter Offsets ***********************
 ******************************************************************************/
#define SI1133_PARAM_I2C_ADDR          0x00
#define SI1133_PARAM_CH_LIST           0x01
#define SI1133_PARAM_ADCCONFIG0        0x02
#define SI1133_PARAM_ADCSENS0          0x03
#define SI1133_PARAM_ADCPOST0          0x04
#define SI1133_PARAM_MEASCONFIG0       0x05
#define SI1133_PARAM_ADCCONFIG1        0x06
#define SI1133_PARAM_ADCSENS1          0x07
#define SI1133_PARAM_ADCPOST1          0x08
#define SI1133_PARAM_MEASCONFIG1       0x09
#define SI1133_PARAM_ADCCONFIG2        0x0A
#define SI1133_PARAM_ADCSENS2          0x0B
#define SI1133_PARAM_ADCPOST2          0x0C
#define SI1133_PARAM_MEASCONFIG2       0x0D
#define SI1133_PARAM_ADCCONFIG3        0x0E
#define SI1133_PARAM_ADCSENS3          0x0F
#define SI1133_PARAM_ADCPOST3          0x10
#define SI1133_PARAM_MEASCONFIG3       0x11
#define SI1133_PARAM_ADCCONFIG4        0x12
#define SI1133_PARAM_ADCSENS4          0x13
#define SI1133_PARAM_ADCPOST4          0x14
#define SI1133_PARAM_MEASCONFIG4       0x15
#define SI1133_PARAM_ADCCONFIG5        0x16
#define SI1133_PARAM_ADCSENS5          0x17
#define SI1133_PARAM_ADCPOST5          0x18
#define SI1133_PARAM_MEASCONFIG5       0x19
#define SI1133_PARAM_MEASRATE_H        0x1A
#define SI1133_PARAM_MEASRATE_L        0x1B
#define SI1133_PARAM_MEASCOUNT0        0x1C
#define SI1133_PARAM_MEASCOUNT1        0x1D
#define SI1133_PARAM_MEASCOUNT2        0x1E
#define SI1133_PARAM_LED1_A            0x1F
#define SI1133_PARAM_LED1_B            0x20
#define SI1133_PARAM_LED2_A            0x21
#define SI1133_PARAM_LED2_B            0x22
#define SI1133_PARAM_LED3_A            0x23
#define SI1133_PARAM_LED3_B            0x24
#define SI1133_PARAM_THRESHOLD0_H      0x25
#define SI1133_PARAM_THRESHOLD0_L      0x26
#define SI1133_PARAM_THRESHOLD1_H      0x27
#define SI1133_PARAM_THRESHOLD1_L      0x28
#define SI1133_PARAM_THRESHOLD2_H      0x29
#define SI1133_PARAM_THRESHOLD2_L      0x2A
#define SI1133_PARAM_BURST             0x2B

#define SI1133_CMD_NOP                 0x00
#define SI1133_CMD_RESET               0x01
#define SI1133_CMD_NEW_ADDR            0x02
#define SI1133_CMD_FORCE_CH            0x11
#define SI1133_CMD_PAUSE_CH            0x12
#define SI1133_CMD_AUTO_CH             0x13
#define SI1133_CMD_PARAM_SET           0x80
#define SI1133_CMD_PARAM_QUERY         0x40

/*******************************************************************************
 *******    Si1133 Register and Parameter Bit Definitions  *********************
 ******************************************************************************/
#define SI1133_RSP0_CHIPSTAT_MASK      0xe0
#define SI1133_RSP0_COUNTER_MASK       0x1f
#define SI1133_RSP0_SLEEP              0x20

/*******************************************************************************
 *******    Si1133 ADC Config  *************************************************
 ******************************************************************************/

#define SI1133_ADCCONFIGX_DECIM_RATE_MASK   0x60
#define SI1133_ADCCONFIGX_DECIM_RATE_SHIFT  (5)
#define SI1133_ADCCONFIGX_DECIM_RATE(x) \
    (((x) << SI1133_ADCCONFIGX_DECIM_RATE_SHIFT) & SI1133_ADCCONFIGX_DECIM_RATE_MASK)

#define SI1133_ADCCONFIGX_ADCMUX_MASK       0x1F
#define SI1133_ADCCONFIGX_ADCMUX_SHIFT      (0)
#define SI1133_ADCCONFIGX_ADCMUX(x) \
    ((x) & SI1133_ADCCONFIGX_ADCMUX_MASK)

enum SI1133_ADCCONFIG_ADCMUX_DEFS {

    SI1133_ADCCONFIG_ADCMUX_SMALL_IR = SI1133_ADCCONFIGX_ADCMUX(0),
    SI1133_ADCCONFIG_ADCMUX_MEDIUM_IR = SI1133_ADCCONFIGX_ADCMUX(1),
    SI1133_ADCCONFIG_ADCMUX_LARGE_IR = SI1133_ADCCONFIGX_ADCMUX(2),
    SI1133_ADCCONFIG_ADCMUX_WHITE = SI1133_ADCCONFIGX_ADCMUX(11),
    SI1133_ADCCONFIG_ADCMUX_LARGE_WHITE = SI1133_ADCCONFIGX_ADCMUX(13),
    SI1133_ADCCONFIG_ADCMUX_UV = SI1133_ADCCONFIGX_ADCMUX(24),
    SI1133_ADCCONFIG_ADCMUX_UV_DEEP = SI1133_ADCCONFIGX_ADCMUX(25)
};

#define SI1133_ADCSENSX_HSIG_MASK               0x80
#define SI1133_ADCSENSX_HSIG_SHIFT              (7)
#define SI1133_ADCSENSX_HSIG(x) \
        (((x) << SI1133_ADCSENSX_HSIG_SHIFT) & SI1133_ADCSENSX_HSIG_MASK)

#define SI1133_ADCSENSX_SW_GAIN_MASK            0x70
#define SI1133_ADCSENSX_SW_GAIN_SHIFT           (4)
#define SI1133_ADCSENSX_SW_GAIN(x) \
        (((x) << SI1133_ADCSENSX_SW_GAIN_SHIFT) & SI1133_ADCSENSX_SW_GAIN_MASK)

#define SI1133_ADCSENSX_HW_GAIN_MASK            0x0F
#define SI1133_ADCSENSX_HW_GAIN_SHIFT           (0)
#define SI1133_ADCSENSX_HW_GAIN(x) \
        ((x) & SI1133_ADCSENSX_HW_GAIN_MASK)


#define SI1133_ADCPOSTX_24BIT_OUT_MASK          0x40
#define SI1133_ADCPOSTX_24BIT_OUT_SHIFT         (6)
#define SI1133_ADCPOSTX_24BIT_OUT(x) \
        (((x) << SI1133_ADCPOSTX_24BIT_OUT_SHIFT) & SI1133_ADCPOSTX_24BIT_OUT_MASK)

#define SI1133_ADCPOSTX_POSTSHIFT_MASK          0x38
#define SI1133_ADCPOSTX_POSTSHIFT_SHIFT         (3)
#define SI1133_ADCPOSTX_POSTSHIFT(x) \
        (((x) << SI1133_ADCPOSTX_POSTSHIFT_SHIFT) & SI1133_ADCPOSTX_POSTSHIFT_MASK)

#define SI1133_ADCPOSTX_THRESH_EN_MASK          0x03
#define SI1133_ADCPOSTX_THRESH_EN_SHIFT         (0)
#define SI1133_ADCPOSTX_THRESH_EN(x) \
        ((x) & SI1133_ADCPOSTX_THRESH_EN_MASK)


#define SI1133_MEASCONFIGX_COUNTER_INDEX_MASK   0xC0
#define SI1133_MEASCONFIGX_COUNTER_INDEX_SHIFT  (6)
#define SI1133_MEASCONFIGX_COUNTER_INDEX(x) \
        (((x) << SI1133_MEASCONFIGX_COUNTER_INDEX_SHIFT) & SI1133_MEASCONFIGX_COUNTER_INDEX_MASK)


#define SI1133_I2C_ADDR_INCR_DIS_MASK  0x40
#define SI1133_I2C_ADDR_INCR_DIS_SHIFT (6)
#define SI1133_I2C_ADDR_INCR_DIS(x) \
        (((x) << SI1133_I2C_ADDR_INCR_DIS_SHIFT) & SI1133_I2C_ADDR_INCR_DIS_MASK)

#endif /* ZEPHYR_DRIVERS_SENSOR_SI1133_SI1133_REGS_H_ */
