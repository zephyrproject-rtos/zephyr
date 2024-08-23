/*
 * Copyright (c) 2023-2024 MUNIC SA
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */
#include <zephyr/irq.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/drivers/pinctrl/pinctrl_ra2.h>

struct pinmux_ra_config {
	uint16_t   offset;
	uint16_t   valid_pins;
	uint8_t    ngpios;
};

#define DT_DRV_COMPAT	renesas_ra2_ioports

#define RA_IOPORTS_NODE		DT_DRV_INST(0)

#define RA_IOPORTS_BASE		\
		((mem_addr_t)DT_REG_ADDR_BY_NAME(RA_IOPORTS_NODE, base))
#define RA_IOPORTS_PWPR		\
		(RA_IOPORTS_BASE + DT_REG_ADDR_BY_NAME(RA_IOPORTS_NODE, pwpr))
#define RA_IOPORTS_PWPR_PFSWE	BIT(6)
#define RA_IOPORTS_PWPR_B0WI	BIT(7)
#define RA_IOPORTS_PRWCNTR	\
	(RA_IOPORTS_BASE + DT_REG_ADDR_BY_NAME(RA_IOPORTS_NODE, prwcntr))

#define DEF_IOPORT_CFG_ITEM(node_id)		\
	[DT_NODE_CHILD_IDX(node_id)] = {	\
		.offset = (uint16_t)DT_REG_ADDR(node_id),		       \
		.valid_pins = GPIO_PORT_PIN_MASK_FROM_DT_NODE(node_id),	       \
		.ngpios = DT_PROP(node_id, ngpios),			       \
	},

#define DEF_IOPORT_CONFS(node_id)	\
	COND_CODE_1(DT_NODE_HAS_COMPAT(node_id, renesas_ra2_pfs), \
		(DEF_IOPORT_CFG_ITEM(node_id)),				       \
		(EMPTY)							       \
	)

static const struct pinmux_ra_config ra_ioports[] = {
	DT_FOREACH_CHILD(RA_IOPORTS_NODE, DEF_IOPORT_CONFS)
};

#define GET_CHILD_ID_MSK(node)		\
	COND_CODE_1(DT_NODE_HAS_COMPAT(node, renesas_ra2_pfs), \
		(BIT(DT_NODE_CHILD_IDX(node))),				       \
		(0)							       \
	)

#define RA_VALID_IOPORTS (DT_FOREACH_CHILD_STATUS_OKAY_SEP(RA_IOPORTS_NODE,    \
				GET_CHILD_ID_MSK, (|)))

static inline
int pinctrl_configure_pin(const pinctrl_soc_pin_t *pinctrl)
{
	int ret = -ENODEV;
	const struct pinmux_ra_config *cfg;
	unsigned int port, pin;

	if (!pinctrl) {
		return -EINVAL;
	}

	pin  = RA_PIN_GET_PIN(*pinctrl);
	port = RA_PIN_GET_PORT(*pinctrl);

	if (port >= ARRAY_SIZE(ra_ioports) ||
			(BIT(port) & RA_VALID_IOPORTS) == 0) {
		return -EINVAL;
	}

	cfg = &ra_ioports[port];

	if (pin < cfg->ngpios && (BIT(pin) & cfg->valid_pins)) {
		mm_reg_t addr = RA_IOPORTS_BASE +
				cfg->offset + (mm_reg_t)pin * 4;
		uint32_t val = *pinctrl & RA_PIN_FLAGS_MASK;

		/* Direction and pull-ups must be configured before function
		 * activation
		 */
		sys_write32(val & ~RA_PIN_FLAGS_PMR, addr);
		sys_write32(val, addr);

		ret = 0;
	}

	return ret;
}

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt,
			   uintptr_t reg)
{
	int ret = 0;
	uint32_t key = irq_lock();

	ARG_UNUSED(reg);

	/* Clear BOWI */
	sys_write8(0, RA_IOPORTS_PWPR);
	/* Set PFSWE */
	sys_write8(RA_IOPORTS_PWPR_PFSWE, RA_IOPORTS_PWPR);

	for (; pin_cnt; pin_cnt--, pins++) {
		ret = pinctrl_configure_pin(pins);

		if (ret) {
			break;
		}
	}

	/* Clear PFSWE */
	sys_write8(0, RA_IOPORTS_PWPR);
	/* Set BOWI */
	sys_write8(RA_IOPORTS_PWPR_B0WI, RA_IOPORTS_PWPR);

	irq_unlock(key);

	return ret;
}

/* This is an extension to the pinctrl API: returns the PIN config.
 *
 * Actually used by the GPIO driver.
 *
 * NB: When/if Renesas starts manufacturing multicore chips, this function
 * must be modified to use spinlock to synchronise port access with
 * pinctrl_configure_pins().
 */
int pinctrl_ra_get_pin(unsigned int port_id, gpio_pin_t pin_id,
		pinctrl_soc_pin_t *pin)
{
	const struct pinmux_ra_config *cfg;

	if (port_id >= ARRAY_SIZE(ra_ioports) ||
			(BIT(port_id) & RA_VALID_IOPORTS) == 0 || !pin) {
		return -EINVAL;
	}

	cfg = &ra_ioports[port_id];

	if (pin_id < cfg->ngpios && (BIT(pin_id) & cfg->valid_pins)) {
		mm_reg_t addr = RA_IOPORTS_BASE +
				cfg->offset + (mm_reg_t)pin_id * 4;

		*pin = sys_read32(addr) |
				RA_PIN_FLAGS_PIN(pin_id) |
				RA_PIN_FLAGS_PORT(port_id);
		return 0;
	}

	return -ENODEV;
}

#if DT_HAS_COMPAT_STATUS_OKAY(DT_RA2_IOPORTS_COMPAT)
/* Ioports driver initialization */
static int ra_ioports_init(void)
{
	sys_write8(DT_PROP(RA_IOPORTS_NODE, prwcntr), RA_IOPORTS_PRWCNTR);
	return 0;
}

SYS_INIT(ra_ioports_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_RA2_IOPORTS_COMPAT) */
