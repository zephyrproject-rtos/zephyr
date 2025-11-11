/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Based on STM32 EXTI driver, which is (c) 2016 Open-RnD Sp. z o.o. */

#include <zephyr/device.h>
#include <zephyr/irq.h>
#include <errno.h>
#include <zephyr/drivers/interrupt_controller/nxp_pint.h>
#include <zephyr/pm/device.h>

#include <fsl_inputmux.h>
#include <soc.h>

#define DT_DRV_COMPAT nxp_pint

static PINT_Type *pint_base = (PINT_Type *)DT_INST_REG_ADDR(0);

/* Describes configuration of PINT IRQ slot */
struct pint_irq_slot {
	nxp_pint_cb_t callback;
	void *user_data;
	uint8_t pin: 6;
	uint8_t used: 1;
	uint8_t irq;
};

#define NO_PINT_ID 0xFF

/* Tracks IRQ configuration for each pint interrupt source */
static struct pint_irq_slot pint_irq_cfg[DT_INST_PROP(0, num_lines)];
/* Tracks pint interrupt source selected for each pin */
static uint8_t pin_pint_id[DT_INST_PROP(0, num_inputs)];

#define PIN_TO_INPUT_MUX_CONNECTION(pin) \
	((PINTSEL_PMUX_ID << PMUX_SHIFT) + (pin))

/* Attaches pin to PINT IRQ slot using INPUTMUX */
static void attach_pin_to_pint(uint8_t pin, uint8_t pint_slot)
{
	INPUTMUX_Init(INPUTMUX);

	/* Three parameters here- INPUTMUX base, the ID of the PINT slot,
	 * and a integer describing the GPIO pin.
	 */
	INPUTMUX_AttachSignal(INPUTMUX, pint_slot,
			      PIN_TO_INPUT_MUX_CONNECTION(pin));

	/* Disable INPUTMUX after making changes, this gates clock and
	 * saves power.
	 */
	INPUTMUX_Deinit(INPUTMUX);
}

/**
 * @brief Enable PINT interrupt source.
 *
 * @param pin: pin to use as interrupt source
 *     0-64, corresponding to GPIO0 pin 1 - GPIO1 pin 31)
 * @param trigger: one of nxp_pint_trigger flags
 * @param wake: indicates if the pin should wakeup the system
 * @return 0 on success, or negative value on error
 */
int nxp_pint_pin_enable(uint8_t pin, enum nxp_pint_trigger trigger, bool wake)
{
	uint8_t slot = 0U;

	if (pin >= ARRAY_SIZE(pin_pint_id)) {
		/* Invalid pin ID */
		return -EINVAL;
	}
	/* Find unused IRQ slot */
	if (pin_pint_id[pin] != NO_PINT_ID) {
		slot = pin_pint_id[pin];
	} else {
		for (slot = 0; slot < ARRAY_SIZE(pint_irq_cfg); slot++) {
			if (!pint_irq_cfg[slot].used) {
				break;
			}
		}
		if (slot == ARRAY_SIZE(pint_irq_cfg)) {
			/* No free IRQ slots */
			return -EBUSY;
		}
		pin_pint_id[pin] = slot;
	}
	pint_irq_cfg[slot].used = true;
	pint_irq_cfg[slot].pin = pin;
	/* Attach pin to interrupt slot using INPUTMUX */
	attach_pin_to_pint(pin, slot);
	/* Now configure the interrupt. No need to install callback, this
	 * driver handles the IRQ
	 */
	PINT_PinInterruptConfig(pint_base, slot, trigger, NULL);
	if (wake) {
		NXP_ENABLE_WAKEUP_SIGNAL(pint_irq_cfg[slot].irq);
	} else {
		NXP_DISABLE_WAKEUP_SIGNAL(pint_irq_cfg[slot].irq);
		irq_enable(pint_irq_cfg[slot].irq);
	}

	return 0;
}


/**
 * @brief disable PINT interrupt source.
 *
 * @param pin: pin interrupt source to disable
 */
void nxp_pint_pin_disable(uint8_t pin)
{
	uint8_t slot;

	if (pin > ARRAY_SIZE(pin_pint_id)) {
		return;
	}

	slot = pin_pint_id[pin];
	if (slot == NO_PINT_ID) {
		return;
	}
	/* Remove this pin from the PINT slot if one was in use */
	pint_irq_cfg[slot].used = false;
	PINT_PinInterruptConfig(pint_base, slot, kPINT_PinIntEnableNone, NULL);
}

/**
 * @brief Install PINT callback
 *
 * @param pin: interrupt source to install callback for
 * @param cb: callback to install
 * @param data: user data to include in callback
 * @return 0 on success, or negative value on error
 */
int nxp_pint_pin_set_callback(uint8_t pin, nxp_pint_cb_t cb, void *data)
{
	uint8_t slot;

	if (pin > ARRAY_SIZE(pin_pint_id)) {
		return -EINVAL;
	}

	slot = pin_pint_id[pin];
	if (slot == NO_PINT_ID) {
		return -EINVAL;
	}

	pint_irq_cfg[slot].callback = cb;
	pint_irq_cfg[slot].user_data = data;
	return 0;
}

/**
 * @brief Remove PINT callback
 *
 * @param pin: interrupt source to remove callback for
 */
void nxp_pint_pin_unset_callback(uint8_t pin)
{
	uint8_t slot;

	if (pin > ARRAY_SIZE(pin_pint_id)) {
		return;
	}

	slot = pin_pint_id[pin];
	if (slot == NO_PINT_ID) {
		return;
	}

	pint_irq_cfg[slot].callback = NULL;
}

/* NXP PINT ISR handler- called with PINT slot ID */
static void nxp_pint_isr(uint8_t *slot)
{
	PINT_PinInterruptClrStatus(pint_base, *slot);
	if (pint_irq_cfg[*slot].used && pint_irq_cfg[*slot].callback) {
		pint_irq_cfg[*slot].callback(pint_irq_cfg[*slot].pin,
					pint_irq_cfg[*slot].user_data);
	}
}

static int intc_nxp_pm_action(const struct device *dev, enum pm_device_action action)
{
	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		break;
	case PM_DEVICE_ACTION_TURN_OFF:
		break;
	case PM_DEVICE_ACTION_TURN_ON:
		PINT_Init(pint_base);
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

/* Defines PINT IRQ handler for a given irq index */
#define NXP_PINT_IRQ(idx, node_id)						\
	IF_ENABLED(DT_IRQ_HAS_IDX(node_id, idx),				\
	(static uint8_t nxp_pint_idx_##idx = idx;				\
	do {									\
		IRQ_CONNECT(DT_IRQ_BY_IDX(node_id, idx, irq),			\
			    DT_IRQ_BY_IDX(node_id, idx, priority),		\
			    nxp_pint_isr, &nxp_pint_idx_##idx, 0);		\
		irq_enable(DT_IRQ_BY_IDX(node_id, idx, irq));			\
		pint_irq_cfg[idx].irq = DT_IRQ_BY_IDX(node_id, idx, irq);	\
	} while (false)))

static int intc_nxp_pint_init(const struct device *dev)
{
	/* First, connect IRQs for each interrupt.
	 * The IRQ handler will receive the PINT slot as a
	 * parameter.
	 */
	LISTIFY(8, NXP_PINT_IRQ, (;), DT_INST(0, DT_DRV_COMPAT));
	memset(pin_pint_id, NO_PINT_ID, ARRAY_SIZE(pin_pint_id));

	return pm_device_driver_init(dev, intc_nxp_pm_action);
}

PM_DEVICE_DT_INST_DEFINE(0, intc_nxp_pm_action);

DEVICE_DT_INST_DEFINE(0, intc_nxp_pint_init, PM_DEVICE_DT_INST_GET(0), NULL, NULL,
		      PRE_KERNEL_1, CONFIG_INTC_INIT_PRIORITY, NULL);
