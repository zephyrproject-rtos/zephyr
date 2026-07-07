/*
 * Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 *
 * Hardware port glue for the on-chip BLE controller on BL808.
 *
 * The controller reaches the BLE baseband through Exchange Memory (EM) at
 * bus address 0x28000000. EM_SEL (GLB_SRAM_CFG3[7:0]) repurposes the top
 * WRAM banks as EM; the CPU accesses that memory through the bridge. CPU
 * writes to 0x28000000 must be Bufferable — Strongly Ordered writes are
 * silently dropped by the bridge. EM_SEL is set in soc_prep_hook, before
 * the controller heap is initialised.
 *
 * Provides printf/puts stubs, flash read (XIP), IRQ glue, and efuse MAC.
 */

#include <zephyr/kernel.h>
#include <zephyr/irq.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/drivers/hwinfo.h>

int bl_flash_read(uint32_t addr, uint8_t *dst, int len)
{
	memcpy(dst, (const uint8_t *)(0x58000000 + addr), len);
	return 0;
}

int bl_flash_erase(uint32_t addr, int len)
{
	ARG_UNUSED(addr);
	ARG_UNUSED(len);
	return -1;
}

int bl_flash_write(uint32_t addr, const uint8_t *src, int len)
{
	ARG_UNUSED(addr);
	ARG_UNUSED(src);
	ARG_UNUSED(len);
	return -1;
}

int bl_efuse_read_mac_smart(uint8_t smart, uint8_t mac[6], uint8_t slot)
{
	ARG_UNUSED(smart);
	ARG_UNUSED(slot);

	return (hwinfo_get_device_id(mac, 6) >= 6) ? 0 : -1;
}

int EF_Ctrl_Read_MAC_Address(uint8_t mac[6])
{
	return (hwinfo_get_device_id(mac, 6) >= 6) ? 0 : -1;
}

void bl_irq_register(int irqnum, void *handler)
{
	irq_connect_dynamic(irqnum, 0, (void (*)(const void *))handler, NULL, 0);
}

void bl_irq_enable(unsigned int source)
{
	irq_enable(source);
}

void bl_irq_disable(unsigned int source)
{
	irq_disable(source);
}

void bl_irq_pending_clear(unsigned int source)
{
	ARG_UNUSED(source);
}
