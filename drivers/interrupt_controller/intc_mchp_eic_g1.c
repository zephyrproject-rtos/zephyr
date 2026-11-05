/*
 * Copyright (c) 2025-2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/mchp_clock_control.h>
#include <zephyr/drivers/interrupt_controller/intc_mchp_eic_g1.h>

#define DT_DRV_COMPAT microchip_eic_g1_intc

LOG_MODULE_REGISTER(intc_mchp_eic_g1, CONFIG_INTC_LOG_LEVEL);

/* This is used for checking whether the eic line is free or not */
#define INTC_LINE_FREE                       0XFF
/* This is the default value of the pin in the `struct mchp_eic_line_assignment`*/
#define INTC_PIN_DEFAULT_VAL                 0x1f
/* This is the default value of the port in the `struct mchp_eic_line_assignment`*/
#define INTC_PORT_DEFAULT_VAL                0X7
/*This mask is used to for clearing the EIC_CONFIG for each eic line */
#define EIC_CONFIG_EIC_LINE_MSK              0xf
/* This is used to get the EIC config register index (0 or 1) */
#define EIC_CONFIG_REG_IDX(eic_line)         ((eic_line) >> 3)
/* Gets the offset inside the config register that is to be updated. At most 8 eic lines
 * configuration is present inside the config register.
 */
#define EIC_CONFIG_EIC_LINE_OFFSET(eic_line) ((eic_line) & 7)
/* Denotes the allotted bits for each bits inside config structure*/
#define NUM_OF_BITS_FOR_EACH_LINE            4
/* finds out the trig type bit position inside the configX register*/
#define EIC_TRIG_TYPE_BIT_POS(eic_line)                                                            \
	(NUM_OF_BITS_FOR_EACH_LINE * EIC_CONFIG_EIC_LINE_OFFSET(eic_line))
/* Port A */
#define PORTA_UNSUPPORTED_PINS      DT_INST_PROP(0, porta_unsupported_pins)
#define PORTA_SPECIAL_PINS_1        DT_INST_PROP_BY_IDX(0, porta_special_pins_1, 0)
#define PORTA_SPECIAL_PINS_1_OFFSET DT_INST_PROP_BY_IDX(0, porta_special_pins_1, 1)

/* Port B */
#define PORTB_UNSUPPORTED_PINS    DT_INST_PROP(0, portb_unsupported_pins)
/* The special pins need an offset when calculating the eic line */
#define PORTB_SPECIAL_PINS        DT_INST_PROP_BY_IDX(0, portb_special_pins_1, 0)
#define PORTB_SPECIAL_PINS_OFFSET DT_INST_PROP_BY_IDX(0, portb_special_pins_1, 1)

/* Port C */
#define PORTC_SUPPORTED_PINS        DT_INST_PROP(0, portc_supported_pins)
#define PORTC_UNSUPPORTED_PINS      DT_INST_PROP(0, portc_unsupported_pins)
/* The special pins need an offset when calculating the eic line */
#define PORTC_SPECIAL_PINS_1        DT_INST_PROP_BY_IDX(0, portc_special_pins_1, 0)
#define PORTC_SPECIAL_PINS_1_OFFSET DT_INST_PROP_BY_IDX(0, portc_special_pins_1, 1)
#define PORTC_SPECIAL_PINS_2        DT_INST_PROP_BY_IDX(0, portc_special_pins_2, 0)
#define PORTC_SPECIAL_PINS_2_OFFSET DT_INST_PROP_BY_IDX(0, portc_special_pins_2, 1)

/* Port D */
#define PORTD_SUPPORTED_PINS DT_INST_PROP(0, portd_supported_pins)

/* The special pins need an offset when calculating the eic line */
#define PORTD_SPECIAL_PINS_1        DT_INST_PROP_BY_IDX(0, portd_special_pins_1, 0)
#define PORTD_SPECIAL_PINS_2        DT_INST_PROP_BY_IDX(0, portd_special_pins_2, 0)
#define PORTD_SPECIAL_PINS_1_OFFSET DT_INST_PROP_BY_IDX(0, portd_special_pins_1, 1)
#define PORTD_SPECIAL_PINS_2_OFFSET DT_INST_PROP_BY_IDX(0, portd_special_pins_2, 1)

#define EIC_LINES_PER_PORT 16

/*
 * Get port index from port ID for secure GPIO ports.
 * Secure GPIO ports (portX_secure) have addresses offset by 0x1000 from non-secure,
 * causing the GPIO driver to calculate port IDs >= MCHP_PORT_ID_MAX.
 * Mask to get actual port index 0-3.
 */
#define GET_PORT_IDX(port_id) ((port_id) & (MCHP_PORT_ID_MAX - 1))

#define TIMEOUT_VALUE_US 1000
#define DELAY_US         2

/* Special EIC line value to indicate NMI routing */
#define EIC_LINE_NMI 0xFE

/* NMI pin configuration from devicetree */
#define NMI_PINS_PROP nmi_pins
#if DT_INST_NODE_HAS_PROP(0, NMI_PINS_PROP)
#define NMI_PINS_COUNT (DT_INST_PROP_LEN(0, NMI_PINS_PROP) / 2)
static const uint8_t nmi_pins_array[] = DT_INST_PROP(0, NMI_PINS_PROP);
#define NMI_PIN_PORT(idx) nmi_pins_array[(idx) * 2]
#define NMI_PIN_NUM(idx)  nmi_pins_array[(idx) * 2 + 1]
#else
#define NMI_PINS_COUNT 0
#endif

struct mchp_eic_clock {
	const struct device *clock_dev;
	clock_control_subsys_t mclk_sys;
	clock_control_subsys_t gclk_sys;
};

/* Structure for assigning a pin and port to an EIC line */
struct mchp_eic_line_assignment {
	uint8_t pin: 5;
	uint8_t port: 3;
};

struct eic_mchp_dev_cfg {
	eic_registers_t *regs;
	struct mchp_eic_clock eic_clock;
	void (*irq_config)(void);
	/* Enable the low power mode to use the ULP32K clock for EIC */
	bool low_power_mode;
};

/* NMI configuration state */
struct mchp_eic_nmi_config {
	uint8_t port_id;
	uint8_t pin_num;
	bool configured;
};

/* Device data structure for containing housekeeping structures */
struct eic_mchp_dev_data {
	/* This is used for checking whether a line is in busy or not.
	 * Each bit correspond to an eic line
	 */
	uint16_t line_busy;
	/* It contains the address of gpio isr function */
	mchp_eic_callback_t eic_line_callback;
	/* This will contain the address of data structure of gpio peripheral. It is used
	 * for callback related functions.
	 */
	void *gpio_data[PORT_GROUP_NUMBER];
	/* Each bit of this uin16_t denotes an eic line. There are MCHP_PORT_ID_MAX number of such
	 * variables. Whenever a eic_line is assigned to a particular port, it is to be updated in
	 * this. This is later used in checking the pending irq
	 */
	struct mchp_eic_line_assignment lines[EIC_LINE_MAX];
	/*Each line will have its own structure
	 *for keeping its data
	 */
	uint16_t port_assigned_line[MCHP_PORT_ID_MAX];
	uint32_t lock;
	/* NMI configuration */
	struct mchp_eic_nmi_config nmi;
};

/**
 * Check if a given port and pin is connected to EIC NMI
 *
 * Returns true if the pin is an NMI pin, false otherwise
 */
static bool is_nmi_pin(int port, int pin)
{
#if NMI_PINS_COUNT > 0
	port = GET_PORT_IDX(port);
	for (int i = 0; i < NMI_PINS_COUNT; i++) {
		if (NMI_PIN_PORT(i) == port && NMI_PIN_NUM(i) == pin) {
			return true;
		}
	}
#else
	ARG_UNUSED(port);
	ARG_UNUSED(pin);
#endif
	return false;
}

/**
 * Find the EIC (External Interrupt Controller) line corresponding to a given port and pin.
 *
 * This function determines the EIC line number for a specified port and pin combination.
 * It takes into account unsupported and special pins for each port, and applies the necessary
 * offsets or marks the line as free if the pin is not supported.
 *
 * It returns the EIC line number corresponding to the given port and pin.
 *         If the pin is unsupported, returns INTC_LINE_FREE.
 *         If the pin is an NMI pin, returns EIC_LINE_NMI.
 */

static uint8_t find_eic_line_from_pin(int port, int pin)
{
	uint8_t eic_line = pin % EIC_LINES_PER_PORT;
	uint32_t pin_mask = BIT(pin);

	/* Check if this is an NMI pin first */
	if (is_nmi_pin(port, pin)) {
		return EIC_LINE_NMI;
	}

	/* Convert port ID to port index for secure GPIO ports */
	port = GET_PORT_IDX(port);

	switch (port) {
	case MCHP_PORT_ID0:
		if ((PORTA_UNSUPPORTED_PINS & pin_mask) != 0) {
			eic_line = INTC_LINE_FREE;
		} else if ((PORTA_SPECIAL_PINS_1 & pin_mask) != 0) {
			eic_line += PORTA_SPECIAL_PINS_1_OFFSET;
		} else {
			/* Nothing to be done */
		}
		break;
	case MCHP_PORT_ID1:
		if ((PORTB_UNSUPPORTED_PINS & pin_mask) != 0) {
			eic_line = INTC_LINE_FREE;
		} else if ((PORTB_SPECIAL_PINS & pin_mask) != 0) {
			eic_line += PORTB_SPECIAL_PINS_OFFSET;
		}
		break;
	case MCHP_PORT_ID2:
		if ((PORTC_SUPPORTED_PINS & pin_mask) == 0) {
			eic_line = INTC_LINE_FREE;
		} else if ((PORTC_UNSUPPORTED_PINS & pin_mask) != 0) {
			eic_line = INTC_LINE_FREE;
		} else if ((PORTC_SPECIAL_PINS_1 & pin_mask) != 0) {
			eic_line += PORTC_SPECIAL_PINS_1_OFFSET;
		} else if ((PORTC_SPECIAL_PINS_2 & pin_mask) != 0) {
			eic_line -= PORTC_SPECIAL_PINS_2_OFFSET;

		} else {
			/* Nothing to be done */
		}
		break;
	case MCHP_PORT_ID3:
		if ((PORTD_SUPPORTED_PINS & pin_mask) == 0) {
			eic_line = INTC_LINE_FREE;
		} else if ((PORTD_SPECIAL_PINS_2 & pin_mask) != 0) {
			eic_line += PORTD_SPECIAL_PINS_2_OFFSET;
		} else if ((PORTD_SPECIAL_PINS_1 & pin_mask) != 0) {
			eic_line -= PORTD_SPECIAL_PINS_1_OFFSET;
		} else {
			/*Nothing to be done*/
		}
		break;
	default:
		LOG_ERR("Unsupported port id provided");
		break;
	}

	return eic_line;
}

static inline void eic_sync_wait(eic_registers_t *eic_reg)
{
	if (WAIT_FOR((eic_reg->EIC_SYNCBUSY == 0), TIMEOUT_VALUE_US, k_busy_wait(DELAY_US)) ==
	    false) {
		LOG_ERR("Timeout waiting for EIC_SYNCBUSY bits to clear");
	}
}

static inline void eic_enable(eic_registers_t *regs)
{
	regs->EIC_CTRLA |= EIC_CTRLA_ENABLE_Msk;
}

static inline void eic_disable(eic_registers_t *regs)
{
	regs->EIC_CTRLA &= ~EIC_CTRLA_ENABLE_Msk;
}

static void enable_interrupt_line(eic_registers_t *regs, uint8_t eic_line)
{
	uint16_t pin_mask = BIT(eic_line);

	regs->EIC_INTFLAG = pin_mask;
	regs->EIC_INTENSET |= pin_mask;
}

/* The bit position for respective eic line in the config
 * register is calculated and written into the respective config
 * register
 */
static inline void eic_config_set_trig(eic_registers_t *regs, uint8_t eic_line, uint32_t trig_type)
{
	const uint32_t shift = EIC_TRIG_TYPE_BIT_POS(eic_line);
	const uint32_t mask = (uint32_t)EIC_CONFIG_EIC_LINE_MSK << shift;
	const uint32_t val = (trig_type & EIC_CONFIG_EIC_LINE_MSK) << shift;

#if defined(CONFIG_SOC_FAMILY_MICROCHIP_PIC32CX_SG) ||                                             \
	defined(CONFIG_SOC_FAMILY_MICROCHIP_PIC32CK_SG_GC) ||                                      \
	defined(CONFIG_SOC_FAMILY_MICROCHIP_PIC32CM_JH)
	/* EIC_CONFIG0 / EIC_CONFIG1 as separate fields */
	if (EIC_CONFIG_REG_IDX(eic_line) == 0) {
		regs->EIC_CONFIG0 = (regs->EIC_CONFIG0 & ~mask) | val;
	} else {
		regs->EIC_CONFIG1 = (regs->EIC_CONFIG1 & ~mask) | val;
	}
#else
	/* EIC_CONFIG[] as an array */
	const uint8_t idx = EIC_CONFIG_REG_IDX(eic_line);

	regs->EIC_CONFIG[idx] = (regs->EIC_CONFIG[idx] & ~mask) | val;
#endif
}

static void disable_interrupt_line(eic_registers_t *regs, uint8_t eic_line)
{
	regs->EIC_INTENCLR = BIT(eic_line);
}

#ifdef CONFIG_INTC_MCHP_EIC_G1_NMI
/**
 * Convert GPIO trigger type to NMI sense configuration
 */
static uint8_t get_nmi_sense(enum mchp_eic_trigger trig_type)
{
	switch (trig_type) {
	case MCHP_EIC_RISING:
		return EIC_NMICTRL_NMISENSE_RISE_Val;
	case MCHP_EIC_FALLING:
		return EIC_NMICTRL_NMISENSE_FALL_Val;
	case MCHP_EIC_BOTH:
		return EIC_NMICTRL_NMISENSE_BOTH_Val;
	case MCHP_EIC_HIGH:
		return EIC_NMICTRL_NMISENSE_HIGH_Val;
	case MCHP_EIC_LOW:
		return EIC_NMICTRL_NMISENSE_LOW_Val;
	default:
		return EIC_NMICTRL_NMISENSE_NONE_Val;
	}
}

/**
 * NMI handler for EIC NMI interrupts
 * This is registered via z_arm_nmi_set_handler() when CONFIG_RUNTIME_NMI is enabled
 */
static struct eic_mchp_dev_data *nmi_eic_data;
static eic_registers_t *nmi_eic_regs;
extern void z_arm_nmi_set_handler(void (*pHandler)(void));

static void eic_nmi_handler(void)
{
	/* Minimal handler - just clear flag and call callback */
	if (nmi_eic_regs != NULL) {
		nmi_eic_regs->EIC_NMIFLAG = EIC_NMIFLAG_NMI_Msk;
	}

	if (nmi_eic_regs != NULL && nmi_eic_data != NULL && nmi_eic_data->nmi.configured &&
	    nmi_eic_data->eic_line_callback != NULL) {
		uint8_t port_idx = GET_PORT_IDX(nmi_eic_data->nmi.port_id);
		uint32_t pins = BIT(nmi_eic_data->nmi.pin_num);

		nmi_eic_data->eic_line_callback(pins, nmi_eic_data->gpio_data[port_idx]);
	}
}
#endif /* CONFIG_INTC_MCHP_EIC_G1_NMI */

int eic_mchp_disable_interrupt(struct eic_config_params *eic_pin_config)
{
	const struct device *const dev = DEVICE_DT_INST_GET(0);
	const struct eic_mchp_dev_cfg *eic_cfg = dev->config;
	struct eic_mchp_dev_data *eic_data = dev->data;

	LOG_DBG("port = %p pint = %d", eic_pin_config->port_addr, eic_pin_config->pin_num);
	/*Check whether the pin was assigned to an eic line.*/
	uint8_t eic_line = find_eic_line_from_pin(eic_pin_config->port_id, eic_pin_config->pin_num);

	/* validate the line before doing anything with it */
	if (eic_line == INTC_LINE_FREE) {
		LOG_ERR("No EIC line associated with port %d pin %d", eic_pin_config->port_id,
			eic_pin_config->pin_num);
		return -ENOTSUP;
	}

	/* Handle NMI pin disable */
	if (eic_line == EIC_LINE_NMI) {
		eic_disable(eic_cfg->regs);
		eic_sync_wait(eic_cfg->regs);

		/* Disable NMI sensing */
		eic_cfg->regs->EIC_NMICTRL = EIC_NMICTRL_NMISENSE(EIC_NMICTRL_NMISENSE_NONE_Val);

		/* Clear any pending NMI flag */
		eic_cfg->regs->EIC_NMIFLAG = EIC_NMIFLAG_NMI_Msk;

		/* Disconnect pin from EIC peripheral */
		eic_pin_config->port_addr->PORT_PINCFG[eic_pin_config->pin_num] &=
			(uint8_t)(~PORT_PINCFG_PMUXEN(1));

		eic_enable(eic_cfg->regs);
		eic_sync_wait(eic_cfg->regs);

		/* Clear NMI configuration */
		eic_data->nmi.configured = false;
		return 0;
	}

	/* If the line was never configured, just return success silently */
	if ((eic_data->line_busy & BIT(eic_line)) == 0) {
		LOG_DBG("EIC Line %d is already free", eic_line);
		return 0;
	}

	/* disable the EIC peripheral before touching config registers */
	eic_disable(eic_cfg->regs);
	eic_sync_wait(eic_cfg->regs);

	/* Mask the interrupt and clear any pending flag */
	disable_interrupt_line(eic_cfg->regs, eic_line);
	eic_cfg->regs->EIC_INTFLAG = BIT(eic_line);

	/* clear the trigger-type bits for this line in EIC_CONFIG */
	eic_config_set_trig(eic_cfg->regs, eic_line, 0);

	/* clear debounce for this line */
	eic_cfg->regs->EIC_DEBOUNCEN &= ~BIT(eic_line);

	/* Disconnect pin from EIC peripheral */
	eic_pin_config->port_addr->PORT_PINCFG[eic_pin_config->pin_num] &=
		(uint8_t)(~PORT_PINCFG_PMUXEN(1));

	/* Re-enable the peripheral*/
	eic_enable(eic_cfg->regs);
	eic_sync_wait(eic_cfg->regs);

	/* House-keeping: free the line */
	eic_data->lock = irq_lock();
	eic_data->line_busy &= ~BIT(eic_line);
	eic_data->lines[eic_line].pin = INTC_PIN_DEFAULT_VAL;
	eic_data->lines[eic_line].port = INTC_PORT_DEFAULT_VAL;
	/* Convert port ID to port index for secure GPIO ports */
	eic_data->port_assigned_line[GET_PORT_IDX(eic_pin_config->port_id)] &= ~BIT(eic_line);

	irq_unlock(eic_data->lock);
	return 0;
}

uint32_t eic_mchp_interrupt_pending(uint8_t port_id)
{
	const struct device *const dev = DEVICE_DT_INST_GET(0);
	const struct eic_mchp_dev_cfg *eic_cfg = dev->config;
	struct eic_mchp_dev_data *eic_data = dev->data;
	uint8_t pin = 0;
	uint8_t eic_line = 0;
	/* Contains the bit mask where each set bit corresponds to a pending interrupt */
	uint32_t ret_val = 0;
	/* Convert port ID to port index for secure GPIO ports */
	port_id = GET_PORT_IDX(port_id);
	if (port_id >= MCHP_PORT_ID_MAX) {
		LOG_ERR("Invalid port id passed");
		return ret_val;
	}
	eic_data->lock = irq_lock();

	/* Check NMI pending for this port */
	if (eic_data->nmi.configured && GET_PORT_IDX(eic_data->nmi.port_id) == port_id &&
	    (eic_cfg->regs->EIC_NMIFLAG & EIC_NMIFLAG_NMI_Msk)) {
		ret_val |= BIT(eic_data->nmi.pin_num);
	}
	uint16_t port_flagged_lines = eic_data->port_assigned_line[port_id];

	/* The port_flagged_lines contains the data of eic lines which are used for a particular
	 * port. ANDing with it gets the interrupt pending flags which are relevant to the given
	 * port id
	 */
	port_flagged_lines = eic_cfg->regs->EIC_INTFLAG & port_flagged_lines;

	/* Get the trailing zeroes to find the eic_line number.
	 * Use the eic_line to get the pin number
	 * then use the BIT(n) function to create a mask with each bit corresponding to a
	 * pin of the port and return it to the user
	 */
	while (port_flagged_lines != 0) {
		eic_line = __builtin_ctz(port_flagged_lines);
		port_flagged_lines &= (port_flagged_lines - 1);
		pin = eic_data->lines[eic_line].pin;
		ret_val |= BIT(pin);
	}
	irq_unlock(eic_data->lock);

	return ret_val;
}

/**
 * This function configures the EIC interrupt for the specified pin, including setting up
 * the trigger type, enabling input, configuring debounce if required,
 * and updating the internal data structures to reflect the new assignment of pin to an eic line.
 */
int eic_mchp_config_interrupt(struct eic_config_params *eic_pin_config)
{
	const struct device *const dev = DEVICE_DT_INST_GET(0);
	const struct eic_mchp_dev_cfg *eic_cfg = dev->config;
	struct eic_mchp_dev_data *eic_data = dev->data;
	uint8_t pin = eic_pin_config->pin_num;
	uint8_t pmux_offset = pin / 2;
	int eic_line = INTC_LINE_FREE;

	/* Convert port ID to port index for secure GPIO ports */
	uint8_t port_idx = GET_PORT_IDX(eic_pin_config->port_id);

	eic_data->gpio_data[port_idx] = eic_pin_config->gpio_data;
	eic_data->eic_line_callback = eic_pin_config->eic_line_callback;

	/* Find the eic line of a given pin of the given port. If no eic line associated or
	 * eic line is busy, return failure
	 */
	eic_line = find_eic_line_from_pin(eic_pin_config->port_id, pin);
	LOG_DBG("eic line of port %d pin %d = %d", port_idx, pin, eic_line);

	if (eic_line == INTC_LINE_FREE) {
		LOG_ERR("no associated eic line found");
		return -ENOTSUP;
	}

	/* Handle NMI pin configuration separately */
	if (eic_line == EIC_LINE_NMI) {
#ifndef CONFIG_INTC_MCHP_EIC_G1_NMI
		LOG_ERR("NMI pin requires CONFIG_INTC_MCHP_EIC_G1_NMI to be enabled");
		return -ENOTSUP;
#else
		LOG_DBG("Configuring NMI for port %d pin %d", eic_pin_config->port_id, pin);

		/* CRITICAL: Register handler and store data FIRST before any HW config */
		eic_data->nmi.port_id = eic_pin_config->port_id;
		eic_data->nmi.pin_num = pin;
		eic_data->nmi.configured = true;
		eic_data->gpio_data[port_idx] = eic_pin_config->gpio_data;
		eic_data->eic_line_callback = eic_pin_config->eic_line_callback;
		nmi_eic_data = eic_data;
		nmi_eic_regs = eic_cfg->regs;
		z_arm_nmi_set_handler(eic_nmi_handler);

		eic_disable(eic_cfg->regs);
		eic_sync_wait(eic_cfg->regs);

		/* Disable NMI sensing first */
		eic_cfg->regs->EIC_NMICTRL = EIC_NMICTRL_NMISENSE(EIC_NMICTRL_NMISENSE_NONE_Val);

		/* Clear any pending NMI flag */
		eic_cfg->regs->EIC_NMIFLAG = EIC_NMIFLAG_NMI_Msk;

#if defined(CONFIG_ARM_TRUSTZONE_M)
		/* On TrustZone devices, ensure NMI is configured as secure */
		eic_cfg->regs->EIC_NONSEC &= ~EIC_NSCHK_NMI_Msk;
		eic_cfg->regs->EIC_NSCHK &= ~EIC_NSCHK_NMI_Msk;
#endif

		/* Configure the pin as input and connect it to EIC peripheral (function A) */
		eic_pin_config->port_addr->PORT_PINCFG[pin] |=
			PORT_PINCFG_PMUXEN(1) | PORT_PINCFG_INEN(1);
		eic_pin_config->port_addr->PORT_PMUX[pmux_offset] &=
			((pin & 1) == 0) ? (~PORT_PMUX_PMUXE_Msk) : (~PORT_PMUX_PMUXO_Msk);

		eic_enable(eic_cfg->regs);
		eic_sync_wait(eic_cfg->regs);

		/* Now enable NMI sensing - handler is already registered */
		uint8_t nmi_sense = get_nmi_sense(eic_pin_config->trig_type);

		eic_cfg->regs->EIC_NMICTRL =
			EIC_NMICTRL_NMISENSE(nmi_sense) | EIC_NMICTRL_NMIFILTEN(1);

		LOG_DBG("NMI configured: NMICTRL=0x%02x sense=%d", eic_cfg->regs->EIC_NMICTRL,
			nmi_sense);
		return 0;
#endif /* CONFIG_INTC_MCHP_EIC_G1_NMI */
	}

	eic_data->lock = irq_lock();
	if ((eic_data->line_busy & BIT(eic_line)) != 0) {
		/* The line is already in use. Allow it ONLY if this is the same
		 * (port, pin) that already owns it — i.e. a legitimate reconfigure
		 * (e.g. switching from EDGE_RISING to EDGE_FALLING).
		 */
		if ((eic_data->lines[eic_line].pin != pin) ||
		    (eic_data->lines[eic_line].port != port_idx)) {
			irq_unlock(eic_data->lock);
			LOG_ERR("EIC Line %d already owned by port %d pin %d "
				"(requested port %d pin %d)",
				eic_line, eic_data->lines[eic_line].port,
				eic_data->lines[eic_line].pin, port_idx, pin);
			return -EBUSY;
		}
		LOG_DBG("Reconfiguring EIC line %d (port %d pin %d)", eic_line, port_idx, pin);
	}
	eic_disable(eic_cfg->regs);
	eic_sync_wait(eic_cfg->regs);

	/* Configure the pin as input and connect it to eic peripheral */
	eic_pin_config->port_addr->PORT_PINCFG[pin] |= PORT_PINCFG_PMUXEN(1) | PORT_PINCFG_INEN(1);
	eic_pin_config->port_addr->PORT_PMUX[pmux_offset] &=
		((pin & 1) == 0) ? (~PORT_PMUX_PMUXE_Msk) : (~PORT_PMUX_PMUXO_Msk);

	/* Set the trigger type for this EIC line in the EIC_CONFIG register */
	eic_config_set_trig(eic_cfg->regs, eic_line, eic_pin_config->trig_type);

	/* Set the debouncing feature of the eic line if required */
	if (eic_pin_config->debounce != 0) {
		eic_cfg->regs->EIC_DEBOUNCEN |= BIT(eic_line);
	}
	LOG_DBG("%s", eic_pin_config->debounce ? "debouncing enabled" : "debouncing disabled");

	enable_interrupt_line(eic_cfg->regs, eic_line);

	eic_enable(eic_cfg->regs);
	eic_sync_wait(eic_cfg->regs);

	/*House keeping */
	eic_data->line_busy |= BIT(eic_line);
	eic_data->lines[eic_line].pin = pin;
	eic_data->lines[eic_line].port = port_idx;
	eic_data->port_assigned_line[port_idx] |= BIT(eic_line);

	irq_unlock(eic_data->lock);
	return 0;
}

static int eic_mchp_init(const struct device *dev)
{
	const struct eic_mchp_dev_cfg *eic_cfg = dev->config;
	int ret_val;

	ret_val = clock_control_on(eic_cfg->eic_clock.clock_dev, eic_cfg->eic_clock.mclk_sys);
	if ((ret_val < 0) && (ret_val != -EALREADY)) {
		LOG_ERR("Clock control on failed for mclk %d", ret_val);
		return ret_val;
	}
	ret_val = clock_control_on(eic_cfg->eic_clock.clock_dev, eic_cfg->eic_clock.gclk_sys);
	if ((ret_val < 0) && (ret_val != -EALREADY)) {
		LOG_ERR("Clock control on failed for gclk %d", ret_val);
		return ret_val;
	}
	eic_cfg->irq_config();

	eic_cfg->regs->EIC_CTRLA = EIC_CTRLA_SWRST(EIC_CTRLA_SWRST_Msk);
	eic_sync_wait(eic_cfg->regs);

	if (eic_cfg->low_power_mode == true) {
		eic_cfg->regs->EIC_CTRLA = EIC_CTRLA_CKSEL(EIC_CTRLA_CKSEL_CLK_ULP32K);
	}
	eic_enable(eic_cfg->regs);
	eic_sync_wait(eic_cfg->regs);
	LOG_DBG("EIC initialisation done 0x%p", eic_cfg->regs);

	return 0;
}

#define EIC_MCHP_DATA_DEFN(n)        static struct eic_mchp_dev_data eic_mchp_data_##n
#define EIC_MCHP_IRQ_HANDLER_DECL(n) static void eic_irq_connect_##n(void)

#ifdef CONFIG_SOC_FAMILY_MICROCHIP_PIC32CM_JH
/* A single ISR is kept because in this family we only have a single IRQ line for all the
 * interrupts
 */
#define EIC_MCHP_CREATE_HANDLERS(n)
#define EIC_NIBS_BIT_COUNT 4
#define EIC_INT_NIBS       4
#define EIC_NIBS_MASK      0xf

static void eic_mchp_isr(const struct device *dev)
{
	const struct eic_mchp_dev_cfg *eic_cfg = dev->config;
	struct eic_mchp_dev_data *eic_data = dev->data;
	uint16_t eic_int_flag = eic_cfg->regs->EIC_INTFLAG;
	uint8_t eic_line = 0;

	eic_cfg->regs->EIC_INTFLAG = eic_int_flag;
	/* Divides the interrupt flag into nibbles and
	 *checks whether it has a set bit inside it
	 */
	for (int nibs = 0; nibs < EIC_INT_NIBS; nibs++) {
		if ((eic_int_flag & (EIC_NIBS_MASK << eic_line)) == 0) {
			eic_line += EIC_NIBS_BIT_COUNT;
			continue;
		}
		/* find the set eic line from within the nibble
		 * and call the callback as expected
		 */
		for (int i = 0; i < EIC_NIBS_BIT_COUNT; i++) {
			if ((eic_int_flag & BIT(eic_line)) == 0) {
				eic_line++;
				continue;
			}
			uint8_t port_id = eic_data->lines[eic_line].port;

			if (eic_data->eic_line_callback != NULL) {
				uint32_t pins = BIT(eic_data->lines[eic_line].pin);

				eic_data->eic_line_callback(pins, eic_data->gpio_data[port_id]);
			}
			eic_int_flag &= ~BIT(eic_line);
			eic_line++;
		}
	}
}

/* clang-format off */
#define EIC_MCHP_IRQ_CONNECT(eic_line, inst)                                                    \
	IF_ENABLED(DT_INST_IRQ_HAS_IDX(inst, eic_line), (					\
	do {											\
		IRQ_CONNECT(DT_INST_IRQ_BY_IDX(inst, eic_line, irq),				\
				DT_INST_IRQ_BY_IDX(inst, eic_line, priority), eic_mchp_isr,	\
				DEVICE_DT_INST_GET(inst), inst);				\
		irq_enable(DT_INST_IRQ_BY_IDX(inst, eic_line, irq));				\
	} while (false);									\
			))
#else

#define EIC_MCHP_CREATE_HANDLERS(n)  LISTIFY(DT_NUM_IRQS(DT_DRV_INST(n)), EIC_MCHP_CB_INIT, (;),)
#define EIC_MCHP_CB_INIT(eic_line, _)							\
	static void eic_mchp_isr_##eic_line(const struct device *dev)			\
	{										\
		const struct eic_mchp_dev_cfg *eic_cfg = dev->config;			\
		struct eic_mchp_dev_data *eic_data = dev->data;				\
		uint8_t port_id = eic_data->lines[EIC_LINE_##eic_line].port ;		\
											\
		eic_cfg->regs->EIC_INTFLAG = BIT(EIC_LINE_##eic_line);			\
		if (eic_data->eic_line_callback != NULL) {				\
			uint32_t pins = BIT(eic_data->lines[EIC_LINE_##eic_line].pin);	\
			eic_data->eic_line_callback(pins, eic_data->gpio_data[port_id]);\
		}									\
	}

#define EIC_MCHP_IRQ_CONNECT(eic_line, inst)							\
	IF_ENABLED(DT_INST_IRQ_HAS_IDX(inst, eic_line), (					\
	do {											\
		IRQ_CONNECT(DT_INST_IRQ_BY_IDX(inst, eic_line, irq),				\
			DT_INST_IRQ_BY_IDX(inst, eic_line, priority), eic_mchp_isr_##eic_line,	\
			DEVICE_DT_INST_GET(inst), inst);					\
		irq_enable(DT_INST_IRQ_BY_IDX(inst, eic_line, irq));				\
	} while (false);									\
			))
#endif /* CONFIG_SOC_FAMILY_MICROCHIP_PIC32CM_JH */

#define EIC_MCHP_CLOCK_DEFN(n)                                                                  \
	.eic_clock.clock_dev = DEVICE_DT_GET(DT_NODELABEL(clock)),                              \
	.eic_clock.mclk_sys = (void *)DT_INST_CLOCKS_CELL_BY_NAME(n, mclk, subsystem),          \
	.eic_clock.gclk_sys = (void *)DT_INST_CLOCKS_CELL_BY_NAME(n, gclk, subsystem)

#define EIC_MCHP_CFG_DEFN(n)						\
	static const struct eic_mchp_dev_cfg eic_mchp_dev_cfg_##n = {	\
		.regs = (eic_registers_t *)DT_INST_REG_ADDR(n),		\
		EIC_MCHP_CLOCK_DEFN(n),                                 \
		.irq_config = eic_irq_connect_##n,                      \
		.low_power_mode = DT_INST_PROP(n, low_power_mode)}


#define EIC_MCHP_IRQ_HANDLER(n)					\
	static void eic_irq_connect_##n(void)			\
	{                                                       \
		/** Connect all IRQs for this instance */	\
		LISTIFY(					\
			DT_NUM_IRQS(DT_DRV_INST(n)),		\
			EIC_MCHP_IRQ_CONNECT,			\
			(),					\
			n					\
		)                                               \
	}

#define EIC_MCHP_DEVICE_INIT(n)                                                                    \
	EIC_MCHP_CREATE_HANDLERS(n);                                                               \
	EIC_MCHP_IRQ_HANDLER_DECL(n);                                                              \
	EIC_MCHP_DATA_DEFN(n);                                                                     \
	EIC_MCHP_CFG_DEFN(n);                                                                      \
	DEVICE_DT_INST_DEFINE(n, eic_mchp_init, NULL, &eic_mchp_data_##n, &eic_mchp_dev_cfg_##n,   \
			      PRE_KERNEL_1, CONFIG_INTC_INIT_PRIORITY, NULL);                      \
	EIC_MCHP_IRQ_HANDLER(n)
/* clang-format on */

DT_INST_FOREACH_STATUS_OKAY(EIC_MCHP_DEVICE_INIT)
