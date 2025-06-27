/*
 * Copyright (c) 2024 Realtek Semiconductor, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_PINCTRL_RTS5817_REG_H_
#define ZEPHYR_DRIVERS_PINCTRL_RTS5817_REG_H_

#define PAD_BASE_ADDR 0x0

#define PAD_CFG         (PAD_BASE_ADDR + 0X0000)
#define PAD_GPIO_O      (PAD_BASE_ADDR + 0X0004)
#define PAD_GPIO_OE     (PAD_BASE_ADDR + 0X0008)
#define PAD_GPIO_I      (PAD_BASE_ADDR + 0X000C)
#define PAD_GPIO_INC    (PAD_BASE_ADDR + 0X0010)
#define PAD_GPIO_INT_EN (PAD_BASE_ADDR + 0X0010)
#define PAD_GPIO_INT    (PAD_BASE_ADDR + 0X0014)

#define SP_BASE0            0x0
#define AL_GPIO_VALUE       (SP_BASE0 + 0X0000)
#define AL_GPIO_CFG         (SP_BASE0 + 0X0004)
#define AL_GPIO_IRQ_EN      (SP_BASE0 + 0X0008)
#define AL_GPIO_IRQ         (SP_BASE0 + 0X000C)
#define AL_SENSOR_RST_CTL   (SP_BASE0 + 0X0010)
#define AL_SENSOR_SCS_N_CTL (SP_BASE0 + 0X0014)
#define FW_WAKEUP_TRIG      (SP_BASE0 + 0X0018)
#define GPIO_SVIO_CFG       (SP_BASE0 + 0X001C)

#define SP_BASE1        0x0
#define SENSOR_INT_CFG  (SP_BASE1 + 0X0000)
#define SENSOR_INT_FLAG (SP_BASE1 + 0X0004)

/* Bits of PAD_CFG (0X0000) */

#define PD_OFFSET 0
#define PD_BITS   1
#define PD_MASK   (((1 << 1) - 1) << 0)
#define PD        (PD_MASK)

#define PU_OFFSET 1
#define PU_BITS   1
#define PU_MASK   (((1 << 1) - 1) << 1)
#define PU        (PU_MASK)

#define H3L1_OFFSET 2
#define H3L1_BITS   1
#define H3L1_MASK   (((1 << 1) - 1) << 2)
#define H3L1        (H3L1_MASK)

#define DRIVING_OFFSET 3
#define DRIVING_BITS   2
#define DRIVING_MASK   (((1 << 2) - 1) << 3)
#define DRIVING        (DRIVING_MASK)

#define SHDN_OFFSET 5
#define SHDN_BITS   1
#define SHDN_MASK   (((1 << 1) - 1) << 5)
#define SHDN        (SHDN_MASK)

#define IEV18_OFFSET 6
#define IEV18_BITS   1
#define IEV18_MASK   (((1 << 1) - 1) << 6)
#define IEV18        (IEV18_MASK)

#define PAD_FUNCTION_SEL_OFFSET 8
#define PAD_FUNCTION_SEL_BITS   2
#define PAD_FUNCTION_SEL_MASK   (((1 << 2) - 1) << 8)
#define PAD_FUNCTION_SEL        (PAD_FUNCTION_SEL_MASK)

#define GPIO_FUNCTION_SEL_OFFSET 8
#define GPIO_FUNCTION_SEL_BITS   6
#define GPIO_FUNCTION_SEL_MASK   (((1 << 6) - 1) << 8)
#define GPIO_FUNCTION_SEL        (GPIO_FUNCTION_SEL_MASK)

/* Bits of PAD_GPIO_O (0X0004) */

#define GPIO_O_OFFSET 0
#define GPIO_O_BITS   1
#define GPIO_O_MASK   (((1 << 1) - 1) << 0)
#define GPIO_O        (GPIO_O_MASK)

/* Bits of PAD_GPIO_OE (0X0008) */

#define GPIO_OE_OFFSET 0
#define GPIO_OE_BITS   1
#define GPIO_OE_MASK   (((1 << 1) - 1) << 0)
#define GPIO_OE        (GPIO_OE_MASK)

/* Bits of PAD_GPIO_I (0X000C) */

#define GPIO_I_OFFSET 0
#define GPIO_I_BITS   1
#define GPIO_I_MASK   (((1 << 1) - 1) << 0)
#define GPIO_I        (GPIO_I_MASK)

/* Bits of PAD_GPIO_INT_EN (0X0010) */

#define GPIO_FALLING_INT_EN_OFFSET 0
#define GPIO_FALLING_INT_EN_BITS   1
#define GPIO_FALLING_INT_EN_MASK   (((1 << 1) - 1) << 0)
#define GPIO_FALLING_INT_EN        (GPIO_FALLING_INT_EN_MASK)

#define GPIO_RISING_INT_EN_OFFSET 1
#define GPIO_RISING_INT_EN_BITS   1
#define GPIO_RISING_INT_EN_MASK   (((1 << 1) - 1) << 1)
#define GPIO_RISING_INT_EN        (GPIO_RISING_INT_EN_MASK)

/* Bits of PAD_GPIO_INT (0X0014) */

#define GPIO_FALLING_INT_OFFSET 0
#define GPIO_FALLING_INT_BITS   1
#define GPIO_FALLING_INT_MASK   (((1 << 1) - 1) << 0)
#define GPIO_FALLING_INT        (GPIO_FALLING_INT_MASK)

#define GPIO_RISING_INT_OFFSET 1
#define GPIO_RISING_INT_BITS   1
#define GPIO_RISING_INT_MASK   (((1 << 1) - 1) << 1)
#define GPIO_RISING_INT        (GPIO_RISING_INT_MASK)

/* Bits of R_SENSOR_INT_CFG (0X0024) */

#define IE_SENSOR_INT_OFFSET 0
#define IE_SENSOR_INT_BITS   1
#define IE_SENSOR_INT_MASK   (((1 << 1) - 1) << 0)
#define IE_SENSOR_INT        (IE_SENSOR_INT_MASK)

#define I_SENSOR_PRIORITY_OFFSET 1
#define I_SENSOR_PRIORITY_BITS   1
#define I_SENSOR_PRIORITY_MASK   (((1 << 1) - 1) << 1)
#define I_SENSOR_PRIORITY        (I_SENSOR_PRIORITY_MASK)

#define CFG_SENSOR_INT_POS_EN_OFFSET 2
#define CFG_SENSOR_INT_POS_EN_BITS   2
#define CFG_SENSOR_INT_POS_EN_MASK   (((1 << 2) - 1) << 2)
#define CFG_SENSOR_INT_POS_EN        (CFG_SENSOR_INT_POS_EN_MASK)

#define CFG_SENSOR_INT_NEG_EN_OFFSET 4
#define CFG_SENSOR_INT_NEG_EN_BITS   2
#define CFG_SENSOR_INT_NEG_EN_MASK   (((1 << 2) - 1) << 4)
#define CFG_SENSOR_INT_NEG_EN        (CFG_SENSOR_INT_NEG_EN_MASK)

/* Bits of R_SENSOR_INT_FLAG (0X0028) */

#define I_SENSOR_INTF_FALL_OFFSET 0
#define I_SENSOR_INTF_FALL_BITS   2
#define I_SENSOR_INTF_FALL_MASK   (((1 << 2) - 1) << 0)
#define I_SENSOR_INTF_FALL        (I_SENSOR_INTF_FALL_MASK)

#define I_SENSOR_INTF_RISE_OFFSET 2
#define I_SENSOR_INTF_RISE_BITS   2
#define I_SENSOR_INTF_RISE_MASK   (((1 << 2) - 1) << 2)
#define I_SENSOR_INTF_RISE        (I_SENSOR_INTF_RISE_MASK)

#define I_SENSOR_INTF_OFFSET 4
#define I_SENSOR_INTF_BITS   2
#define I_SENSOR_INTF_MASK   (((1 << 2) - 1) << 4)
#define I_SENSOR_INTF        (I_SENSOR_INTF_MASK)

#define SENSOR_SCS_N_GPIO_INT_FALL_OFFSET 6
#define SENSOR_SCS_N_GPIO_INT_FALL_BITS   1
#define SENSOR_SCS_N_GPIO_INT_FALL_MASK   (((1 << 1) - 1) << 6)
#define SENSOR_SCS_N_GPIO_INT_FALL        (SENSOR_SCS_N_GPIO_INT_FALL_MASK)

#define SENSOR_SCS_N_GPIO_INT_RISE_OFFSET 7
#define SENSOR_SCS_N_GPIO_INT_RISE_BITS   1
#define SENSOR_SCS_N_GPIO_INT_RISE_MASK   (((1 << 1) - 1) << 7)
#define SENSOR_SCS_N_GPIO_INT_RISE        (SENSOR_SCS_N_GPIO_INT_RISE_MASK)

#define GPIO_SVIO_INT_FALL_OFFSET 8
#define GPIO_SVIO_INT_FALL_BITS   1
#define GPIO_SVIO_INT_FALL_MASK   (((1 << 1) - 1) << 8)
#define GPIO_SVIO_INT_FALL        (GPIO_SVIO_INT_FALL_MASK)

#define GPIO_SVIO_INT_RISE_OFFSET 9
#define GPIO_SVIO_INT_RISE_BITS   1
#define GPIO_SVIO_INT_RISE_MASK   (((1 << 1) - 1) << 9)
#define GPIO_SVIO_INT_RISE        (GPIO_SVIO_INT_RISE_MASK)

#define SENSOR_RSTN_GPIO_INT_FALL_OFFSET 10
#define SENSOR_RSTN_GPIO_INT_FALL_BITS   1
#define SENSOR_RSTN_GPIO_INT_FALL_MASK   (((1 << 1) - 1) << 10)
#define SENSOR_RSTN_GPIO_INT_FALL        (SENSOR_RSTN_GPIO_INT_FALL_MASK)

#define SENSOR_RSTN_GPIO_INT_RISE_OFFSET 11
#define SENSOR_RSTN_GPIO_INT_RISE_BITS   1
#define SENSOR_RSTN_GPIO_INT_RISE_MASK   (((1 << 1) - 1) << 11)
#define SENSOR_RSTN_GPIO_INT_RISE        (SENSOR_RSTN_GPIO_INT_RISE_MASK)

#define WAKE_UP_I_OFFSET 16
#define WAKE_UP_I_BITS   2
#define WAKE_UP_I_MASK   (((1 << 2) - 1) << 16)
#define WAKE_UP_I        (WAKE_UP_I_MASK)

/* Bits of R_AL_GPIO_VALUE (0X0070) */

#define AL_GPIO_O_OFFSET 0
#define AL_GPIO_O_BITS   3
#define AL_GPIO_O_MASK   (((1 << 3) - 1) << 0)
#define AL_GPIO_O        (AL_GPIO_O_MASK)

#define ASYNC_AL_GPIO_IN_OFFSET 3
#define ASYNC_AL_GPIO_IN_BITS   3
#define ASYNC_AL_GPIO_IN_MASK   (((1 << 3) - 1) << 3)
#define ASYNC_AL_GPIO_IN        (ASYNC_AL_GPIO_IN_MASK)

#define AL_GPIO_I_OFFSET 6
#define AL_GPIO_I_BITS   3
#define AL_GPIO_I_MASK   (((1 << 3) - 1) << 6)
#define AL_GPIO_I        (AL_GPIO_I_MASK)

/* Bits of R_AL_GPIO_CFG (0X0074) */

#define AL_GPIO_OE_OFFSET 0
#define AL_GPIO_OE_BITS   3
#define AL_GPIO_OE_MASK   (((1 << 3) - 1) << 0)
#define AL_GPIO_OE        (AL_GPIO_OE_MASK)

#define AL_GPIO_OE2_OFFSET 3
#define AL_GPIO_OE2_BITS   3
#define AL_GPIO_OE2_MASK   (((1 << 3) - 1) << 3)
#define AL_GPIO_OE2        (AL_GPIO_OE2_MASK)

#define AL_GPIO_OE3_OFFSET 8
#define AL_GPIO_OE3_BITS   3
#define AL_GPIO_OE3_MASK   (((1 << 3) - 1) << 8)
#define AL_GPIO_OE3        (AL_GPIO_OE3_MASK)

#define AL_GPIO_SHDN_OFFSET 11
#define AL_GPIO_SHDN_BITS   3
#define AL_GPIO_SHDN_MASK   (((1 << 3) - 1) << 11)
#define AL_GPIO_SHDN        (AL_GPIO_SHDN_MASK)

#define AL_GPIO_IEV18_OFFSET 16
#define AL_GPIO_IEV18_BITS   3
#define AL_GPIO_IEV18_MASK   (((1 << 3) - 1) << 16)
#define AL_GPIO_IEV18        (AL_GPIO_IEV18_MASK)

#define AL_GPIO_PU_CTRL_OFFSET 19
#define AL_GPIO_PU_CTRL_BITS   3
#define AL_GPIO_PU_CTRL_MASK   (((1 << 3) - 1) << 19)
#define AL_GPIO_PU_CTRL        (AL_GPIO_PU_CTRL_MASK)

#define AL_GPIO_PD_CTRL_OFFSET 24
#define AL_GPIO_PD_CTRL_BITS   3
#define AL_GPIO_PD_CTRL_MASK   (((1 << 3) - 1) << 24)
#define AL_GPIO_PD_CTRL        (AL_GPIO_PD_CTRL_MASK)

#define AL_GPIO_SEL_OFFSET 27
#define AL_GPIO_SEL_BITS   3
#define AL_GPIO_SEL_MASK   (((1 << 3) - 1) << 27)
#define AL_GPIO_SEL        (AL_GPIO_SEL_MASK)

#define CS1_BRIDGE_EN_OFFSET 31
#define CS1_BRIDGE_EN_BITS   1
#define CS1_BRIDGE_EN_MASK   (((1 << 1) - 1) << 31)
#define CS1_BRIDGE_EN        (CS1_BRIDGE_EN_MASK)

/* Bits of R_AL_GPIO_IRQ_EN (0X0078) */

#define AL_GPIO_INT_EN_FALL_OFFSET 0
#define AL_GPIO_INT_EN_FALL_BITS   3
#define AL_GPIO_INT_EN_FALL_MASK   (((1 << 3) - 1) << 0)
#define AL_GPIO_INT_EN_FALL        (AL_GPIO_INT_EN_FALL_MASK)

#define AL_GPIO_INT_EN_RISE_OFFSET 3
#define AL_GPIO_INT_EN_RISE_BITS   3
#define AL_GPIO_INT_EN_RISE_MASK   (((1 << 3) - 1) << 3)
#define AL_GPIO_INT_EN_RISE        (AL_GPIO_INT_EN_RISE_MASK)

/* Bits of R_AL_GPIO_IRQ (0X007C) */

#define AL_GPIO_INT_FALL_OFFSET 0
#define AL_GPIO_INT_FALL_BITS   3
#define AL_GPIO_INT_FALL_MASK   (((1 << 3) - 1) << 0)
#define AL_GPIO_INT_FALL        (AL_GPIO_INT_FALL_MASK)

#define AL_GPIO_INT_RISE_OFFSET 3
#define AL_GPIO_INT_RISE_BITS   3
#define AL_GPIO_INT_RISE_MASK   (((1 << 3) - 1) << 3)
#define AL_GPIO_INT_RISE        (AL_GPIO_INT_RISE_MASK)

/* Bits of R_AL_SENSOR_RST_CTL (0X0080) */

#define PAD_SENSOR_RSTN_I_OFFSET 0
#define PAD_SENSOR_RSTN_I_BITS   1
#define PAD_SENSOR_RSTN_I_MASK   (((1 << 1) - 1) << 0)
#define PAD_SENSOR_RSTN_I        (PAD_SENSOR_RSTN_I_MASK)

#define SENSOR_RSTN_O_OFFSET 1
#define SENSOR_RSTN_O_BITS   1
#define SENSOR_RSTN_O_MASK   (((1 << 1) - 1) << 1)
#define SENSOR_RSTN_O        (SENSOR_RSTN_O_MASK)

#define SENSOR_RSTN_OE0_OFFSET 2
#define SENSOR_RSTN_OE0_BITS   1
#define SENSOR_RSTN_OE0_MASK   (((1 << 1) - 1) << 2)
#define SENSOR_RSTN_OE0        (SENSOR_RSTN_OE0_MASK)

#define SENSOR_RSTN_OE2_OFFSET 3
#define SENSOR_RSTN_OE2_BITS   1
#define SENSOR_RSTN_OE2_MASK   (((1 << 1) - 1) << 3)
#define SENSOR_RSTN_OE2        (SENSOR_RSTN_OE2_MASK)

#define SENSOR_RSTN_OE3_OFFSET 4
#define SENSOR_RSTN_OE3_BITS   1
#define SENSOR_RSTN_OE3_MASK   (((1 << 1) - 1) << 4)
#define SENSOR_RSTN_OE3        (SENSOR_RSTN_OE3_MASK)

#define SENSOR_RSTN_IEV18_OFFSET 5
#define SENSOR_RSTN_IEV18_BITS   1
#define SENSOR_RSTN_IEV18_MASK   (((1 << 1) - 1) << 5)
#define SENSOR_RSTN_IEV18        (SENSOR_RSTN_IEV18_MASK)

#define SENSOR_RSTN_H3L1_OFFSET 6
#define SENSOR_RSTN_H3L1_BITS   1
#define SENSOR_RSTN_H3L1_MASK   (((1 << 1) - 1) << 6)
#define SENSOR_RSTN_H3L1        (SENSOR_RSTN_H3L1_MASK)

#define SENSOR_RSTN_PUE_OFFSET 7
#define SENSOR_RSTN_PUE_BITS   1
#define SENSOR_RSTN_PUE_MASK   (((1 << 1) - 1) << 7)
#define SENSOR_RSTN_PUE        (SENSOR_RSTN_PUE_MASK)

#define SENSOR_RSTN_PDE_OFFSET 8
#define SENSOR_RSTN_PDE_BITS   1
#define SENSOR_RSTN_PDE_MASK   (((1 << 1) - 1) << 8)
#define SENSOR_RSTN_PDE        (SENSOR_RSTN_PDE_MASK)

#define SENSOR_RSTN_GPIO_INT_EN_RISE_OFFSET 10
#define SENSOR_RSTN_GPIO_INT_EN_RISE_BITS   1
#define SENSOR_RSTN_GPIO_INT_EN_RISE_MASK   (((1 << 1) - 1) << 10)
#define SENSOR_RSTN_GPIO_INT_EN_RISE        (SENSOR_RSTN_GPIO_INT_EN_RISE_MASK)

#define SENSOR_RSTN_GPIO_INT_EN_FALL_OFFSET 11
#define SENSOR_RSTN_GPIO_INT_EN_FALL_BITS   1
#define SENSOR_RSTN_GPIO_INT_EN_FALL_MASK   (((1 << 1) - 1) << 11)
#define SENSOR_RSTN_GPIO_INT_EN_FALL        (SENSOR_RSTN_GPIO_INT_EN_FALL_MASK)

/* Bits of R_AL_SENSOR_SCS_N_CTL (0X0084) */

#define PAD_SENSOR_SCS_N_I_OFFSET 0
#define PAD_SENSOR_SCS_N_I_BITS   1
#define PAD_SENSOR_SCS_N_I_MASK   (((1 << 1) - 1) << 0)
#define PAD_SENSOR_SCS_N_I        (PAD_SENSOR_SCS_N_I_MASK)

#define SENSOR_SCS_N_O_OFFSET 1
#define SENSOR_SCS_N_O_BITS   1
#define SENSOR_SCS_N_O_MASK   (((1 << 1) - 1) << 1)
#define SENSOR_SCS_N_O        (SENSOR_SCS_N_O_MASK)

#define SENSOR_SCS_N_OE0_OFFSET 2
#define SENSOR_SCS_N_OE0_BITS   1
#define SENSOR_SCS_N_OE0_MASK   (((1 << 1) - 1) << 2)
#define SENSOR_SCS_N_OE0        (SENSOR_SCS_N_OE0_MASK)

#define SENSOR_SCS_N_OE2_OFFSET 3
#define SENSOR_SCS_N_OE2_BITS   1
#define SENSOR_SCS_N_OE2_MASK   (((1 << 1) - 1) << 3)
#define SENSOR_SCS_N_OE2        (SENSOR_SCS_N_OE2_MASK)

#define SENSOR_SCS_N_OE3_OFFSET 4
#define SENSOR_SCS_N_OE3_BITS   1
#define SENSOR_SCS_N_OE3_MASK   (((1 << 1) - 1) << 4)
#define SENSOR_SCS_N_OE3        (SENSOR_SCS_N_OE3_MASK)

#define SENSOR_SCS_N_IEV18_OFFSET 5
#define SENSOR_SCS_N_IEV18_BITS   1
#define SENSOR_SCS_N_IEV18_MASK   (((1 << 1) - 1) << 5)
#define SENSOR_SCS_N_IEV18        (SENSOR_SCS_N_IEV18_MASK)

#define SENSOR_SCS_N_H3L1_OFFSET 6
#define SENSOR_SCS_N_H3L1_BITS   1
#define SENSOR_SCS_N_H3L1_MASK   (((1 << 1) - 1) << 6)
#define SENSOR_SCS_N_H3L1        (SENSOR_SCS_N_H3L1_MASK)

#define SENSOR_SCS_N_PUE_OFFSET 7
#define SENSOR_SCS_N_PUE_BITS   1
#define SENSOR_SCS_N_PUE_MASK   (((1 << 1) - 1) << 7)
#define SENSOR_SCS_N_PUE        (SENSOR_SCS_N_PUE_MASK)

#define SENSOR_SCS_N_PDE_OFFSET 8
#define SENSOR_SCS_N_PDE_BITS   1
#define SENSOR_SCS_N_PDE_MASK   (((1 << 1) - 1) << 8)
#define SENSOR_SCS_N_PDE        (SENSOR_SCS_N_PDE_MASK)

#define SENSOR_SCS_N_SEL_OFFSET 9
#define SENSOR_SCS_N_SEL_BITS   1
#define SENSOR_SCS_N_SEL_MASK   (((1 << 1) - 1) << 9)
#define SENSOR_SCS_N_SEL        (SENSOR_SCS_N_SEL_MASK)

#define SENSOR_SCS_N_GPIO_INT_EN_RISE_OFFSET 10
#define SENSOR_SCS_N_GPIO_INT_EN_RISE_BITS   1
#define SENSOR_SCS_N_GPIO_INT_EN_RISE_MASK   (((1 << 1) - 1) << 10)
#define SENSOR_SCS_N_GPIO_INT_EN_RISE        (SENSOR_SCS_N_GPIO_INT_EN_RISE_MASK)

#define SENSOR_SCS_N_GPIO_INT_EN_FALL_OFFSET 11
#define SENSOR_SCS_N_GPIO_INT_EN_FALL_BITS   1
#define SENSOR_SCS_N_GPIO_INT_EN_FALL_MASK   (((1 << 1) - 1) << 11)
#define SENSOR_SCS_N_GPIO_INT_EN_FALL        (SENSOR_SCS_N_GPIO_INT_EN_FALL_MASK)

/* Bits of R_GPIO_SVIO_CFG (0X008C) */

#define GPIO_SVIO_I_OFFSET 0
#define GPIO_SVIO_I_BITS   1
#define GPIO_SVIO_I_MASK   (((1 << 1) - 1) << 0)
#define GPIO_SVIO_I        (GPIO_SVIO_I_MASK)

#define GPIO_SVIO_O_OFFSET 1
#define GPIO_SVIO_O_BITS   1
#define GPIO_SVIO_O_MASK   (((1 << 1) - 1) << 1)
#define GPIO_SVIO_O        (GPIO_SVIO_O_MASK)

#define GPIO_SVIO_OE0_OFFSET 2
#define GPIO_SVIO_OE0_BITS   1
#define GPIO_SVIO_OE0_MASK   (((1 << 1) - 1) << 2)
#define GPIO_SVIO_OE0        (GPIO_SVIO_OE0_MASK)

#define GPIO_SVIO_OE2_OFFSET 3
#define GPIO_SVIO_OE2_BITS   1
#define GPIO_SVIO_OE2_MASK   (((1 << 1) - 1) << 3)
#define GPIO_SVIO_OE2        (GPIO_SVIO_OE2_MASK)

#define GPIO_SVIO_OE3_OFFSET 4
#define GPIO_SVIO_OE3_BITS   1
#define GPIO_SVIO_OE3_MASK   (((1 << 1) - 1) << 4)
#define GPIO_SVIO_OE3        (GPIO_SVIO_OE3_MASK)

#define GPIO_SVIO_H3L1_OFFSET 5
#define GPIO_SVIO_H3L1_BITS   1
#define GPIO_SVIO_H3L1_MASK   (((1 << 1) - 1) << 5)
#define GPIO_SVIO_H3L1        (GPIO_SVIO_H3L1_MASK)

#define GPIO_SVIO_IEV18_OFFSET 6
#define GPIO_SVIO_IEV18_BITS   1
#define GPIO_SVIO_IEV18_MASK   (((1 << 1) - 1) << 6)
#define GPIO_SVIO_IEV18        (GPIO_SVIO_IEV18_MASK)

#define GPIO_SVIO_PULLCTL_OFFSET 8
#define GPIO_SVIO_PULLCTL_BITS   2
#define GPIO_SVIO_PULLCTL_MASK   (((1 << 2) - 1) << 8)
#define GPIO_SVIO_PULLCTL        (GPIO_SVIO_PULLCTL_MASK)

#define GPIO_SVIO_INT_EN_RISE_OFFSET 10
#define GPIO_SVIO_INT_EN_RISE_BITS   1
#define GPIO_SVIO_INT_EN_RISE_MASK   (((1 << 1) - 1) << 10)
#define GPIO_SVIO_INT_EN_RISE        (GPIO_SVIO_INT_EN_RISE_MASK)

#define GPIO_SVIO_INT_EN_FALL_OFFSET 11
#define GPIO_SVIO_INT_EN_FALL_BITS   1
#define GPIO_SVIO_INT_EN_FALL_MASK   (((1 << 1) - 1) << 11)
#define GPIO_SVIO_INT_EN_FALL        (GPIO_SVIO_INT_EN_FALL_MASK)

#endif /* ZEPHYR_DRIVERS_PINCTRL_RTS5817_REG_H_ */
