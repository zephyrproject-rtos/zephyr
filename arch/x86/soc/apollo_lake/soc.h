/*
 * Copyright (c) 2018, Intel Corporation
 * Copyright (c) 2010-2015, Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Board configuration macros for the Apollo Lake SoC
 *
 * This header file is used to specify and describe soc-level aspects for
 * the 'Apollo Lake' SoC.
 */

#ifndef __SOC_H_
#define __SOC_H_

#include <misc/util.h>

#ifndef _ASMLANGUAGE
#include <device.h>
#include <random/rand32.h>
#endif

#ifdef CONFIG_PCI

/*
 * PCI definitions
 */
#define PCI_BUS_NUMBERS                         1

#define PCI_CTRL_ADDR_REG                       0xCF8
#define PCI_CTRL_DATA_REG                       0xCFC

/**
 * @brief Convert PCI interrupt PIN to IRQ
 *
 * BIOS should have assigned vectors linearly.
 * If not, override this in board configuration.
 */
#define pci_pin2irq(bus, dev, pin)              (pin)


/* UARTs */
#ifdef CONFIG_UART_NS16550_PCI

#ifdef CONFIG_UART_NS16550_PORT_0_PCI

#define UART_NS16550_PORT_0_PCI_CLASS           0x11
#define UART_NS16550_PORT_0_PCI_BUS             0
#define UART_NS16550_PORT_0_PCI_DEV             18
#define UART_NS16550_PORT_0_PCI_VENDOR_ID       0x8086
#define UART_NS16550_PORT_0_PCI_DEVICE_ID       0x5abc
#define UART_NS16550_PORT_0_PCI_FUNC            0
#define UART_NS16550_PORT_0_PCI_BAR             0

#endif /* CONFIG_UART_NS16550_PORT_0_PCI */

#ifdef CONFIG_UART_NS16550_PORT_1_PCI

#define UART_NS16550_PORT_1_PCI_CLASS           0x11
#define UART_NS16550_PORT_1_PCI_BUS             0
#define UART_NS16550_PORT_1_PCI_DEV             18
#define UART_NS16550_PORT_1_PCI_VENDOR_ID       0x8086
#define UART_NS16550_PORT_1_PCI_DEVICE_ID       0x5abe
#define UART_NS16550_PORT_1_PCI_FUNC            1
#define UART_NS16550_PORT_1_PCI_BAR             0

#endif /* CONFIG_UART_NS16550_PORT_1_PCI */

#ifdef CONFIG_UART_NS16550_PORT_2_PCI

#define UART_NS16550_PORT_2_PCI_CLASS           0x11
#define UART_NS16550_PORT_2_PCI_BUS             0
#define UART_NS16550_PORT_2_PCI_DEV             18
#define UART_NS16550_PORT_2_PCI_VENDOR_ID       0x8086
#define UART_NS16550_PORT_2_PCI_DEVICE_ID       0x5ac0
#define UART_NS16550_PORT_2_PCI_FUNC            2
#define UART_NS16550_PORT_2_PCI_BAR             0

#endif /* CONFIG_UART_NS16550_PORT_2_PCI */

#ifdef CONFIG_UART_NS16550_PORT_3_PCI

#define UART_NS16550_PORT_3_PCI_CLASS           0x11
#define UART_NS16550_PORT_3_PCI_BUS             0
#define UART_NS16550_PORT_3_PCI_DEV             18
#define UART_NS16550_PORT_3_PCI_VENDOR_ID       0x8086
#define UART_NS16550_PORT_3_PCI_DEVICE_ID       0x5aee
#define UART_NS16550_PORT_3_PCI_FUNC            3
#define UART_NS16550_PORT_3_PCI_BAR             0

#endif /* CONFIG_UART_NS16550_PORT_3_PCI */

#endif /* CONFIG_UART_NS16550_PCI */

/* I2C controllers */
#define I2C_DW_0_PCI_VENDOR_ID                  0x8086
#define I2C_DW_0_PCI_DEVICE_ID                  0x5aac
#define I2C_DW_0_PCI_CLASS                      0x11
#define I2C_DW_0_PCI_BUS                        0
#define I2C_DW_0_PCI_DEV                        16
#define I2C_DW_0_PCI_FUNCTION                   0
#define I2C_DW_0_PCI_BAR                        0

#define I2C_DW_1_PCI_VENDOR_ID                  0x8086
#define I2C_DW_1_PCI_DEVICE_ID                  0x5aae
#define I2C_DW_1_PCI_CLASS                      0x11
#define I2C_DW_1_PCI_BUS                        0
#define I2C_DW_1_PCI_DEV                        16
#define I2C_DW_1_PCI_FUNCTION                   1
#define I2C_DW_1_PCI_BAR                        0

#define I2C_DW_2_PCI_VENDOR_ID                  0x8086
#define I2C_DW_2_PCI_DEVICE_ID                  0x5ab0
#define I2C_DW_2_PCI_CLASS                      0x11
#define I2C_DW_2_PCI_BUS                        0
#define I2C_DW_2_PCI_DEV                        16
#define I2C_DW_2_PCI_FUNCTION                   2
#define I2C_DW_2_PCI_BAR                        0

#define I2C_DW_3_PCI_VENDOR_ID                  0x8086
#define I2C_DW_3_PCI_DEVICE_ID                  0x5ab2
#define I2C_DW_3_PCI_CLASS                      0x11
#define I2C_DW_3_PCI_BUS                        0
#define I2C_DW_3_PCI_DEV                        16
#define I2C_DW_3_PCI_FUNCTION                   3
#define I2C_DW_3_PCI_BAR                        0

#define I2C_DW_4_PCI_VENDOR_ID                  0x8086
#define I2C_DW_4_PCI_DEVICE_ID                  0x5ab4
#define I2C_DW_4_PCI_CLASS                      0x11
#define I2C_DW_4_PCI_BUS                        0
#define I2C_DW_4_PCI_DEV                        17
#define I2C_DW_4_PCI_FUNCTION                   0
#define I2C_DW_4_PCI_BAR                        0

#define I2C_DW_5_PCI_VENDOR_ID                  0x8086
#define I2C_DW_5_PCI_DEVICE_ID                  0x5ab6
#define I2C_DW_5_PCI_CLASS                      0x11
#define I2C_DW_5_PCI_BUS                        0
#define I2C_DW_5_PCI_DEV                        17
#define I2C_DW_5_PCI_FUNCTION                   1
#define I2C_DW_5_PCI_BAR                        0

#define I2C_DW_6_PCI_VENDOR_ID                  0x8086
#define I2C_DW_6_PCI_DEVICE_ID                  0x5ab8
#define I2C_DW_6_PCI_CLASS                      0x11
#define I2C_DW_6_PCI_BUS                        0
#define I2C_DW_6_PCI_DEV                        17
#define I2C_DW_6_PCI_FUNCTION                   2
#define I2C_DW_6_PCI_BAR                        0

#define I2C_DW_7_PCI_VENDOR_ID                  0x8086
#define I2C_DW_7_PCI_DEVICE_ID                  0x5aba
#define I2C_DW_7_PCI_CLASS                      0x11
#define I2C_DW_7_PCI_BUS                        0
#define I2C_DW_7_PCI_DEV                        17
#define I2C_DW_7_PCI_FUNCTION                   3
#define I2C_DW_7_PCI_BAR                        0

#endif /* CONFIG_PCI */

#endif /* __SOC_H_ */
