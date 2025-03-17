/*
 * Copyright (c) 2024 Hounjoung Rim <hounjoung@tsnlab.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_GPIO_GPIO_TCCVCP_H_
#define ZEPHYR_INCLUDE_DRIVERS_GPIO_GPIO_TCCVCP_H_

/*
 * GLOBAL DEFINITIONS
 */

/*
 * gpio cfg structures
 *   [31:14]: reserved
 *   [11:10]: input buffer
 *   [9]    : direction
 *   [8:6]  : driver strength (0~3)
 *   [5:4]  : pull up/down
 *   [3:0]  : function selection (0~15)
 */

#define GPIO_INPUTBUF_SHIFT 10
#define GPIO_INPUTBUF_MASK  0x3UL
#define GPIO_INPUTBUF_EN    ((2UL | 1UL) << (uint32_t)GPIO_INPUTBUF_SHIFT)
#define GPIO_INPUTBUF_DIS   ((2UL | 0UL) << (uint32_t)GPIO_INPUTBUF_SHIFT)

#define GPIO_OUTPUT_SHIFT 9
#define VCP_GPIO_OUTPUT   (1UL << (uint32_t)GPIO_OUTPUT_SHIFT)
#define VCP_GPIO_INPUT    (0UL << (uint32_t)GPIO_OUTPUT_SHIFT)

#define GPIO_DS_SHIFT 6
#define GPIO_DS_MASK  0x7UL
#define GPIO_DS(x)    ((((x) & (uint32_t)GPIO_DS_MASK) | 0x4UL) << (uint32_t)GPIO_DS_SHIFT)

#define GPIO_PULL_SHIFT 4
#define GPIO_PULL_MASK  0x3UL
#define GPIO_NOPULL     (0UL << (uint32_t)GPIO_PULL_SHIFT)
#define GPIO_PULLUP     (1UL << (uint32_t)GPIO_PULL_SHIFT)
#define GPIO_PULLDN     (2UL << (uint32_t)GPIO_PULL_SHIFT)

#define GPIO_DATA         0x00
#define GPIO_OUT_EN       0x04
#define GPIO_OUT_DATA_OR  0x08
#define GPIO_OUT_DATA_BIC 0x0C
#define GPIO_OUT_DATA_XOR 0x10
#define GPIO_IN_EN        0x24
#define GPIO_IODIR        0x08

#define GPIO_FUNC_MASK 0xFUL
#define GPIO_FUNC(x)   ((x) & (uint32_t)GPIO_FUNC_MASK)

#define GPIO_MFIO_CFG_CH_SEL0     0
#define GPIO_MFIO_CFG_PERI_SEL0   4
#define GPIO_MFIO_CFG_CH_SEL1     8
#define GPIO_MFIO_CFG_PERI_SEL1   12
#define GPIO_MFIO_CFG_CH_SEL2     16
#define GPIO_MFIO_CFG_PERI_SEL2   20
#define GPIO_MFIO_DISABLE         0
#define GPIO_MFIO_SPI2            1
#define GPIO_MFIO_UART3           2
#define GPIO_MFIO_I2C3            3
#define GPIO_MFIO_SPI3            1
#define GPIO_MFIO_UART4           2
#define GPIO_MFIO_I2C4            3
#define GPIO_MFIO_SPI4            1
#define GPIO_MFIO_UART5           2
#define GPIO_MFIO_I2C5            3
#define GPIO_MFIO_CH0             0
#define GPIO_MFIO_CH1             1
#define GPIO_MFIO_CH2             2
#define GPIO_MFIO_CH3             3
#define GPIO_PERICH_SEL_UARTSEL_0 0
#define GPIO_PERICH_SEL_UARTSEL_1 1
#define GPIO_PERICH_SEL_UARTSEL_2 2
#define GPIO_PERICH_SEL_I2CSEL_0  3
#define GPIO_PERICH_SEL_I2CSEL_1  4
#define GPIO_PERICH_SEL_I2CSEL_2  5
#define GPIO_PERICH_SEL_SPISEL_0  6
#define GPIO_PERICH_SEL_SPISEL_1  7
#define GPIO_PERICH_SEL_I2SSEL_0  8
#define GPIO_PERICH_SEL_PWMSEL_0  10
#define GPIO_PERICH_SEL_PWMSEL_1  12
#define GPIO_PERICH_SEL_PWMSEL_2  14
#define GPIO_PERICH_SEL_PWMSEL_3  16
#define GPIO_PERICH_SEL_PWMSEL_4  18
#define GPIO_PERICH_SEL_PWMSEL_5  20
#define GPIO_PERICH_SEL_PWMSEL_6  22
#define GPIO_PERICH_SEL_PWMSEL_7  24
#define GPIO_PERICH_SEL_PWMSEL_8  26
#define GPIO_PERICH_CH0           0
#define GPIO_PERICH_CH1           1
#define GPIO_PERICH_CH2           2
#define GPIO_PERICH_CH3           3

/*
 * gpio port & pin structures
 *   [31:10]: reserved
 *   [9:5] : port (A,B,C,...)
 *   [4:0] : pin number (0~31)
 */

#define GPIO_PIN_MASK     0x1FUL
#define GPIO_PIN_NUM_MASK 0x3FUL

#define GPIO_PORT_SHIFT 5
#define GPIO_PORT_MASK  ((uint32_t)0x1F << (uint32_t)GPIO_PORT_SHIFT)

#define GPIO_PORT_A ((uint32_t)0 << (uint32_t)GPIO_PORT_SHIFT)
#define GPIO_PORT_B ((uint32_t)1 << (uint32_t)GPIO_PORT_SHIFT)
#define GPIO_PORT_C ((uint32_t)2 << (uint32_t)GPIO_PORT_SHIFT)
#define GPIO_PORT_K ((uint32_t)3 << (uint32_t)GPIO_PORT_SHIFT)

#define GPIO_GPA(x) (GPIO_PORT_A | ((x) & (uint32_t)0x1F))
#define GPIO_GPB(x) (GPIO_PORT_B | ((x) & (uint32_t)0x1F))
#define GPIO_GPC(x) (GPIO_PORT_C | ((x) & (uint32_t)0x1F))
#define GPIO_GPK(x) (GPIO_PORT_K | ((x) & (uint32_t)0x1F))

#define GPIO_GP_MAX GPIO_GPK((uint32_t)0x1f)

#define GPIO_PMGPIO_BASE (MCU_BSP_PMIO_BASE)

#define GPIO_REG_BASE(x)                                                                           \
	(MCU_BSP_GPIO_BASE + ((((x) & GPIO_PORT_MASK) >> (uint32_t)GPIO_PORT_SHIFT) * 0x40UL))

#define GPIO_IS_GPIOK(x) (bool)((((x) & GPIO_PORT_MASK) == GPIO_PORT_K) ? 1 : 0)

#define GPIO_REG_DATA(x)     (GPIO_REG_BASE(x) + 0x00UL)
#define GPIO_REG_OUTEN(x)    (GPIO_REG_BASE(x) + 0x04UL)
#define GPIO_REG_DATA_OR(x)  (GPIO_REG_BASE(x) + 0x08UL)
#define GPIO_REG_DATA_BIC(x) (GPIO_REG_BASE(x) + 0x0CUL)
#define GPIO_REG_PULLEN(x)                                                                         \
	(GPIO_IS_GPIOK(x) ? (GPIO_PMGPIO_BASE + 0x10UL) : (GPIO_REG_BASE(x) + 0x1CUL))
#define GPIO_REG_PULLSEL(x)                                                                        \
	(GPIO_IS_GPIOK(x) ? (GPIO_PMGPIO_BASE + 0x14UL) : (GPIO_REG_BASE(x) + 0x20UL))
#define GPIO_REG_CD(x, pin)                                                                        \
	((GPIO_IS_GPIOK(x) ? (GPIO_PMGPIO_BASE + 0x18UL) : (GPIO_REG_BASE(x) + 0x14UL)) +          \
	 (0x4UL * ((pin) / (uint32)16)))
#define GPIO_REG_IEN(x)                                                                            \
	(GPIO_IS_GPIOK(x) ? (GPIO_PMGPIO_BASE + 0x0CUL) : (GPIO_REG_BASE(x) + 0x24UL))
#define GPIO_REG_FN(x, pin) ((GPIO_REG_BASE(x) + 0x30UL) + (0x4UL * ((pin) / (uint32_t)8)))
#define GPIO_MFIO_CFG       (MCU_BSP_GPIO_BASE + (0x2B4UL))
#define GPIO_PERICH_SEL     (MCU_BSP_GPIO_BASE + (0x2B8UL))

#define GPIO_PMGPIO_SEL (GPIO_PMGPIO_BASE + 0x8UL)

#define GPIO_LIST_NUM 6

/*
 * FUNCTION PROTOTYPES
 */

/*
 * vcp_gpio_config
 *
 * @param    [In] port     :   Gpio port index, GPIO_GPX(X)
 * @param    [In] config   :   Gpio configuration options
 * @return
 * Notes
 */
int32_t vcp_gpio_config(uint32_t port, uint32_t config);

/*
 * vcp_gpio_set
 *
 * @param    [In] port     :   Gpio port index, GPIO_GPX(X)
 * @param    [In] data     :   Gpio data value, (0 or 1)
 * @return   0 or 1
 *
 * Notes
 */
int32_t vcp_gpio_set(uint32_t port, uint32_t data);

/*
 * vcp_gpio_peri_chan_sel
 *
 * @param    [In] peri_sel    : Gpio peri select index
 * @param    [In] chan        : Gpio peri select channel index
 * @return   0 or 1
 *
 * Notes
 */
int32_t vcp_gpio_peri_chan_sel(uint32_t peri_sel, uint32_t chan);

/*
 * vcp_gpio_mfio_config
 *
 * @param    [In] peri_sel      :   MFIO peri select index
 * @param    [In] peri_type     :   MFIO peri select type (Disable/GPSB/UART/I2C)
 * @param    [In] chan_sel      :   MFIO channel select index
 * @param    [In] chan_num      :   MFIO channel select value
 * @return   0 or 1
 *
 * Notes
 */
int32_t vcp_gpio_mfio_config(uint32_t peri_sel, uint32_t peri_type, uint32_t chan_sel,
			     uint32_t chan_num);

#endif /* ZEPHYR_INCLUDE_DRIVERS_GPIO_GPIO_TCCVCP_H_ */
