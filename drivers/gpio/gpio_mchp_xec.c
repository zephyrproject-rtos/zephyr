/*
 * Copyright (c) 2019 Intel Corporation
 * Copyright (c) 2025 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_xec_gpio

#include <soc.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/dt-bindings/gpio/gpio.h>
#include <zephyr/dt-bindings/interrupt-controller/mchp-xec-ecia.h>
#include <zephyr/dt-bindings/pinctrl/mchp-xec-pinctrl.h>
#include <zephyr/sys/slist.h>

#define XEC_GPIO_EDGE_DLY_COUNT 8

/* port_base is the base address of the array of 32 CR1 registers.
 * pin in [0, 31] is the GPIO pin in this port.
 */
#define XEC_GPIO_CR1_ADDR(port_base, pin) ((uintptr_t)(port_base) + ((uint32_t)(pin) * 4u))
#define XEC_GPIO_CR2_ADDR(port_base, pin) (XEC_GPIO_CR1_ADDR(port_base, pin) + MEC_GPIO_CR2_OFS)

struct gpio_xec_data {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;
	/* port ISR callback routine address */
	sys_slist_t callbacks;
	/* bitmap of pins configured for level interrupt detection */
	uint32_t level_intr_bm;
};

/* port_base is the base address of an array of 32 32-bit control 1 registers
 *   Control 2 32-bit register array starts offset +0x500
 * parin_addr and parout_addr are the addresses of a single 32-bit register
 * where each bit represents one of the 32 port pins.
 */
struct gpio_xec_devcfg {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;
	uintptr_t port_base;   /* base address of port (32 32-bit control registers) */
	uintptr_t parin_addr;  /* address of one 32-bit register, 1 bit per GPIO */
	uintptr_t parout_addr; /* address of one 32-bit register, 1 bit per GPIO */
	uint32_t flags;
	uint8_t girq;
};

/* NOTE: gpio_flags_t b[0:15] are defined in the dt-binding gpio header.
 * b[31:16] are defined in the driver gpio header.
 */
static int gpio_xec_validate_flags(gpio_flags_t flags)
{
	if (flags & GPIO_LINE_OPEN_SOURCE) {
		return -ENOTSUP;
	}

	if ((flags & GPIO_OUTPUT_INIT_LOW) && (flags & GPIO_OUTPUT_INIT_HIGH)) {
		return -EINVAL;
	}

	return 0;
}

static const uint8_t port_girq_tbl[] = {
	11u, /* port 0 GPIO 000 - 036 */
	10u, /* port 1 GPIO 040 - 076 */
	9u,  /* port 2 GPIO 0100 - 0136 */
	8u,  /* port 3 GPIO 0140 - 0176 */
	12u, /* port 4 GPIO 0200 - 0236 */
	26u, /* port 5 GPIO 0240 - 0276 */
};

#define XEC_GPIO_PORT_FROM_CR1_ADDR(a) (((uint32_t)(a) & 0x3ffu) / 0x80u)

#define XEC_GPIO_ENCODE(port_base, pos)                                                            \
	((((uint32_t)(port_base) & 0x3ffu) / 0x80u) | ((uint32_t)(pos) & 0x1fu))

#define XEC_GPIO_PIN_NUM_TO_PORT_BIT(gpin)                                                         \
	((((uint32_t)(gpin) / 32u) << 8) | ((uint32_t)(gpin) % 32u))

static const uint16_t vbat_pins[] = {
	XEC_GPIO_PIN_NUM_TO_PORT_BIT(0),    XEC_GPIO_PIN_NUM_TO_PORT_BIT(0101),
	XEC_GPIO_PIN_NUM_TO_PORT_BIT(0102), XEC_GPIO_PIN_NUM_TO_PORT_BIT(0161),
	XEC_GPIO_PIN_NUM_TO_PORT_BIT(0162), XEC_GPIO_PIN_NUM_TO_PORT_BIT(0172),
	XEC_GPIO_PIN_NUM_TO_PORT_BIT(0173), XEC_GPIO_PIN_NUM_TO_PORT_BIT(0174),
	XEC_GPIO_PIN_NUM_TO_PORT_BIT(0234)};

static bool is_vbat_pin(uint16_t enc_gpio)
{
	for (size_t i = 0; i < ARRAY_SIZE(vbat_pins); i++) {
		if (vbat_pins[i] == enc_gpio) {
			return true;
		}
	}

	return false;
}

/* Each GPIO pin has two 32-bit control registers. Control 1 configures pin
 * features except for drive strength and slew rate in Control 2.
 * A pin's input and output state can be read/written from either the Control 1
 * register or from corresponding bits in the GPIO parallel input/output registers.
 * The parallel input and output registers group 32 pins into each register.
 * The GPIO hardware restricts the pin output state to Control 1 or the parallel bit.
 * The GPIO CR1 output state bit and corresponding parallel bit are mirrored.
 * Writing the one that is writable is reflected in reading the other.
 *
 * NOTE 1: pins powered by VBAT cannot be completely disconnected. Only input pad can
 *         be disabled. Power gating should remain in VTR mode.
 */
static uint32_t output_config(uint32_t cr1_new, uint32_t cr1_current, uint32_t flags)
{
	uint32_t cr1_out = cr1_new;
	uint32_t pud = MEC_GPIO_CR1_PUD_GET(cr1_new);

	cr1_out |= MEC_GPIO_CR1_DIR_SET(MEC_GPIO_CR1_DIR_OUT);

	if ((flags & GPIO_LINE_OPEN_DRAIN) != 0) {
		cr1_out |= MEC_GPIO_CR1_OBUF_SET(MEC_GPIO_CR1_OBUF_OD);
	} else {
		cr1_out &= ~(MEC_GPIO_CR1_OBUF_SET(MEC_GPIO_CR1_OBUF_OD));
	}

	if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0) {
		cr1_out |= MEC_GPIO_CR1_ODAT_SET(MEC_GPIO_CR1_ODAT_HI);
	} else if ((flags & GPIO_OUTPUT_INIT_LOW) != 0) {
		cr1_out &= ~(MEC_GPIO_CR1_ODAT_SET(MEC_GPIO_CR1_ODAT_HI));
	} else {
		if (MEC_GPIO_CR1_PG_GET(cr1_current) == MEC_GPIO_CR1_PG_UNPWRD) {
			if ((pud == MEC_GPIO_CR1_PUD_PU) || (pud == MEC_GPIO_CR1_PUD_RPT)) {
				cr1_out |= BIT(MEC_GPIO_CR1_ODAT_POS);
			}
		} else {
			if ((cr1_current & BIT(MEC_GPIO_CR1_ODAT_POS)) != 0) {
				cr1_out |= MEC_GPIO_CR1_ODAT_SET(MEC_GPIO_CR1_ODAT_HI);
			} else {
				cr1_out &= ~(MEC_GPIO_CR1_ODAT_SET(MEC_GPIO_CR1_ODAT_HI));
			}
		}
	}

	return cr1_out;
}

static int gpio_xec_configure(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
	const struct gpio_xec_devcfg *devcfg = dev->config;
	uint32_t cr1_addr = 0u, cr1 = 0, cr1_new = 0, msk = 0, regval = 0;
	int ret = 0;
	uint16_t enc_gpio = 0;

	ret = gpio_xec_validate_flags(flags);
	if (ret != 0) {
		return ret;
	}

	cr1_addr = XEC_GPIO_CR1_ADDR(devcfg->port_base, pin);
	cr1 = sys_read32(cr1_addr);

	if (flags == GPIO_DISCONNECTED) {
		enc_gpio = XEC_GPIO_ENCODE(devcfg->port_base, pin);
		regval = cr1 & (uint32_t)~MEC_GPIO_CR1_PG_MSK;
		regval |= MEC_GPIO_CR1_INPD_SET(MEC_GPIO_CR1_INPD_DIS);

		if (is_vbat_pin(enc_gpio) == false) {
			regval |= MEC_GPIO_CR1_PG_SET(MEC_GPIO_CR1_PG_UNPWRD);
		} else {
			regval |= MEC_GPIO_CR1_PG_SET(MEC_GPIO_CR1_PG_VTR);
		}

		sys_write32(regval, cr1_addr);
		return 0;
	}

	/* build new CR1 register value */
	msk = (MEC_GPIO_CR1_PUD_MSK | MEC_GPIO_CR1_PG_MSK | MEC_GPIO_CR1_OBUF_MSK |
	       MEC_GPIO_CR1_DIR_MSK | MEC_GPIO_CR1_OCR_MSK | MEC_GPIO_CR1_FPOL_MSK |
	       MEC_GPIO_CR1_MUX_MSK | MEC_GPIO_CR1_INPD_MSK | MEC_GPIO_CR1_ODAT_MSK);

	cr1_new = (MEC_GPIO_CR1_PG_SET(MEC_GPIO_CR1_PG_VTR) |
		   MEC_GPIO_CR1_OCR_SET(MEC_GPIO_CR1_OCR_PR) |
		   MEC_GPIO_CR1_FPOL_SET(MEC_GPIO_CR1_FPOL_NORM) |
		   MEC_GPIO_CR1_MUX_SET(MEC_GPIO_CR1_MUX_GPIO) |
		   MEC_GPIO_CR1_INPD_SET(MEC_GPIO_CR1_INPD_EN));

	/* internal pull up and pull down. If both are requested the result is repeater mode */
	if ((flags & GPIO_PULL_UP) != 0) {
		cr1_new |= MEC_GPIO_CR1_PUD_SET(MEC_GPIO_CR1_PUD_PU);
		cr1_new |= MEC_GPIO_CR1_ODAT_SET(MEC_GPIO_CR1_ODAT_HI);
	}

	if ((flags & GPIO_PULL_DOWN) != 0) {
		cr1_new |= MEC_GPIO_CR1_PUD_SET(MEC_GPIO_CR1_PUD_PD);
	}

	cr1_new |= BIT(MEC_GPIO_CR1_INPD_POS);           /* disable input pad */
	cr1_new &= (uint32_t)~BIT(MEC_GPIO_CR1_DIR_POS); /* direction is input */

	if ((flags & GPIO_OUTPUT) != 0) { /* set direction to output */
		cr1_new = output_config(cr1_new, cr1, flags);
	}

	if ((flags & GPIO_INPUT) != 0) {                /* input direction also requested */
		cr1_new &= ~BIT(MEC_GPIO_CR1_INPD_POS); /* enable input pad */
	}

	cr1 = sys_read32(cr1_addr);
	if ((cr1 & msk) != cr1_new) {
		/* first write of CR1 use CR1 output control to write control and
		 * output state in one register write. GPIO hardware will reflect
		 * the output state from CR1 into corresponding parallel output bit.
		 */
		cr1_new |= cr1 & ~msk;
		cr1_new &= (uint32_t)~(MEC_GPIO_CR1_OCR_MSK);
		sys_write32(cr1_new, cr1_addr);

		/* Enable parallel output bit for this pin. Parallel input is always enabled. */
		sys_set_bit(cr1_addr, MEC_GPIO_CR1_OCR_POS);
	}

	return 0;
}

static uint8_t gen_gpio_ctrl_icfg(enum gpio_int_mode mode, enum gpio_int_trig trig)
{
	uint8_t idet;

	if (mode == GPIO_INT_MODE_DISABLED) {
		idet = MEC_GPIO_CR1_IDET_DIS;
	} else {
		if (mode == GPIO_INT_MODE_LEVEL) {
			if (trig == GPIO_INT_TRIG_HIGH) {
				idet = MEC_GPIO_CR1_IDET_LH;
			} else {
				idet = MEC_GPIO_CR1_IDET_LL;
			}
		} else {
			switch (trig) {
			case GPIO_INT_TRIG_LOW:
				idet = MEC_GPIO_CR1_IDET_FE;
				break;
			case GPIO_INT_TRIG_HIGH:
				idet = MEC_GPIO_CR1_IDET_RE;
				break;
			case GPIO_INT_TRIG_BOTH:
				idet = MEC_GPIO_CR1_IDET_BE;
				break;
			default:
				idet = MEC_GPIO_CR1_IDET_DIS;
				break;
			}
		}
	}

	return idet;
}

/* GPIO device is for each bank of 32 GPIO pins.
 * 0 <= pin <= 31
 */
static int gpio_xec_pin_interrupt_config(const struct device *dev, gpio_pin_t pin,
					 enum gpio_int_mode mode, enum gpio_int_trig trig)
{
	const struct gpio_xec_devcfg *devcfg = dev->config;
	struct gpio_xec_data *data = dev->data;
	uint32_t cr1 = 0, cr1_addr = 0, idet = 0, idet_curr = 0, port_num = 0;

	/* Check if GPIO port supports interrupts */
	if ((mode != GPIO_INT_MODE_DISABLED) && !(devcfg->flags & GPIO_INT_ENABLE)) {
		return -ENOTSUP;
	}

	port_num = XEC_GPIO_PORT_FROM_CR1_ADDR(devcfg->port_base);

	cr1_addr = XEC_GPIO_CR1_ADDR(devcfg->port_base, pin);
	cr1 = sys_read32(cr1_addr);

	if (mode == GPIO_INT_MODE_DISABLED) {
		cr1 &= (uint32_t)~(MEC_GPIO_CR1_IDET_MSK);
		cr1 |= MEC_GPIO_CR1_IDET_SET(MEC_GPIO_CR1_IDET_DIS);
		sys_write32(cr1, cr1_addr);
		soc_ecia_girq_ctrl(devcfg->girq, pin, 0);
		soc_ecia_girq_status_clear(devcfg->girq, pin);
		data->level_intr_bm &= (uint32_t)~BIT(pin);
		return 0;
	}

	idet_curr = MEC_GPIO_CR1_IDET_GET(cr1);
	idet = gen_gpio_ctrl_icfg(mode, trig);

	if (idet_curr == idet) {
		return 0;
	}

	if ((idet == MEC_GPIO_CR1_IDET_LL) || (idet == MEC_GPIO_CR1_IDET_LH)) {
		data->level_intr_bm |= BIT(pin);
	} else {
		data->level_intr_bm &= (uint32_t)~BIT(pin);
	}

	cr1 &= (uint32_t)~(MEC_GPIO_CR1_IDET_MSK);
	cr1 |= (uint32_t)MEC_GPIO_CR1_IDET_SET(idet);
	sys_write32(cr1, cr1_addr);

	/* MEC GPIO HW always signals interrupt if detection mode is changed from
	 * disabled to enabled even if input is not in state to cause an interrupt!
	 */
	if (idet_curr == MEC_GPIO_CR1_IDET_DIS) {
		for (int i = 0; i < XEC_GPIO_EDGE_DLY_COUNT; i++) {
			sys_read32(cr1_addr); /* bus reads take >= 3 bus clocks (AHB) */
		}

		soc_ecia_girq_status_clear(devcfg->girq, pin);
	}

	soc_ecia_girq_ctrl(devcfg->girq, pin, 1u);

	return 0;
}

static int gpio_xec_port_set_masked_raw(const struct device *dev, uint32_t mask, uint32_t value)
{
	const struct gpio_xec_devcfg *devcfg = dev->config;
	uint32_t parout_cr_addr = devcfg->parout_addr;
	uint32_t regval = sys_read32(parout_cr_addr);

	regval &= (uint32_t)~mask;
	regval |= (value & mask);
	sys_write32(regval, parout_cr_addr);

	return 0;
}

static int gpio_xec_port_set_bits_raw(const struct device *dev, uint32_t mask)
{
	const struct gpio_xec_devcfg *devcfg = dev->config;
	uint32_t parout_cr_addr = devcfg->parout_addr;

	sys_set_bits(parout_cr_addr, mask);

	return 0;
}

static int gpio_xec_port_clear_bits_raw(const struct device *dev, uint32_t mask)
{
	const struct gpio_xec_devcfg *devcfg = dev->config;
	uint32_t parout_cr_addr = devcfg->parout_addr;

	sys_clear_bits(parout_cr_addr, mask);

	return 0;
}

static int gpio_xec_port_toggle_bits(const struct device *dev, uint32_t mask)
{
	const struct gpio_xec_devcfg *devcfg = dev->config;
	uint32_t parout_cr_addr = devcfg->parout_addr;
	uint32_t regval = sys_read32(parout_cr_addr);

	regval ^= mask;
	sys_write32(regval, parout_cr_addr);

	return 0;
}

static int gpio_xec_port_get_raw(const struct device *dev, uint32_t *value)
{
	const struct gpio_xec_devcfg *devcfg = dev->config;
	uint32_t parin_cr_addr = devcfg->parin_addr;

	if (value == NULL) {
		return -EINVAL;
	}

	*value = sys_read32(parin_cr_addr);

	return 0;
}

static int gpio_xec_manage_callback(const struct device *dev, struct gpio_callback *cb, bool set)
{
	struct gpio_xec_data *data = dev->data;

	gpio_manage_callback(&data->callbacks, cb, set);

	return 0;
}

#ifdef CONFIG_GPIO_GET_DIRECTION
static int gpio_xec_get_direction(const struct device *port_dev, gpio_port_pins_t map,
				  gpio_port_pins_t *inputs, gpio_port_pins_t *outputs)
{
	const struct gpio_xec_devcfg *devcfg = port_dev->config;
	uint32_t cr1 = 0u, cr1_addr = 0u, inp = 0u, outp = 0u;
	uint8_t pwr_gate = 0u, dir = 0u, in_pad_dis = 0u;

	for (uint8_t pin_pos = 0; pin_pos < 32; pin_pos++) {
		if (map == 0) {
			break;
		}

		if ((map & BIT(pin_pos)) != 0) {
			cr1_addr = XEC_GPIO_CR1_ADDR(devcfg->port_base, pin_pos);
			cr1 = sys_read32(cr1_addr);

			pwr_gate = (uint8_t)MEC_GPIO_CR1_PG_GET(cr1);
			dir = (uint8_t)MEC_GPIO_CR1_DIR_GET(cr1);
			in_pad_dis = (uint8_t)MEC_GPIO_CR1_INPD_GET(cr1);

			if (pwr_gate != MEC_GPIO_CR1_PG_UNPWRD) {
				if (dir == MEC_GPIO_CR1_DIR_OUT) {
					outp |= BIT(pin_pos);
				}

				if (in_pad_dis == MEC_GPIO_CR1_INPD_EN) {
					inp |= BIT(pin_pos);
				}
			}

			map &= ~BIT(pin_pos);
		}
	}

	if (inputs != NULL) {
		*inputs = inp;
	}

	if (outputs != NULL) {
		*outputs = outp;
	}

	return 0;
}
#endif

#ifdef CONFIG_GPIO_GET_CONFIG
int gpio_xec_get_config(const struct device *port_dev, gpio_pin_t pin, gpio_flags_t *flags)
{
	const struct gpio_xec_devcfg *devcfg = port_dev->config;
	uint32_t cr1 = 0, cr1_addr = 0, pin_flags = 0;

	if ((flags == NULL) || (pin >= MEC_GPIO_PINS_PER_PORT)) {
		return -EINVAL;
	}

	cr1_addr = XEC_GPIO_CR1_ADDR(devcfg->port_base, pin);
	cr1 = sys_read32(cr1_addr);

	if (MEC_GPIO_CR1_DIR_GET(cr1) == MEC_GPIO_CR1_DIR_OUT) {
		pin_flags |= GPIO_OUTPUT;

		if (MEC_GPIO_CR1_ODAT_GET(cr1) == MEC_GPIO_CR1_ODAT_HI) {
			pin_flags |= GPIO_OUTPUT_INIT_HIGH;
		} else {
			pin_flags |= GPIO_OUTPUT_INIT_LOW;
		}

		if (MEC_GPIO_CR1_OBUF_GET(cr1) == MEC_GPIO_CR1_OBUF_OD) {
			pin_flags |= GPIO_OPEN_DRAIN;
		}
	} else {
		if (MEC_GPIO_CR1_INPD_GET(cr1) == MEC_GPIO_CR1_INPD_EN) {
			pin_flags |= GPIO_INPUT;
		}
	}

	if (pin_flags != 0) {
		*flags = pin_flags;
	} else {
		*flags = GPIO_DISCONNECTED;
	}

	return 0;
}
#endif

static void gpio_xec_port_isr(const struct device *port_dev)
{
	const struct gpio_xec_devcfg *devcfg = port_dev->config;
	struct gpio_xec_data *data = port_dev->data;
	uint32_t girq_result = 0xffffffffu;
	uint32_t lvl_bm = 0, port_num = 0u;
	uint8_t girq = 0;

	/* GIRQ result register indicates which GPIO pins on the port have detected
	 * in interrupt event.
	 */
	port_num = XEC_GPIO_PORT_FROM_CR1_ADDR(devcfg->port_base);
	girq = port_girq_tbl[port_num];

	soc_ecia_girq_result(girq, &girq_result);
	/* if level detect and external signal is still asserting, this will
	 * not be able to clear the interrupt status.
	 */
	soc_ecia_girq_status_clear_bm(girq, girq_result);

	gpio_fire_callbacks(&data->callbacks, port_dev, girq_result);

	/* Clear a second time to handle level case */
	lvl_bm = girq_result & data->level_intr_bm;
	if (lvl_bm) {
		soc_ecia_girq_status_clear_bm(girq, lvl_bm);
	}
}

/* GPIO driver official API table */
static DEVICE_API(gpio, gpio_xec_driver_api) = {
	.pin_configure = gpio_xec_configure,
	.port_get_raw = gpio_xec_port_get_raw,
	.port_set_masked_raw = gpio_xec_port_set_masked_raw,
	.port_set_bits_raw = gpio_xec_port_set_bits_raw,
	.port_clear_bits_raw = gpio_xec_port_clear_bits_raw,
	.port_toggle_bits = gpio_xec_port_toggle_bits,
	.pin_interrupt_configure = gpio_xec_pin_interrupt_config,
	.manage_callback = gpio_xec_manage_callback,
#ifdef CONFIG_GPIO_GET_DIRECTION
	.port_get_direction = gpio_xec_get_direction,
#endif
#ifdef CONFIG_GPIO_GET_CONFIG
	.pin_get_config = gpio_xec_get_config,
#endif
};

#define XEC_GPIO_PORT_FLAGS(inst) ((DT_INST_IRQ_HAS_CELL(inst, irq)) ? GPIO_INT_ENABLE : 0)
#define XEC_GPIO_GIRQ(inst)       (MCHP_XEC_ECIA_GIRQ(DT_INST_PROP_BY_IDX(inst, girqs, 0)))

#define XEC_GPIO_PORT(i)                                                                           \
	static int gpio_xec_port_init##i(const struct device *dev)                                 \
	{                                                                                          \
		const struct gpio_xec_devcfg *devcfg = dev->config;                                \
		if (!(DT_INST_IRQ_HAS_CELL(i, irq))) {                                             \
			return 0;                                                                  \
		}                                                                                  \
		soc_ecia_girq_aggr_ctrl(devcfg->girq, 1u);                                         \
		IRQ_CONNECT(DT_INST_IRQN(i), DT_INST_IRQ(i, priority), gpio_xec_port_isr,          \
			    DEVICE_DT_INST_GET(i), 0u);                                            \
		irq_enable(DT_INST_IRQN(i));                                                       \
		return 0;                                                                          \
	}                                                                                          \
	static struct gpio_xec_data gpio_xec_port_data##i;                                         \
	static const struct gpio_xec_devcfg gpio_xec_dcfg##i = {                                   \
		.common = GPIO_COMMON_CONFIG_FROM_DT_INST(i),                                      \
		.port_base = (uintptr_t)DT_INST_REG_ADDR_BY_IDX(i, 0),                             \
		.parin_addr = (uintptr_t)DT_INST_REG_ADDR_BY_IDX(i, 1),                            \
		.parout_addr = (uintptr_t)DT_INST_REG_ADDR_BY_IDX(i, 2),                           \
		.flags = XEC_GPIO_PORT_FLAGS(i),                                                   \
		.girq = (uint8_t)XEC_GPIO_GIRQ(i),                                                 \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(i, gpio_xec_port_init##i, NULL, &gpio_xec_port_data##i,              \
			      &gpio_xec_dcfg##i, PRE_KERNEL_1, CONFIG_GPIO_INIT_PRIORITY,          \
			      &gpio_xec_driver_api);

DT_INST_FOREACH_STATUS_OKAY(XEC_GPIO_PORT)
