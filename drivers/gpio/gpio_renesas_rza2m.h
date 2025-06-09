/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_GPIO_GPIO_RENESAS_RZA2M_H_
#define ZEPHYR_DRIVERS_GPIO_GPIO_RENESAS_RZA2M_H_

#include <zephyr/sys/device_mmio.h>

#define RZA2M_PDR_OFFSET  0x0000
#define RZA2M_PODR_OFFSET 0x0040
#define RZA2M_PIDR_OFFSET 0x0060
#define RZA2M_PMR_OFFSET  0x0080
#define RZA2M_DSCR_OFFSET 0x0140
#define RZA2M_PFS_OFFSET  0x0200
#define RZA2M_PWPR_OFFSET 0x02FF

#define RZA2M_PDR(dev, port)      (DEVICE_MMIO_GET(dev) + RZA2M_PDR_OFFSET + ((port) * 2))
#define RZA2M_PODR(dev, port)     (DEVICE_MMIO_GET(dev) + RZA2M_PODR_OFFSET + (port))
#define RZA2M_PIDR(dev, port)     (DEVICE_MMIO_GET(dev) + RZA2M_PIDR_OFFSET + (port))
#define RZA2M_PMR(dev, port)      (DEVICE_MMIO_GET(dev) + RZA2M_PMR_OFFSET + (port))
#define RZA2M_DSCR(dev, port)     (DEVICE_MMIO_GET(dev) + RZA2M_DSCR_OFFSET + ((port) * 2))
#define RZA2M_PFS(dev, port, pin) (DEVICE_MMIO_GET(dev) + RZA2M_PFS_OFFSET + ((port) * 8) + (pin))
#define RZA2M_PWPR(dev)           (DEVICE_MMIO_GET(dev) + RZA2M_PWPR_OFFSET)

#define RZA2M_GICD_ICFGR31               0xE8221C7C
#define RZA2M_GICD_ICFGR30               0xE8221C78
#define RZA2M_GPIO_DRIVE_STRENGTH_HIGH   0b11
#define RZA2M_GPIO_DRIVE_STRENGTH_NORMAL 0x01
#define RZA2M_DSCR_MASK                  0x03
#define RZA2M_PDR_MASK                   0x03
#define RZA2M_PDR_INPUT                  0x02
#define RZA2M_PDR_OUTPUT                 0x03
#define RZA2M_PWPR_PFSWE_MASK            0x40
#define RZA2M_PWPR_B0WI_MASK             0x80
#define RZA2M_PFS_ISEL_MASK              0x40

/* As per Table 51.37 of the hardware manual, each TINT group supports up to 2 ports. */
#define RZA2M_MAX_PORTS_PER_TINT 2

#define TINT0  480
#define TINT1  481
#define TINT2  482
#define TINT3  483
#define TINT4  484
#define TINT5  485
#define TINT6  486
#define TINT7  487
#define TINT8  488
#define TINT9  489
#define TINT10 490
#define TINT11 491
#define TINT12 492
#define TINT13 493
#define TINT14 494
#define TINT15 495
#define TINT16 496
#define TINT17 497
#define TINT18 498
#define TINT19 499
#define TINT20 500
#define TINT21 501
#define TINT22 502
#define TINT23 503
#define TINT24 504
#define TINT25 505
#define TINT26 506
#define TINT27 507
#define TINT28 508
#define TINT29 509
#define TINT30 510
#define TINT31 511

#define PORT0 0x00
#define PORT1 0x01
#define PORT2 0x02
#define PORT3 0x03
#define PORT4 0x04
#define PORT5 0x05
#define PORT6 0x06
#define PORT7 0x07
#define PORT8 0x08
#define PORT9 0x09
#define PORTA 0x0A
#define PORTB 0x0B
#define PORTC 0x0C
#define PORTD 0x0D
#define PORTE 0x0E
#define PORTF 0x0F
#define PORTG 0x10
#define PORTH 0x11
#define PORTJ 0x12
#define PORTK 0x13
#define PORTL 0x14
#define PORTM 0x15

#define UNUSED_PORT 0xFF
#define UNUSED_MASK 0x00

struct gpio_rza2m_tint_config {
	DEVICE_MMIO_ROM;
	void (*gpio_int_init)(void);
};

struct gpio_rza2m_tint_data {
	DEVICE_MMIO_RAM;
};

typedef struct gpio_rza2m_high_allowed_pin {
	uint8_t port;
	uint8_t mask;
} gpio_rza2m_high_allowed_pin_t;

struct gpio_rza2m_port_config {
	struct gpio_driver_config common;
	uint8_t port;
	uint8_t ngpios;
	const struct device *int_dev;
};

struct gpio_rza2m_port_data {
	struct gpio_driver_data common;
	sys_slist_t callbacks;
	uint8_t mask_irq_en;
};

typedef struct gpio_rza2m_port_tint_map {
	uint32_t tint;
	uint32_t ports[RZA2M_MAX_PORTS_PER_TINT];
	uint16_t masks[RZA2M_MAX_PORTS_PER_TINT];
} gpio_rza2m_port_tint_map_t;

enum gpio_rza2m_tint_sense {
	RZA2M_GPIO_TINT_SENSE_HIGH_LEVEL,
	RZA2M_GPIO_TINT_SENSE_RISING_EDGE,
};

#endif /* ZEPHYR_DRIVERS_GPIO_GPIO_RENESAS_RZA2M_H_ */
