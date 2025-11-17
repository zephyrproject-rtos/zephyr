/*
 * Copyright (c) 2025 LiteX Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Driver for LiteX CLIC (Core Local Interrupt Controller)
 * 
 * This driver implements support for the LiteX CLIC implementation
 * with CSR-based register access and workarounds for the pending bit
 * auto-clear issue.
 * 
 * Architecture notes:
 * - Follows RISC-V CLIC interrupt model with vectored interrupts
 * - Integrates with Zephyr's interrupt controller framework
 * - Provides both arch_irq_* and device-specific APIs
 * - Handles CSRStorage limitation through software workarounds
 */

#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/arch/riscv/csr.h>
#include <zephyr/arch/riscv/irq.h>
#include <zephyr/device.h>
#include <zephyr/drivers/interrupt_controller/riscv_clic.h>
#include <zephyr/irq.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
#include <zephyr/sw_isr_table.h>
#include "intc_litex_clic.h"

#define DT_DRV_COMPAT litex_clic

LOG_MODULE_REGISTER(intc_litex_clic, CONFIG_INTERRUPT_LOG_LEVEL);

struct litex_clic_data {
	uint32_t num_interrupts;
	uint8_t priority_levels;
	/* Workaround: Track pending bits to prevent infinite loops */
	uint32_t pending_mask;
	uint32_t handling_mask;  /* Interrupts currently being handled */
};

struct litex_clic_config {
	mem_addr_t base;
	uint32_t num_interrupts;
	uint8_t priority_bits;
	void (*config_func)(void);  /* Optional configuration callback */
};

/* Helper functions for CSR access */
static inline void litex_clic_write32(const struct device *dev, uint32_t offset, uint32_t value)
{
	const struct litex_clic_config *config = dev->config;
	mem_addr_t addr = config->base + offset;
	sys_write32(value, addr);
}

static inline uint32_t litex_clic_read32(const struct device *dev, uint32_t offset)
{
	const struct litex_clic_config *config = dev->config;
	mem_addr_t addr = config->base + offset;
	return sys_read32(addr);
}

static inline void litex_clic_write8(const struct device *dev, uint32_t offset, uint8_t value)
{
	const struct litex_clic_config *config = dev->config;
	mem_addr_t addr = config->base + offset;
	sys_write8(value, addr);
}

static inline uint8_t litex_clic_read8(const struct device *dev, uint32_t offset)
{
	const struct litex_clic_config *config = dev->config;
	mem_addr_t addr = config->base + offset;
	return sys_read8(addr);
}

/* Set interrupt pending bit */
static void litex_clic_set_pending(const struct device *dev, uint32_t irq)
{
	struct litex_clic_data *data = dev->data;
	
	if (irq >= data->num_interrupts) {
		LOG_ERR("Invalid IRQ %u (max %u)", irq, data->num_interrupts - 1);
		return;
	}
	
	/* Only first 16 interrupts have CSR control in LiteX CLIC */
	if (irq < LITEX_CLIC_CSR_INTERRUPTS) {
		uint32_t offset = LITEX_CLIC_CLICINTIP_BASE + (irq * 4);
		litex_clic_write32(dev, offset, 1);
		data->pending_mask |= BIT(irq);
		LOG_DBG("Set pending for IRQ %u", irq);
	}
}

/* Clear interrupt pending bit - WORKAROUND for auto-clear issue */
static void litex_clic_clear_pending(const struct device *dev, uint32_t irq)
{
	struct litex_clic_data *data = dev->data;
	
	if (irq >= data->num_interrupts) {
		LOG_ERR("Invalid IRQ %u (max %u)", irq, data->num_interrupts - 1);
		return;
	}
	
	/* Only first 16 interrupts have CSR control */
	if (irq < LITEX_CLIC_CSR_INTERRUPTS) {
		uint32_t offset = LITEX_CLIC_CLICINTIP_BASE + (irq * 4);
		
		/* WORKAROUND: Due to CSRStorage limitation, we need special handling */
		/* Disable interrupt temporarily to prevent re-triggering */
		uint32_t ie_offset = LITEX_CLIC_CLICINTIE_BASE + (irq * 4);
		uint32_t ie_state = litex_clic_read32(dev, ie_offset);
		
		/* Disable interrupt */
		litex_clic_write32(dev, ie_offset, 0);
		
		/* Clear pending bit */
		litex_clic_write32(dev, offset, 0);
		data->pending_mask &= ~BIT(irq);
		data->handling_mask &= ~BIT(irq);
		
		/* Re-enable interrupt if it was enabled */
		if (ie_state) {
			litex_clic_write32(dev, ie_offset, 1);
		}
		
		LOG_DBG("Cleared pending for IRQ %u", irq);
	}
}

/* Check if interrupt is pending */
static bool litex_clic_is_pending(const struct device *dev, uint32_t irq)
{
	const struct litex_clic_data *data = dev->data;
	
	if (irq >= data->num_interrupts) {
		return false;
	}
	
	if (irq < LITEX_CLIC_CSR_INTERRUPTS) {
		uint32_t offset = LITEX_CLIC_CLICINTIP_BASE + (irq * 4);
		return litex_clic_read32(dev, offset) != 0;
	}
	
	/* For hardware interrupts, check debug registers */
	uint32_t offset = LITEX_CLIC_DEBUG_IP_HW + (irq * 4);
	return litex_clic_read32(dev, offset) != 0;
}

/* Enable interrupt */
void litex_clic_irq_enable(const struct device *dev, uint32_t irq)
{
	struct litex_clic_data *data = dev->data;
	
	if (irq >= data->num_interrupts) {
		LOG_ERR("Invalid IRQ %u (max %u)", irq, data->num_interrupts - 1);
		return;
	}
	
	uint32_t offset = LITEX_CLIC_CLICINTIE_BASE + (irq * 4);
	litex_clic_write32(dev, offset, 1);
	LOG_DBG("Enabled IRQ %u", irq);
}

/* Disable interrupt */
void litex_clic_irq_disable(const struct device *dev, uint32_t irq)
{
	struct litex_clic_data *data = dev->data;
	
	if (irq >= data->num_interrupts) {
		LOG_ERR("Invalid IRQ %u (max %u)", irq, data->num_interrupts - 1);
		return;
	}
	
	uint32_t offset = LITEX_CLIC_CLICINTIE_BASE + (irq * 4);
	litex_clic_write32(dev, offset, 0);
	LOG_DBG("Disabled IRQ %u", irq);
}

/* Check if interrupt is enabled */
int litex_clic_irq_is_enabled(const struct device *dev, uint32_t irq)
{
	const struct litex_clic_data *data = dev->data;
	
	if (irq >= data->num_interrupts) {
		return 0;
	}
	
	uint32_t offset = LITEX_CLIC_CLICINTIE_BASE + (irq * 4);
	return litex_clic_read32(dev, offset) != 0;
}

/* Set interrupt priority */
void litex_clic_set_priority(const struct device *dev, uint32_t irq, uint32_t priority)
{
	struct litex_clic_data *data = dev->data;
	
	if (irq >= data->num_interrupts) {
		LOG_ERR("Invalid IRQ %u (max %u)", irq, data->num_interrupts - 1);
		return;
	}
	
	/* Limit priority to configured bits */
	uint8_t max_priority = (1 << data->priority_levels) - 1;
	if (priority > max_priority) {
		priority = max_priority;
	}
	
	uint32_t offset = LITEX_CLIC_CLICIPRIO_BASE + (irq * 4);
	litex_clic_write32(dev, offset, priority);
	LOG_DBG("Set IRQ %u priority to %u", irq, priority);
}

/* Get interrupt priority */
uint32_t litex_clic_get_priority(const struct device *dev, uint32_t irq)
{
	const struct litex_clic_data *data = dev->data;
	
	if (irq >= data->num_interrupts) {
		return 0;
	}
	
	uint32_t offset = LITEX_CLIC_CLICIPRIO_BASE + (irq * 4);
	return litex_clic_read32(dev, offset);
}

/* Set interrupt trigger type */
void litex_clic_set_trigger(const struct device *dev, uint32_t irq, uint32_t trigger)
{
	struct litex_clic_data *data = dev->data;
	
	if (irq >= data->num_interrupts) {
		LOG_ERR("Invalid IRQ %u (max %u)", irq, data->num_interrupts - 1);
		return;
	}
	
	uint32_t offset = LITEX_CLIC_CLICINTATTR_BASE + (irq * 4);
	uint32_t attr = litex_clic_read32(dev, offset);
	
	/* Clear trigger bits and set new ones */
	attr &= ~CLIC_ATTR_TRIG_MASK;
	
	switch (trigger) {
	case IRQ_TYPE_LEVEL:
		attr |= CLIC_ATTR_TRIG_LEVEL;
		break;
	case IRQ_TYPE_EDGE:
		attr |= CLIC_ATTR_TRIG_EDGE_POS;
		break;
	default:
		LOG_WRN("Unsupported trigger type %u for IRQ %u", trigger, irq);
		return;
	}
	
	litex_clic_write32(dev, offset, attr);
	LOG_DBG("Set IRQ %u trigger to %u", irq, trigger);
}

/**
 * @brief Main interrupt handler for LiteX CLIC
 * 
 * This handler is called when external interrupts are signaled to the CPU.
 * It scans for pending interrupts and dispatches to the appropriate ISR
 * from the software ISR table.
 * 
 * Architecture notes:
 * - Follows Zephyr's ISR table integration pattern
 * - Implements workaround for CSRStorage pending bit issue
 * - Handles both edge and level triggered interrupts
 * - Prevents re-entrancy through handling mask
 */
static void litex_clic_irq_handler(const void *arg)
{
	const struct device *dev = arg;
	struct litex_clic_data *data = dev->data;
	const struct _isr_table_entry *ite;
	uint32_t highest_prio = 0;
	int32_t highest_irq = -1;
	
	/* 
	 * Find highest priority pending interrupt
	 * This mimics hardware priority arbitration in software
	 */
	for (uint32_t irq = 0; irq < data->num_interrupts; irq++) {
		if (litex_clic_is_pending(dev, irq) && 
		    litex_clic_irq_is_enabled(dev, irq) &&
		    !(data->handling_mask & BIT(irq))) {
			
			uint32_t prio = litex_clic_get_priority(dev, irq);
			if (prio > highest_prio || highest_irq == -1) {
				highest_prio = prio;
				highest_irq = irq;
			}
		}
	}
	
	/* Handle the highest priority interrupt */
	if (highest_irq >= 0) {
		uint32_t irq = (uint32_t)highest_irq;
		
		/* Mark as being handled to prevent re-entry */
		data->handling_mask |= BIT(irq);
		
		/* Get ISR from software ISR table */
		ite = &_sw_isr_table[irq];
		
		/* WORKAROUND: Handle pending bit clearing based on trigger type */
		uint32_t attr = litex_clic_read32(dev, LITEX_CLIC_INTATTR(irq));
		bool is_edge_triggered = (attr & LITEX_CLIC_ATTR_TRIG_MASK) != LITEX_CLIC_ATTR_TRIG_LEVEL;
		
		if (is_edge_triggered) {
			/* Edge triggered - clear before ISR to prevent re-trigger */
			litex_clic_clear_pending(dev, irq);
		}
		
		/* Call the ISR if registered */
		if (ite->isr) {
			LOG_DBG("Dispatching IRQ %u to ISR %p with arg %p", 
				irq, ite->isr, ite->arg);
			ite->isr(ite->arg);
		} else {
			LOG_WRN("No ISR registered for IRQ %u", irq);
		}
		
		/* For level triggered, clear after ISR (source should be cleared) */
		if (!is_edge_triggered) {
			litex_clic_clear_pending(dev, irq);
		}
		
		/* Clear handling flag */
		data->handling_mask &= ~BIT(irq);
		
		/* Check for more pending interrupts (nested handling) */
		if (IS_ENABLED(CONFIG_LITEX_CLIC_NESTED_INTERRUPTS)) {
			/* Recursively handle next interrupt */
			litex_clic_irq_handler(arg);
		}
	}
}

/* Architecture-specific interrupt enable/disable */
void arch_irq_enable(unsigned int irq)
{
	const struct device *dev = DEVICE_DT_INST_GET(0);
	litex_clic_irq_enable(dev, irq);
}

void arch_irq_disable(unsigned int irq)
{
	const struct device *dev = DEVICE_DT_INST_GET(0);
	litex_clic_irq_disable(dev, irq);
}

int arch_irq_is_enabled(unsigned int irq)
{
	const struct device *dev = DEVICE_DT_INST_GET(0);
	return litex_clic_irq_is_enabled(dev, irq);
}

/* Initialize the LiteX CLIC - follows Zephyr interrupt controller init pattern */
static int litex_clic_init(const struct device *dev)
{
	const struct litex_clic_config *config = dev->config;
	struct litex_clic_data *data = dev->data;
	
	LOG_INF("Initializing LiteX CLIC at 0x%08x with %u interrupts", 
		(uint32_t)config->base, config->num_interrupts);
	
	/* Step 1: Hardware Detection and Validation */
	data->num_interrupts = config->num_interrupts;
	data->priority_levels = config->priority_bits;
	data->pending_mask = 0;
	data->handling_mask = 0;
	
	/* Step 2: Register Setup - Initialize to known state */
	for (uint32_t i = 0; i < data->num_interrupts; i++) {
		/* Clear all pending bits first */
		if (LITEX_CLIC_IS_CSR_CONTROLLED(i)) {
			litex_clic_write32(dev, LITEX_CLIC_INTIP(i), 0);
		}
		/* Disable all interrupts */
		litex_clic_write32(dev, LITEX_CLIC_INTIE(i), 0);
		/* Set default priority */
		litex_clic_write32(dev, LITEX_CLIC_INTPRIO(i), 0);
		/* Set default attributes (level-triggered) */
		litex_clic_write32(dev, LITEX_CLIC_INTATTR(i), LITEX_CLIC_ATTR_TRIG_LEVEL);
	}
	
	/* Step 3: Routing Configuration - Not applicable for CLIC (direct to CPU) */
	
	/* Step 4: Priority Configuration - Already set to defaults above */
	
	/* Step 5: Enable Controller */
	/* For RISC-V CLIC, we need to enable machine external interrupt */
	csr_set(mie, MIP_MEIP);
	
	/* Connect the main interrupt handler for external interrupts */
	IRQ_CONNECT(RISCV_IRQ_MEXT, 0, litex_clic_irq_handler, 
		    DEVICE_DT_INST_GET(0), 0);
	
	/* Enable machine interrupts globally */
	csr_set(mstatus, MSTATUS_IEN);
	
	/* Optional: Run configuration callback if provided */
	if (config->config_func) {
		config->config_func();
	}
	
	LOG_INF("LiteX CLIC initialized successfully (CSR control for first %u interrupts)",
		LITEX_CLIC_CSR_INTERRUPTS);
	
	return 0;
}

/* Device tree configuration */
static const struct litex_clic_config litex_clic_config_0 = {
	.base = DT_INST_REG_ADDR(0),
	.num_interrupts = DT_INST_PROP_OR(0, num_interrupts, LITEX_CLIC_MAX_INTERRUPTS),
	.priority_bits = DT_INST_PROP_OR(0, priority_bits, 8),
	.config_func = NULL,  /* No additional configuration needed */
};

static struct litex_clic_data litex_clic_data_0;

DEVICE_DT_INST_DEFINE(0, litex_clic_init, NULL,
		      &litex_clic_data_0, &litex_clic_config_0,
		      PRE_KERNEL_1, CONFIG_INTC_INIT_PRIORITY, NULL);