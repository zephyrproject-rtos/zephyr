/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Hardware port for the Bouffalo Lab BLE controller binary blob on BL70XL.
 * Overrides the weak implementations in the precompiled library.
 */

#include <zephyr/kernel.h>
#include <zephyr/irq.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/sys/sys_io.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>

#include <clic.h>
#include <bflb_soc.h>
#include <glb_reg.h>
#include <ef_data_reg.h>

/* Machine Software Interrupt — used by the BLE controller blob for deferred
 * context switch requests (FreeRTOS portYIELD equivalent).  Zephyr handles
 * rescheduling automatically, so we just clear the pending bit.
 */
#define MSOFT_IRQN 3

/* BLE IRQ handler from the controller blob (bare void(*)(void) function) */
static void (*ble_irq_handler_fn)(void);

static void ble_irq_bridge(const void *arg)
{
	ARG_UNUSED(arg);

	if (ble_irq_handler_fn != NULL) {
		ble_irq_handler_fn();
	}
}

static void msoft_irq_handler(const void *arg)
{
	ARG_UNUSED(arg);
	/* Clear the machine software interrupt pending bit */
	sys_write8(0, CLIC_HART0_ADDR + CLIC_INTIP + MSOFT_IRQN);
}

/*
 * btblecontroller_ble_irq_init — Register and enable the BLE interrupt
 *
 * The handler is a bare function pointer from the controller blob.
 * We bridge it through a Zephyr-compatible ISR wrapper to avoid
 * calling through an incompatible function pointer type.
 *
 * Also registers the MSOFT handler: the blob triggers machine software
 * interrupts for deferred context switch requests (FreeRTOS portYIELD).
 */
void btblecontroller_ble_irq_init(void *handler)
{
	ble_irq_handler_fn = (void (*)(void))handler;

	/* Clear any pending BLE interrupt */
	sys_write8(0, CLIC_HART0_ADDR + CLIC_INTIP + BLE_IRQn);

	irq_connect_dynamic(BLE_IRQn, 0, ble_irq_bridge, NULL, 0);
	irq_enable(BLE_IRQn);

	/* Register MSOFT handler — the BLE controller blob sets MSOFT pending
	 * to request a context switch after BLE IRQ processing.
	 */
	sys_write8(0, CLIC_HART0_ADDR + CLIC_INTIP + MSOFT_IRQN);
	irq_connect_dynamic(MSOFT_IRQN, 1, msoft_irq_handler, NULL, 0);
	irq_enable(MSOFT_IRQN);
}

void btblecontroller_ble_irq_enable(uint8_t enable)
{
	if (enable) {
		irq_enable(BLE_IRQn);
	} else {
		irq_disable(BLE_IRQn);
	}
}

/*
 * btblecontroller_enable_ble_clk — Enable/disable BLE peripheral clock
 *
 * Implemented in bl_platform_shim.c as GLB_Set_BLE_CLK() which handles
 * the proper register sequencing for clock gate control.
 */
extern uint32_t GLB_Set_BLE_CLK(uint8_t enable);

void btblecontroller_enable_ble_clk(uint8_t enable)
{
	GLB_Set_BLE_CLK(enable);
}

void btblecontroller_rf_restore(void)
{
	/* BL702L: no RF restore needed (BL616-specific feature) */
}

void btblecontroller_sys_reset(void)
{
	sys_reboot(0);
}

void btblecontroller_puts(const char *str)
{
	printk("%s", str);
}

int btblecontroller_printf(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vprintk(fmt, args);
	va_end(args);
	return 0;
}
